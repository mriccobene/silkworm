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

#include "BlockRequestLogic.hpp"
#include "HeaderLogic.hpp"
#include "messages/InboundGetBlockHeaders.hpp"
#include "rpc/ReceiveMessages.hpp"
#include "rpc/SendMessageById.hpp"
#include "rpc/SetStatus.hpp"
#include "ConcurrentContainers.hpp"

namespace silkworm {

Stage1::Stage1(ChainConfig chain, Hash genesis_hash, std::vector<BlockNum> hard_forks, std::string db_path, std::string sentry_addr):
    chain_{chain}, genesis_hash_{genesis_hash}, hard_forks_{hard_forks},
    db_{db_path},
    channel_{grpc::CreateChannel(sentry_addr, grpc::InsecureChannelCredentials())},
    sentry_{channel_}
{
}

void Stage1::execution_loop() { // no-thread version
    using std::shared_ptr;
    using namespace std::chrono_literals;

    ConcurrentQueue<shared_ptr<InboundMessage>> incoming_msgs{};
    ConcurrentQueue<shared_ptr<OutboundMessage>> outgoing_msgs{};

    auto [head_hash, head_td] = HeaderLogic::head_hash_and_total_difficulty(db_);

    // set status
    auto set_status = rpc::SetStatus::make(chain_, genesis_hash_, hard_forks_, head_hash, head_td);
    sentry_.exec_remotely(set_status);

    // start message receiving
    auto receive_messages = rpc::ReceiveMessages::make();
    receive_messages->on_receive_reply([&](auto&) { // unused parameter "call"
      sentry::InboundMessage& raw_reply = receive_messages->reply();
      auto reply = InboundMessage::make(raw_reply);
      incoming_msgs.push(reply);
    });
    sentry_.exec_remotely(receive_messages);

    // receive message
    std::thread receiving{[&]() {
      while (!exiting_) {
          auto executed_rpc = sentry_.receive_one_result();
            // check executed_rpc status?
          if (executed_rpc && executed_rpc->terminated()) {
              auto status = executed_rpc->status();
              if (status.ok())
                  std::cout << "RPC ok";
              else
                  std::cerr << "RPC failed, error: " << status.error_message() << ", code: " << status.error_code()
                            << ", details: " << status.error_details() << std::endl;
          }
        }
    }};

    // reply to inbound requests
    std::thread responding{[&]() {
      while (!exiting_) {
          shared_ptr<InboundMessage> msg;
          if (incoming_msgs.timed_wait_and_pop(msg, 1000ms)) {
              shared_ptr<OutboundMessage> reply = msg->execute();
              if (reply) {
                  auto reply_rpc = reply->create_send_rpc();
                  sentry_.exec_remotely(reply_rpc);
              }
          }
      }
    }};

    // make outbound requests
    std::thread requesting{[&]() {
      while (!exiting_) {
          // check if we need to do a request to the peers
          shared_ptr<OutboundMessage> request = BlockRequestLogic::execute();
          if (request) {
              auto rpc = request->create_send_rpc();
              rpc->on_receive_reply([request](auto& call) { // copy request and retain its lifetime (shared_ptr)
                request->receive_reply(call);    // call convey reply (SentPeers or Empty)
              });
              sentry_.exec_remotely(rpc);
          }
          else
            std::this_thread::sleep_for(5s);
      }
    }};

    receiving.join();
    responding.join();
    requesting.join();
}

}  // namespace silkworm

