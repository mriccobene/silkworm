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

#ifndef SILKWORM_HEADERLOGIC_HPP
#define SILKWORM_HEADERLOGIC_HPP

#include <vector>
#include "stage1.hpp"

namespace silkworm {

class HeaderLogic {     // todo: modularize this!
  public:
    static const long soft_response_limit = 2 * 1024 * 1024; // Target maximum size of returned blocks, headers or node data.
    static const long est_header_rlp_size = 500;             // Approximate size of an RLP encoded block header
    static const long max_header_fetch = 192;                // Amount of block headers to be fetched per retrieval request

    static std::vector<Header> recoverByHash(Hash origin, uint64_t amount, uint64_t skip, bool reverse) {
        using std::optional;

        DbTx& db = STAGE1.db_tx();  // todo: use singleton, is there a better way? HeaderLogic singleton?

        std::vector<Header> headers;
        long long bytes = 0;
        Hash hash = origin;
        bool unknown = false;

        // first
        optional<Header> header = db.read_header(hash);
        if (!header) return headers;
        BlockNum block_num = header->number;
        headers.push_back(*header);
        bytes += est_header_rlp_size;

        // followings
        do {
            // compute next hash & number - todo: implements! ##################################################
            if (!reverse)   // wrong! implements!
                block_num += skip + 1; // wrong! implements!
            // end todo implements! ############################################################################

            if (unknown) break;

            header = db.read_header(block_num, hash);
            if (!header) break;
            headers.push_back(*header);
            bytes += est_header_rlp_size;

        } while(headers.size() < amount && bytes < soft_response_limit && headers.size() < max_header_fetch);

        return headers;
    }

    static std::vector<Header> recoverByNumber(BlockNum origin, uint64_t amount, uint64_t skip, bool reverse) {
        using std::optional;

        DbTx& db = STAGE1.db_tx(); // todo: use singleton, is there a better way? HeaderLogic singleton?

        std::vector<Header> headers;
        long long bytes = 0;
        BlockNum block_num = origin;

        do {
            optional<Header> header = db.read_canonical_header(block_num);
            if (!header) break;

            headers.push_back(*header);
            bytes += est_header_rlp_size;

            if (!reverse)
                block_num += skip + 1; // Number based traversal towards the leaf block
            else
                block_num -= skip + 1; // Number based traversal towards the genesis block

        } while(block_num > 0 && headers.size() < amount && bytes < soft_response_limit && headers.size() < max_header_fetch);

        return headers;
    }

    // Node current status
    static std::tuple<Hash,BigInt> head_hash_and_total_difficulty(DbTx& db) {
        BlockNum head_height = db.stage_progress(db::stages::kBlockBodiesKey);
        auto head_hash = db.read_canonical_hash(head_height);
        if (!head_hash)
            throw std::logic_error("canonical hash at height " + std::to_string(head_height) + " not found in db");
        std::optional<BigInt> head_td = db.read_total_difficulty(head_height, *head_hash);
        if (!head_td)
            throw std::logic_error("total difficulty of canonical hash at height " + std::to_string(head_height) +
                                   " not found in db");
        return {*head_hash, *head_td};
    }

};

}

#endif  // SILKWORM_HEADERLOGIC_HPP
