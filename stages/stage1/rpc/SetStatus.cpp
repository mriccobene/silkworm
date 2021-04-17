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

#include "SetStatus.hpp"

namespace silkworm::rpc {

SetStatus::SetStatus(ChainConfig chain, Hash genesis, std::vector<BlockNum> hard_forks, Hash best_hash, BigInt total_difficulty):
    AsyncUnaryCall(&sentry::Sentry::Stub::PrepareAsyncSetStatus, {})
{
    request_.set_network_id(chain.chain_id);

    ByteView td = rlp::big_endian(total_difficulty);    // remove trailing zeros - WARNING it uses thread_local var
    request_.set_total_difficulty(td.data(), td.length());

    request_.set_best_hash(best_hash.raw_bytes(), best_hash.length());

    auto* forks = new sentry::Forks{};
    forks->set_genesis(genesis.raw_bytes(), genesis.length());
    for(uint64_t block: hard_forks)
        forks->add_forks(block);
    request_.set_allocated_fork_data(forks); // take ownership
}

}
