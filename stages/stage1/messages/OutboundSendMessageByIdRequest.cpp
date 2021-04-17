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

#include "OutboundSendMessageByIdRequest.hpp"
#include "SendMessageById.hpp"

namespace silkworm {

OutboundSendMessageByIdRequest::OutboundSendMessageByIdRequest(const std::string& peerId, std::unique_ptr<sentry::OutboundMessageData> message) {
    packet_.set_peer_id(peerId);
    packet_.set_allocated_data(message.release());  // take ownership
}

std::shared_ptr<SentryRpc> OutboundSendMessageByIdRequest::create_send_rpc() {
    return std::make_shared<rpc::SendMessageById>(packet_);
}

void OutboundSendMessageByIdRequest::receive_reply(SentryRpc& base_rpc) {
    //auto rpc = std::dynamic_pointer_cast<rpc::SendMessageById>(base_rpc);
    auto& rpc = dynamic_cast<rpc::SendMessageById&>(base_rpc);
    // use rpc.status_
    sentry::SentPeers peers = rpc.reply();
    // use peers
}

}