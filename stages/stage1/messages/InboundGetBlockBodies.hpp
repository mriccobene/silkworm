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

#ifndef SILKWORM_INBOUNDGETBLOCKBODIES_HPP
#define SILKWORM_INBOUNDGETBLOCKBODIES_HPP

#include "InboundMessage.hpp"
#include "stages/stage1/packets/GetBlockBodiesPacket.hpp"

namespace silkworm {

class InboundGetBlockBodies: public InboundMessage {
  public:
    InboundGetBlockBodies(const sentry::InboundMessage& msg);

    std::string name() const override {return "InboundGetBlockBodies";}
    std::string content() const override;

    reply_calls_t execute() override;

    void handle_completion(SentryRpc&) override;

  private:
    std::string peerId_;
    GetBlockBodiesPacket66 packet_;
};

}
#endif  // SILKWORM_INBOUNDGETBLOCKBODIES_HPP
