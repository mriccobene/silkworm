/*
   Copyright 2021 The Silkworm Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "stage1.hpp"

#include <chrono>

#include <silkworm/common/log.hpp>

#include "BlockRequestLogic.hpp"
#include "HeaderLogic.hpp"
#include "messages/InboundGetBlockHeaders.hpp"
#include "messages/OutboundGetBlockHeaders.hpp"
#include "messages/CompletionMessage.hpp"
#include "rpc/ReceiveMessages.hpp"
#include "rpc/SetStatus.hpp"
#include "ConcurrentContainers.hpp"

namespace silkworm {

Stage1::Stage1(ChainIdentity chain_identity, std::string db_path, std::string sentry_addr):
    chain_identity_(std::move(chain_identity)),
    db_{db_path},
    sentry_{sentry_addr}
{
}

void Stage1::execution_loop() { // no-thread version
    using std::shared_ptr;
    using namespace std::chrono_literals;

    ConcurrentQueue<shared_ptr<Message>> messages{};

    auto [head_hash, head_td] = HeaderLogic::head_hash_and_total_difficulty(db_);

    // set status
    auto set_status = rpc::SetStatus::make(chain_identity_.chain, chain_identity_.genesis_hash, chain_identity_.hard_forks, head_hash, head_td);
    set_status->on_receive_reply([&](auto& call) {
      if (!call.status().ok()) {
          exiting_ = true;
          SILKWORM_LOG(LogCritical) << "failed to set status to the remote sentry, cause:'" << call.status().error_message() << "', exiting...\n";
      }
    });
    sentry_.exec_remotely(set_status);
    std::this_thread::sleep_for(3s); // wait for connection setup before submit other requests

    // start message receiving
    auto receive_messages = rpc::ReceiveMessages::make();
    receive_messages->on_receive_reply([&](auto& call) {
      if (!call.terminated()) {
          sentry::InboundMessage& reply = receive_messages->reply();
          auto message = InboundMessage::make(reply);
          if (message) {
              SILKWORM_LOG(LogInfo) << "Message received from remote peer: " << message->name() << "\n";
              messages.push(message);
          }
      }
      else {
          exiting_ = true;
          SILKWORM_LOG(LogCritical) << "receiving messages stream interrupted, cause:'" << call.status().error_message() << "', exiting...\n";
      }
    });
    sentry_.exec_remotely(receive_messages);

    // handling async rpc
    std::thread rpc_handling{[&]() {    // todo: add try...catch to trap exceptions and set exiting_=true to cause other thread exiting
      while (!exiting_) {
          auto executed_rpc = sentry_.receive_one_result();
          // check executed_rpc status?
          if (executed_rpc && executed_rpc->terminated()) {
              auto status = executed_rpc->status();
              if (status.ok())
                  SILKWORM_LOG(LogDebug) << "RPC ok, " << executed_rpc->name() << "\n";
              else
              SILKWORM_LOG(LogWarn) << "RPC failed, " << executed_rpc->name() << ", error: '" << status.error_message() << "', code: " << status.error_code()
                                    << ", details: " << status.error_details() << std::endl;
          }
      }
      SILKWORM_LOG(LogInfo) << "rpc_handling thread exiting...\n";
    }};

    // reply to inbound requests
    std::thread message_processing{[&]() {
      while (!exiting_) {
          shared_ptr<Message> message;
          bool present = messages.timed_wait_and_pop(message, 1000ms);
          if (!present) continue;   // timeout

          SILKWORM_LOG(LogDebug) << "Processing message " << message->name() << "\n";
          shared_ptr<SentryRpc> rpc = message->execute();
          if (!rpc) continue;

          if (std::dynamic_pointer_cast<InboundMessage>(message))
              SILKWORM_LOG(LogInfo) << "Replying to incoming request " << message->name() << "\n";
          else // OutboundMessage
              SILKWORM_LOG(LogInfo) << "Sending outgoing request " << message->name() << "\n";

          rpc->on_receive_reply([message, rpc, &messages](auto&) { // copy message and rpc to retain their lifetime (shared_ptr) [avoid rpc passing using make_shared_from_this in AsyncCall]
            SILKWORM_LOG(LogInfo) << "Received rpc result of " << message->name() << "\n";
            //message->handle_completion(call);  // call convey reply (SentPeers or Empty), dangerous... will be executed in another thread
            shared_ptr<Message> completion = CompletionMessage::make(message, rpc);
            messages.push(completion); // coroutines would avoid this
          });

          sentry_.exec_remotely(rpc);
      }
      SILKWORM_LOG(LogInfo) << "message_processing thread exiting...\n";
    }};


    // make outbound requests
    std::thread request_generation{[&]() {
      while (!exiting_) {
          shared_ptr<Message> message = std::make_shared<OutboundGetBlockHeaders>();
          messages.push(message);

          std::this_thread::sleep_for(5s);
      }
      SILKWORM_LOG(LogInfo) << "request_generation thread exiting...\n";
    }};

    rpc_handling.join();
    message_processing.join();
    request_generation.join();
}

}  // namespace silkworm

