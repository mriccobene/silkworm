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

#ifndef SILKWORM_RECEIVEUPLOADMESSAGES_HPP
#define SILKWORM_RECEIVEUPLOADMESSAGES_HPP

#include "stages/stage1/SentryClient.hpp"

namespace silkworm::rpc {

class ReceiveUploadMessages: public rpc::AsyncOutStreamingCall<sentry::Sentry, google::protobuf::Empty, sentry::InboundMessage> {
  public:
    ReceiveUploadMessages();

    using SentryRpc::on_receive_reply;

    static std::shared_ptr<ReceiveUploadMessages> make() {return std::make_shared<ReceiveUploadMessages>();}
};

}

#endif  // SILKWORM_RECEIVEUPLOADMESSAGES_HPP
