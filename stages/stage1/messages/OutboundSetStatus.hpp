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

#ifndef SILKWORM_OUTBOUNDSETSTATUS_HPP
#define SILKWORM_OUTBOUNDSETSTATUS_HPP

#include <string>
#include <vector>
#include <silkworm/chain/config.hpp>
#include "OutboundMessage.hpp"

namespace silkworm {

class OutboundSetStatus: public OutboundMessage {
  public:
    using call_t = rpc::AsyncUnaryCall<sentry::Sentry, sentry::StatusData, google::protobuf::Empty>;
    using callback_t = std::function<void(call_t&)>; // callback to handle reply

    OutboundSetStatus(ChainConfig chain, Hash genesis, std::vector<BlockNum> hard_forks, Hash best_hash, BigInt total_difficulty);

    void on_receive_reply(callback_t callback) {callback_ = callback;}

  private:
    std::shared_ptr<call_base_t> create_send_call() override;
    void receive_reply(call_base_t& call) override;

    sentry::StatusData packet_;
    callback_t callback_;
};

}
#endif  // SILKWORM_OUTBOUNDSETSTATUS_HPP
