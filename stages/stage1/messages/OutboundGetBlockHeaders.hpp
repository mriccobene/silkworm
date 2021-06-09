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

#ifndef SILKWORM_OUTBOUNDGETBLOCKHEADERS_HPP
#define SILKWORM_OUTBOUNDGETBLOCKHEADERS_HPP

#include <stages/stage1/packets/GetBlockHeadersPacket.hpp>
#include "OutboundMessage.hpp"

namespace silkworm {

class OutboundGetBlockHeaders : public OutboundMessage {
  public:
    OutboundGetBlockHeaders();

    std::string name() const override {return "OutboundGetBlockHeaders";}
    std::string content() const override;
    uint64_t reqId() const override;

    request_calls_t execute() override;

    void handle_completion(SentryRpc&) override;

  private:
    GetBlockHeadersPacket66 packet_;
};

}
#endif  // SILKWORM_OUTBOUNDGETBLOCKHEADERS_HPP
