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

#ifndef SILKWORM_INBOUNDMESSAGE_HPP
#define SILKWORM_INBOUNDMESSAGE_HPP

#include <memory>
#include "Message.hpp"
#include "stages/stage1/SentryClient.hpp"

namespace silkworm {
class OutboundMessage;


class InboundMessage : public Message {
  public:
    using reply_t = std::shared_ptr<OutboundMessage>;

    virtual reply_t execute() = 0;

    virtual ~InboundMessage() = default;

    static std::shared_ptr<InboundMessage> make(const sentry::InboundMessage& msg);
};

}
#endif  // SILKWORM_INBOUNDMESSAGE_HPP
