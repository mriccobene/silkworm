//
// Created by miche on 04/06/2021.
//

#ifndef SILKWORM_RLPTYPES_HPP
#define SILKWORM_RLPTYPES_HPP

#include "stages/stage1/Types.hpp"

namespace silkworm::rlp {

    inline void encode(Bytes& to, const Hash& h) { rlp::encode(to, dynamic_cast<const evmc::bytes32&>(h)); }

    inline rlp::DecodingResult decode(ByteView& from, Hash& to) noexcept {
        return rlp::decode(from, dynamic_cast<evmc::bytes32&>(to));
    }

}

#endif  // SILKWORM_RLPTYPES_HPP
