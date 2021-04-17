//
// Created by miche on 16/04/2021.
//

#include "OutboundSendMessageByIdRequest.hpp"

namespace silkworm {

OutboundSendMessageByIdRequest::OutboundSendMessageByIdRequest(const std::string& peerId,
                                                               std::unique_ptr<sentry::OutboundMessageData> message) {
    packet_.set_peer_id(peerId);
    packet_.set_allocated_data(message.release());  // take ownership
}

auto OutboundSendMessageByIdRequest::create_send_call() -> std::shared_ptr<call_base_t> {
    auto call = std::make_shared<call_t>(&sentry::Sentry::Stub::PrepareAsyncSendMessageById, packet_); // todo: packet_ copyed, think about this again
    return call;
}

void OutboundSendMessageByIdRequest::receive_reply(call_base_t& call_base) {
    if (!callback_) return;
    auto& call = dynamic_cast<call_t&>(call_base);
    callback_(call);
}

}
