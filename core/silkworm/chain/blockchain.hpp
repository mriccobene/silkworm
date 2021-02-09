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

#ifndef SILKWORM_CHAIN_BLOCKCHAIN_HPP_
#define SILKWORM_CHAIN_BLOCKCHAIN_HPP_

#include <silkworm/chain/validity.hpp>
#include <silkworm/state/buffer.hpp>

namespace silkworm {

class Blockchain {
  public:
    Blockchain(StateBuffer& state, const ChainConfig& config, const Block& genesis_block);

    Blockchain(const Blockchain&) = delete;
    Blockchain& operator=(const Blockchain&) = delete;

    ValidationError insert_block(Block& block, bool check_state_root);

  private:
    ValidationError execute_and_canonize_block(const Block& block, const evmc::bytes32& hash, bool check_state_root);

    ValidationError canonize_chain(const Block& block, evmc::bytes32 hash, uint64_t canonical_ancestor,
                                   bool check_state_root);

    void decanonize_chain(uint64_t back_to);

    uint64_t canonical_ancestor(const BlockHeader& header, const evmc::bytes32& hash) const;

    StateBuffer& state_;
    const ChainConfig& config_;
};

}  // namespace silkworm

#endif  // SILKWORM_CHAIN_BLOCKCHAIN_HPP_