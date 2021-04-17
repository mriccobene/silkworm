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

#include "InStreamMessage.hpp"

namespace silkworm {

void InStreamMessage::start_via(SentryClient& sentry) {
    call_ = create_send_call();

    std::weak_ptr<OutboundMessage> origin = weak_from_this();

    call_->on_complete([origin](call_base_t& call) {
      if (origin.expired()) return;
      auto message = origin.lock();
      message->receive_reply(call);
    });

    sentry.exec_remotely(call_);
}

}
