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

#ifndef SILKWORM_INBOUNDGETBLOCKHEADERS_HPP
#define SILKWORM_INBOUNDGETBLOCKHEADERS_HPP

#include <variant>

#include <grpcpp/grpcpp.h>
#include <interfaces/sentry.grpc.pb.h>

#include <silkworm/rlp/decode.hpp>
#include <silkworm/rlp/encode.hpp>

#include "stages/stage1/Types.hpp"
#include "stages/stage1/HashOrNumber.hpp"

#include "InboundMessage.hpp"

namespace silkworm {

class InboundGetBlockHeaders: public InboundMessage {
  public:
    InboundGetBlockHeaders(const sentry::InboundMessage& msg);

    reply_t execute() override;

  private:
    struct GetBlockHeadersPacket {
        //uint64_t requestId; // eth/66 version
        HashOrNumber origin;  // Block hash or block number from which to retrieve headers
        uint64_t amount;      // Maximum number of headers to retrieve
        uint64_t skip;        // Blocks to skip between consecutive headers
        bool reverse;         // Query direction (false = rising towards latest, true = falling towards genesis)
    };

    static rlp::DecodingResult decode(ByteView& from, GetBlockHeadersPacket& to) noexcept;

    std::string peerId_;
    GetBlockHeadersPacket packet_;
};

}
#endif  // SILKWORM_INBOUNDGETBLOCKHEADERS_HPP
