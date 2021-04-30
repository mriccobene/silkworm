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
#include "rpc/ReceiveMessages.hpp"
#include "rpc/SendMessageById.hpp"
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

    // NOTE:
    // this version is less readable of the previous one; perhaps it is a useful starting point for coroutinizing the stage1

    using work_t = std::function<void(void)>;
    ConcurrentQueue<work_t> work_queue{};

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
            auto in_msg = InboundMessage::make(reply);
            if (in_msg) {
                work_t work = [this, in_msg]() {
                  SILKWORM_LOG(LogDebug) << "Processing message\n";
                  shared_ptr<OutboundMessage> out_msg = in_msg->execute();  // process request
                  if (!out_msg) return;
                  SILKWORM_LOG(LogInfo) << "Replying to request\n";
                  auto out_rpc = out_msg->create_send_rpc();
                  out_rpc->on_receive_reply([out_msg](auto& call) { // copy request and retain its lifetime (shared_ptr)
                    SILKWORM_LOG(LogInfo) << "Received reply to our request\n";
                    out_msg->receive_reply(call);    //  call convey reply (SentPeers or Empty)  // WARNING, this will be executed in another thread todo: fix this
                  });
                  sentry_.exec_remotely(out_rpc);
                };
                work_queue.push(work);
            }
        }
        else {
            exiting_ = true;
            SILKWORM_LOG(LogCritical) << "Receiving messages stream interrupted, cause:'" << call.status().error_message() << "', exiting...\n";
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
    std::thread work_processing{[&]() {
      while (!exiting_) {
          work_t work;
          bool present = work_queue.timed_wait_and_pop(work, 1000ms);
          if (!present) continue;   // timeout
          SILKWORM_LOG(LogDebug) << "Executing work\n";
          work();
      }
      SILKWORM_LOG(LogInfo) << "work_processing thread exiting...\n";
    }};


    // make outbound requests
    std::thread requesting{[&]() {
      while (!exiting_) {
          // check if we need to do a request to the peers
          work_t work = [this]() {
              shared_ptr<OutboundMessage> request = BlockRequestLogic::execute();
              if (request) {
                  SILKWORM_LOG(LogInfo) << "Sending request\n"; // todo: log message name
                  auto rpc = request->create_send_rpc();
                  rpc->on_receive_reply([request](auto& call) { // copy request and retain its lifetime (shared_ptr)
                    SILKWORM_LOG(LogInfo) << "Received reply to our request\n"; // todo: log message name
                    request->receive_reply(call);    // call convey reply (SentPeers or Empty) // WARNING, this will be executed in another thread todo: fix this
                  });
                  sentry_.exec_remotely(rpc);
              }
          };
          work_queue.push(work);
          std::this_thread::sleep_for(5s);
      }
      SILKWORM_LOG(LogInfo) << "requesting thread exiting...\n";
    }};

    rpc_handling.join();
    work_processing.join();
    requesting.join();
}

}  // namespace silkworm

