/*
   Copyright 2020 The Silkworm Authors

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

// TODO(Andrew) implement

#include "intra_block_state.hpp"

namespace silkworm::eth {

bool IntraBlockState::Exists(const evmc::address&) const { return false; }

intx::uint256 IntraBlockState::GetBalance(const evmc::address&) const { return 0; }

void IntraBlockState::AddBalance(const evmc::address&, const intx::uint256&) {}

void IntraBlockState::SubBalance(const evmc::address&, const intx::uint256&) {}

uint64_t IntraBlockState::GetNonce(const evmc::address&) const { return 0; }

void IntraBlockState::SetNonce(const evmc::address&, uint64_t) {}

uint64_t IntraBlockState::GetRefund() const { return 0; }

}  // namespace silkworm::eth
