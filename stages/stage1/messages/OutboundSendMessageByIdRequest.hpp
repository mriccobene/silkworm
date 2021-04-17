//
// Created by miche on 16/04/2021.
//

#ifndef SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP
#define SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP

#include <string>
#include <memory>
#include "OutboundMessage.hpp"

namespace silkworm {

class OutboundSendMessageByIdRequest: public OutboundMessage {
  public:
    using call_t = rpc::AsyncUnaryCall<sentry::Sentry, sentry::SendMessageByIdRequest, sentry::SentPeers>;
    using callback_t = std::function<void(call_t&)>; // callback to handle reply

    OutboundSendMessageByIdRequest(const std::string& peerId, std::unique_ptr<sentry::OutboundMessageData> message);

    void on_receive_reply(callback_t callback) {callback_ = callback;}

  protected:
    std::shared_ptr<call_base_t> create_send_call() override;
    void receive_reply(call_base_t& call) override;

  private:
    sentry::SendMessageByIdRequest packet_;
    callback_t callback_;
};

}
#endif  // SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP
