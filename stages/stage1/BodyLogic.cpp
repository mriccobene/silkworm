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

#include "BodyLogic.hpp"
#include <silkworm/types/block.hpp>

namespace silkworm {
/*
func AnswerGetBlockBodiesQuery(db ethdb.Tx, query GetBlockBodiesPacket) []rlp.RawValue { //nolint:unparam
	// Gather blocks until the fetch or network limits is reached
	var (
		bytes  int
		bodies []rlp.RawValue
	)
	for lookups, hash := range query {
		if bytes >= softResponseLimit || len(bodies) >= maxBodiesServe ||
			lookups >= 2*maxBodiesServe {
			break
		}
		number := rawdb.ReadHeaderNumber(db, hash)
		if number == nil {
			continue
		}
		data := rawdb.ReadBodyRLP(db, hash, *number)
		if len(data) == 0 {
			continue
		}
		bodies = append(bodies, data)
		bytes += len(data)
	}
	return bodies
}
 */

std::vector<BlockBody> BodyLogic::recover([[maybe_unused]] std::vector<Hash> request) {
    std::vector<BlockBody> response;

    // todo: implements!

    return response;
}

}