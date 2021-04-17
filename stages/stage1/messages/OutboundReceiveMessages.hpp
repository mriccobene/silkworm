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

#ifndef SILKWORM_OUTBOUNDRECEIVEMESSAGES_HPP
#define SILKWORM_OUTBOUNDRECEIVEMESSAGES_HPP

#include "InStreamMessage.hpp"
#include "InboundMessage.hpp"

namespace silkworm {

class OutboundReceiveMessages: public InStreamMessage {
  public:
    using call_t = rpc::AsyncOutStreamingCall<sentry::Sentry, google::protobuf::Empty, sentry::InboundMessage>;
    using callback_t = std::function<void(std::shared_ptr<InboundMessage>)>; // callback to handle reply

    OutboundReceiveMessages();

    void on_receive_reply(callback_t callback) {callback_ = callback;}

    callback_t callback() {return callback_;}

  protected:
    std::shared_ptr<call_base_t> create_send_call() override;
    void receive_reply(call_base_t& call) override;

    callback_t callback_;
};

}
#endif  // SILKWORM_OUTBOUNDRECEIVEMESSAGES_HPP
