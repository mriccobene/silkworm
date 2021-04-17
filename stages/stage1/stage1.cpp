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
#include "ThreadSafeQueue.hpp"
#include "OutboundReceiveMessages.hpp"
#include "OutboundSetStatus.hpp"
#include "HeaderLogic.hpp"
#include "BlockRequestLogic.hpp"


template <typename T>
using ConcurrentQueue = ThreadSafeQueue<T>;  // todo: use a better alternative from a known library (Intel oneTBB?)

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

    auto set_status = std::make_shared<OutboundSetStatus>(chain_, genesis_hash_, hard_forks_, head_hash, head_td);
    set_status->send_via(sentry_); // todo: ignore response?

    auto receive_messages = std::make_shared<OutboundReceiveMessages>();
    receive_messages->on_receive_reply([&](std::shared_ptr<InboundMessage> msg) {
        incoming_msgs.push(msg);
    });
    receive_messages->start_via(sentry_);

    std::thread receiving{[&]() {
      while (!exiting_) {
          auto* executed_rpc = sentry_.receive_one_result();
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

    std::thread responding{[&]() {
      while (!exiting_) {
          shared_ptr<InboundMessage> msg;
          if (incoming_msgs.timed_wait_and_pop(msg, 1000ms)) {
              shared_ptr<OutboundMessage> reply = msg->execute();
              if (reply) {
                  reply->send_via(sentry_); // todo: ignore reply of reply (SentPeers or Empty)?
              }
          }
      }
    }};

    std::thread requesting{[&]() {
      while (!exiting_) {
          // check if we need to do a request to the peers
          shared_ptr<OutboundMessage> request = BlockRequestLogic::execute();
          if (!request)
              std::this_thread::sleep_for(5s);
          request->send_via(sentry_); // todo: ingnore reply (SentPeers or Empty)?
      }
    }};

    receiving.join();
    responding.join();
    requesting.join();
}


/*
void Stage1::execution_loop() { // no-thread version
    using std::shared_ptr;
    using namespace std::chrono_literals;

    auto [head_hash, head_td] = Header::head_hash_and_total_difficulty(db_);

    auto set_status = make_shared<OutboundSetStatus>(chain_, genesis_hash_, hard_forks_, head_hash, head_td);
    sentry.send(set_status);

    auto start_receiving = make_shared<OutboundReceiveMessages>();
    sentry.send(start_receiving);

    while (!exiting_) {
        shared_ptr<Message> message = client.receive_one(timeout_5_sec_);  // la complete() ritorna il reply

        // if there is an incoming request from a peer, satisfy it
        if (message) {
            shared_ptr<Message> reply = message->execute();  // reply è un msg ma per mandarsi avvia una call
            if (reply) {
                reply->execute();
                sentry.send(reply);
            }

        }

        // check if we need to do a request to the peers
        shared_ptr<Message> request = make_shared<out_messages::GetBlockHeaders>();
        request->execute();
        sentry.send(request);
    }
}
*/

// other versions -----------------------------------------------------------------------------------------------------
/*
void Stage1::execution_loop() { // threaded version
    using std::shared_ptr;
    using namespace std::chrono_literals;

    ConcurrentQueue<shared_ptr<Message>> incoming_msgs{};
    ConcurrentQueue<shared_ptr<Message>> outgoing_msgs{};

    std::thread receiving{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg = client.receive_one();
          incoming_msgs.push(msg);
      }
    }};

    std::thread executing{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (incoming_msgs.timed_wait_and_pop(msg, 1000ms)) {
              shared_ptr<Message> reply = msg->execute();
              if (reply) {
                  reply->execute();
                  outgoing_msgs.push(reply);
              }
          }
      }
    }};

    std::thread requesting{[&]() {
      while (!exiting_) {
          shared_ptr<Message> request = SomeLogic()->generate_request();
          if (request) {
              request->execut();
              outgoing_msgs.push(reply);
          }
          sleep(5secs);
      }
    }};

    std::thread sending{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (outgoing_msgs.timed_wait_and_pop(msg, 1000ms))
              client.send(msg);
      }
    }};

    receiving.join();
    executing.join();
    sending.join();
}
*/
/*
void Stage1::execution_loop() {
    using std::shared_ptr;
    using namespace std::chrono_literals;

    ConcurrentQueue<shared_ptr<Message>> incoming_msgs{};
    ConcurrentQueue<shared_ptr<Message>> outgoing_msgs{};

    auto set_status = make_shared<out_messages::SetCurrentStatus>(chain_, genesis_hash_, hard_forks_); // compute other
    client_.send(set_status);

    auto start_receiving = make_shared<out_messages::ReceiveMessages>();
    start_receiving->use(outgoing_msgs);
    client_.send(start_receiving);

    std::thread receiving{[&]() {
      while (!exiting_) {
          client.receive_one();
      }
    }};

    std::thread executing{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (incoming_msgs.timed_wait_and_pop(msg, 1000ms))
              msg->execute();
      }
    }};

    std::thread sending{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (outgoing_msgs.timed_wait_and_pop(msg, 1000ms))
              client.send(msg);
      }
    }};

    receiving.join();
    executing.join();
    sending.join();
}

*/

/*
void execution_loop() override {
    using std::shared_ptr;
    using namespace std::chrono_literals;

    ConcurrentQueue<shared_ptr<Message>> incoming_msgs{};
    ConcurrentQueue<shared_ptr<Message>> outgoing_msgs{};

    std::thread receiving{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg = client.receive_one();
          incoming_msgs.push(msg);
      }
    }};

    std::thread executing{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (incoming_msgs.timed_wait_and_pop(msg, 1000ms))
              msg->execute();
      }
    }};

    std::thread sending{[&]() {
      while (!exiting_) {
          shared_ptr<Message> msg;
          if (outgoing_msgs.timed_wait_and_pop(msg, 1000ms))
              client.send(msg);
      }
    }};

    receiving.join();
    executing.join();
    sending.join();
}
*/


/*
namespace coroutine_sample {
    void inbound_loop() {
        auto stream = co_await sentry.receive_messages_stream();
        for(auto message: co_await stream) {
            // process message -> no! blocca tutto
            co_task<void> message_handling = handle_inbound_message(message);
            message_handling.schedule();
        }
    }

    co_task<void> handle_inbound_message(InboundMessage message) {
        // processa message....
        OutboundMessage omessage;
        // riempe omessage
        auto result = co_await sentry.send(omessage);
        if (result) ...
    }

    void outbound_loop() {
        do {
            task<void> create_outbound_message();
            create_outbound_message.schedule();
            sleep(2s);
        } while(!exiting)
    }
}
*/

}  // namespace silkworm

