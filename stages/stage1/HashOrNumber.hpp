//
// Created by miche on 10/04/2021.
//

#ifndef SILKWORM_HASHORNUMBER_HPP
#define SILKWORM_HASHORNUMBER_HPP

#include <variant>

#include <silkworm/rlp/decode.hpp>
#include <silkworm/rlp/encode.hpp>

#include "Types.hpp"

namespace silkworm {

// HashOrNumber type def
using HashOrNumber = std::variant<Hash, BlockNum>;

// HashOrNumber rlp encoding/decoding
namespace rlp {

    class rlp_error : public std::runtime_error {
      public:
        rlp_error() : std::runtime_error("rlp encoding/decoding error") {}
        rlp_error(const std::string& description) : std::runtime_error(description) {}
    };

    void encode(Bytes& to, const HashOrNumber& from) {
        if (std::holds_alternative<Hash>(from))
            rlp::encode(to, std::get<Hash>(from));
        else
            rlp::encode(to, std::get<BlockNum>(from));
    }

    DecodingResult decode(ByteView& from, HashOrNumber& to) noexcept {
        ByteView copy(from);  // to decode but not consume
        auto [h, err] = decode_header(copy);
        if (err != DecodingResult::kOk) {
            return err;
        }

        // uint8_t payload_length = from[0] - 0x80; // in the simple cases that we need here

        if (h.payload_length == 32) {
            Hash hash;
            err = rlp::decode(from, hash);
            to = hash;
        } else if (h.payload_length <= 8) {
            BlockNum number;
            err = rlp::decode(from, number);
            to = number;
        } else {
            err = DecodingResult::kUnexpectedLength;
        }
        return err;
    }

}

}
#endif  // SILKWORM_HASHORNUMBER_HPP
