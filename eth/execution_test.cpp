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

#include "execution.hpp"

#include <evmc/evmc.hpp>

#include "../tests/catch.hpp"
#include "common.hpp"

namespace silkworm::eth {

TEST_CASE("validation", "[execution]") {
  using namespace evmc::literals;

  evmc::address miner = 0x829bd824b016326a401d083b33d092293333a830_address;
  uint64_t block_number{1};

  eth::Transaction txn{
      .nonce = 12,
      .gas_price = 20000000000,
      .gas_limit = 21000,
      .to = 0x727fc6a68321b754475c668a6abfb6e9e71c169a_address,
      .value = 10 * kEther,
  };

  IntraBlockState state;
  ExecutionProcessor processor{state, miner, block_number};

  ExecutionResult res = processor.execute_transaction(txn);
  CHECK(res.error == ValidationError::kMissingSender);

  txn.from = 0x68d7899b6635146a37d01934461d0c9e4b65ddda_address;
  res = processor.execute_transaction(txn);
  CHECK(res.error == ValidationError::kMissingSender);

  // TODO(Andrew) other validation errors
}

}  // namespace silkworm::eth
