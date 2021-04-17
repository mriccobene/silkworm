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
#ifndef SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP
#define SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP

#include "OutboundMessage.hpp"

namespace silkworm {

class OutboundSendMessageByIdRequest: public OutboundMessage {
  public:
    OutboundSendMessageByIdRequest(const std::string& peerId, std::unique_ptr<sentry::OutboundMessageData> message);

    virtual std::shared_ptr<SentryRpc> create_send_rpc();

    virtual void receive_reply(SentryRpc&) override;

  private:
    sentry::SendMessageByIdRequest packet_;
};

}
#endif  // SILKWORM_OUTBOUNDSENDMESSAGEBYIDREQUEST_HPP
