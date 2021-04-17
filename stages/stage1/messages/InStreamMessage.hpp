//
// Created by miche on 17/04/2021.
//

#ifndef SILKWORM_INSTREAMMESSAGE_HPP
#define SILKWORM_INSTREAMMESSAGE_HPP

#include <memory>

#include <interfaces/sentry.grpc.pb.h>
#include <stages/stage1/SentryClient.hpp>
#include "stages/stage1/Types.hpp"
#include "InboundMessage.hpp"

namespace silkworm {

class InStreamMessage : public Message, std::enable_shared_from_this<OutboundMessage> {
  public:
    using call_base_t = rpc::AsyncCall<sentry::Sentry>;                       // base RPC to send this message
    using callback_t = std::function<void(std::shared_ptr<InboundMessage>)>;  // callback to handle reply

    void start_via(SentryClient&);

    void on_receive_reply(callback_t callback) { callback_ = callback; }

    callback_t callback() { return callback_; }

  protected:
    virtual std::shared_ptr<call_base_t> create_send_call() = 0;
    virtual void receive_reply(call_base_t& call) = 0;

  private:
    std::shared_ptr<call_base_t> call_;
    callback_t callback_;
};

}

#endif  // SILKWORM_INSTREAMMESSAGE_HPP
