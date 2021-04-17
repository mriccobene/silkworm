//
// Created by miche on 13/04/2021.
//

#ifndef SILKWORM_HEADERLOGIC_HPP
#define SILKWORM_HEADERLOGIC_HPP

#include <vector>
#include "DbTx.hpp"
#include "Types.hpp"

namespace silkworm {

class HeaderLogic {     // todo: modularize this!
  public:
    static std::vector<Header> recoverByHash(Hash origin, uint64_t amount, uint64_t skip, bool reverse);
    static std::vector<Header> recoverByNumber(BlockNum origin, uint64_t amount, uint64_t skip, bool reverse);

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
