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

#include "OutboundSetStatus.hpp"

namespace silkworm {

OutboundSetStatus::OutboundSetStatus(ChainConfig chain, Hash genesis, std::vector<BlockNum> hard_forks, Hash best_hash, BigInt total_difficulty)
{
    packet_.set_network_id(chain.chain_id);

    ByteView td = rlp::big_endian(total_difficulty);    // remove trailing zeros - WARNING it uses thread_local var
    packet_.set_total_difficulty(td.data(), td.length());

    packet_.set_best_hash(best_hash.raw_bytes(), best_hash.length());

    auto* forks = new sentry::Forks{};
    forks->set_genesis(genesis.raw_bytes(), genesis.length());
    for(uint64_t block: hard_forks)
        forks->add_forks(block);
    packet_.set_allocated_fork_data(forks); // take ownership
}

auto OutboundSetStatus::create_send_call() -> std::shared_ptr<call_base_t> {
    auto call = std::make_shared<call_t>(&sentry::Sentry::Stub::PrepareAsyncSetStatus, packet_); // todo: packet_ copyed, think about this again
    return call;
}

void OutboundSetStatus::receive_reply(call_base_t& call_base) {
    if (!callback_) return;
    auto& call = dynamic_cast<call_t&>(call_base);
    callback_(call);
}
}