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

#ifndef SILKWORM_NEWBLOCKHASHPACKET_HPP
#define SILKWORM_NEWBLOCKHASHPACKET_HPP

#include <silkworm/rlp/decode.hpp>
#include <silkworm/rlp/encode.hpp>

namespace silkworm {

    struct NewBlock {     // one particular block being announced
        Hash hash;        // hash of the block
        BlockNum number;  // number of the block
    };

    struct NewBlockHashesPacket {
        int num_of_elements;  // WARNING: this field is not on the wire
        NewBlock elements[];  // a list of block announcements (len not specified)
    };

namespace rlp {

    inline void encode(Bytes& to, const NewBlock& from) noexcept {
        // todo: check! is there an rlp_header or not?
        rlp::encode(to, from.hash);
        rlp::encode(to, from.number);
    }

    inline rlp::DecodingResult decode(ByteView& from, NewBlock& to) noexcept {
        // todo: check! is there an rlp_header or not?
        /*
        auto [rlp_head, err]{decode_header(from)};
        if (err != DecodingResult::kOk) {
            return err;
        }
        if (!rlp_head.list) {
            return DecodingResult::kUnexpectedString;
        }
        */

        if (DecodingResult err{rlp::decode(from, to.hash)}; err != DecodingResult::kOk) {
            return err;
        }
        if (DecodingResult err{rlp::decode(from, to.number)}; err != DecodingResult::kOk) {
            return err;
        }

        return DecodingResult::kOk; // todo: if there is an header check leftover bytes
    }

    inline size_t length(const NewBlock& from) noexcept {
        // todo: check! is there an rlp_header or not?
        return rlp::length(from.hash) + rlp::length(from.number);
    }

    inline void encode(Bytes& to, const NewBlockHashesPacket& from) noexcept {
        rlp::Header rlp_head{true, 0};

        if (from.num_of_elements == 0) {
            rlp::encode_header(to, rlp_head);
            return;
        }

        rlp_head.payload_length += rlp::length(from.elements[0]) * from.num_of_elements;
        rlp::encode_header(to, rlp_head);

        // do not encode from.num_of_elements, it is not part of the NewBlockHashesPacket

        for(int i = 0; i < from.num_of_elements; i++) {
            encode(to, from.elements[i]);
        }
    }

    inline rlp::DecodingResult decode(ByteView& from, NewBlockHashesPacket& to) noexcept {
        using namespace rlp;

        auto [rlp_head, err] = decode_header(from);
        if (err != DecodingResult::kOk) {
            return err;
        }
        if (!rlp_head.list) {
            return DecodingResult::kUnexpectedString;
        }

        uint64_t leftover{from.length() - rlp_head.payload_length};

        to.num_of_elements = rlp_head.payload_length / length(NewBlock{});  // todo: I think it doesn't works!

        for(int i = 0; i < to.num_of_elements; i++) {
            DecodingResult err = decode(from, to.elements[i]);
            if (err != DecodingResult::kOk) return err;
        }

        return from.length() == leftover ? DecodingResult::kOk : DecodingResult::kListLengthMismatch;
    }

}

}
#endif  // SILKWORM_NEWBLOCKHASHPACKET_HPP
