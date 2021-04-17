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

#include "OutboundReceiveMessages.hpp"
#include "InboundGetBlockHeaders.hpp"

namespace silkworm {

OutboundReceiveMessages::OutboundReceiveMessages() {

}

auto OutboundReceiveMessages::create_send_call() -> std::shared_ptr<call_base_t> {
    google::protobuf::Empty request;
    auto call = std::make_shared<call_t>(&sentry::Sentry::Stub::PrepareAsyncReceiveMessages, request);
    return call;
}

void OutboundReceiveMessages::receive_reply(call_base_t& call_base) {
    if (!callback())
        return;
    //auto specific_call = std::dynamic_pointer_cast<call_t>(call_base);
    auto& call = dynamic_cast<call_t&>(call_base);
    if (call.reply().id() != sentry::MessageId::GetBlockHeaders)
        throw std::logic_error("not implemented yet");
    std::shared_ptr<InboundMessage> message = std::make_shared<InboundGetBlockHeaders>(call.reply());
    callback()(message);
}

}