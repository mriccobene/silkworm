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

#ifndef SILKWORM_OUTBOUNDMESSAGE_HPP
#define SILKWORM_OUTBOUNDMESSAGE_HPP

#include <memory>

#include <interfaces/sentry.grpc.pb.h>
#include <stages/stage1/SentryClient.hpp>
#include "stages/stage1/Types.hpp"
#include "InboundMessage.hpp"

namespace silkworm {

class OutboundMessage : public Message,
                        std::enable_shared_from_this<OutboundMessage> {
  public:
    using call_base_t = rpc::AsyncCall<sentry::Sentry>; // base RPC to send this message

    void send_via(SentryClient&);

  protected:
    virtual std::shared_ptr<call_base_t> create_send_call() =0;
    virtual void receive_reply(call_base_t& call) =0;

  private:
    std::shared_ptr<call_base_t> call_;
};

}
#endif  // SILKWORM_OUTBOUNDMESSAGE_HPP
