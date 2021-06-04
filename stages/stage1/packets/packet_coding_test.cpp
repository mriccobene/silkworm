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

#include <catch2/catch.hpp>
//#include "NewBlockHashesPacket.hpp"
//#include "GetBlockHeadersPacket.hpp"
#include "Packets.hpp"

namespace silkworm {

// TESTs related to NewBlockHashesPacket encoding/decoding
// ----------------------------------------------------------------------------

/*
input:  e6e5a0eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc35483bd8410
decoded:
        e6 = 26 bytes -> list
         |--  e5 = 25 bytes -> list
             |-- a0 = 20 bytes -> string
                  |-- eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc354
             |-- 83 = 3 bytes -> string
                  |-- bd8410 (= decimal 12.420.112)
 */
TEST_CASE("NewBlockHashesPacket decoding") {
    using namespace std;

    optional<Bytes> encoded = from_hex("e6e5a0eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc35483bd8410");
    REQUIRE(encoded.has_value());

    NewBlockHashesPacket packet;

    ByteView encoded_view = encoded.value();
    rlp::DecodingResult result = rlp::decode(encoded_view, packet);

    REQUIRE(result == rlp::DecodingResult::kOk);
    REQUIRE(packet.size() == 1);
    REQUIRE(packet[0].hash == Hash::from_hex("eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc354"));
    REQUIRE(packet[0].number == 12'420'112);
}

TEST_CASE("NewBlockHashesPacket encoding") {
    using namespace std;

    NewBlockHashesPacket packet;

    NewBlock newBlock;
    newBlock.hash = Hash::from_hex("eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc354");
    newBlock.number = 12'420'112;
    packet.push_back(newBlock);

    Bytes encoded;
    rlp::encode(encoded, packet);

    REQUIRE(to_hex(encoded) == "e6e5a0eb2c33963824bf97d01cff8a65f00dc402fbf64f473cb4778a547ac08cebc35483bd8410");

    // todo: activate this test

    //auto len = rlp::length(packet);

    //REQUIRE(len == encoded.size());
}

// TESTs related to GetBlockHeadersPacket encoding/decoding - eth/65 version
// ----------------------------------------------------------------------------
/*
input:  c783b9ffff018080 (check!)
decoded:
         C7 = list, 7 bytes
            |-- 83b9ffff018080
                 |-- 83 = string, 3 bytes
                      |-- b9ffff
                 |-- 01 = string, value = 01
                 |-- 80 = string, 0 bytes
                 |-- 80 = string, 0 bytes
 */
TEST_CASE("GetBlockHeadersPacket (eth/65) decoding") {
    using namespace std;

    optional<Bytes> encoded = from_hex("c783b9ffff018080");
    REQUIRE(encoded.has_value());

    GetBlockHeadersPacket packet;

    ByteView encoded_view = encoded.value();
    rlp::DecodingResult result = rlp::decode(encoded_view, packet);

    REQUIRE(result == rlp::DecodingResult::kOk);
    REQUIRE(std::holds_alternative<BlockNum>(packet.origin) == true);
    REQUIRE(std::get<BlockNum>(packet.origin) == 12189695); //intx::from_string("0xb9ffff"));
    REQUIRE(packet.amount == 1);
    REQUIRE(packet.skip == 0);
    REQUIRE(packet.reverse == false);
}

TEST_CASE("GetBlockHeadersPacket (eth/65) encoding") {
    using namespace std;

    GetBlockHeadersPacket packet;

    packet.origin = BlockNum{12189695};
    packet.amount = 1;
    packet.skip = 0;
    packet.reverse = false;

    Bytes encoded;
    rlp::encode(encoded, packet);

    REQUIRE(to_hex(encoded) == "c783b9ffff018080");

    auto len = rlp::length(packet);

    REQUIRE(len == encoded.size());
}

// TESTs related to GetBlockHeadersPacket66 encoding/decoding - eth/66 version
// ----------------------------------------------------------------------------
/*
input:  d1886b1a456ba6e2f81dc783b9ffff018080
decoded:
        d1 = list, 17 bytes
         |-- 886b1a456ba6e2f81dc783b9ffff018080
               |-- 88 = string, 8 bytes
                    |-- 6b1a456ba6e2f81d
               |-- C7 = list, 7 bytes
                    |-- 83b9ffff018080
                          |-- 83 = string, 3 bytes
                               |-- b9ffff
                          |-- 01 = string, value = 01
                          |-- 80 = string, 0 bytes
                          |-- 80 = string, 0 bytes
 */
TEST_CASE("GetBlockHeadersPacket (eth/66) decoding") {
    using namespace std;

    optional<Bytes> encoded = from_hex("d1886b1a456ba6e2f81dc783b9ffff018080");
    REQUIRE(encoded.has_value());

    GetBlockHeadersPacket66 packet;

    ByteView encoded_view = encoded.value();
    rlp::DecodingResult result = rlp::decode(encoded_view, packet);

    REQUIRE(result == rlp::DecodingResult::kOk);
    REQUIRE(packet.requestId == 0x6b1a456ba6e2f81d);
    REQUIRE(std::holds_alternative<BlockNum>(packet.request.origin) == true);
    REQUIRE(std::get<BlockNum>(packet.request.origin) == 12189695); //intx::from_string("0xb9ffff"));
    REQUIRE(packet.request.amount == 1);
    REQUIRE(packet.request.skip == 0);
    REQUIRE(packet.request.reverse == false);
}

TEST_CASE("GetBlockHeadersPacket (eth/66) encoding") {
    using namespace std;

    GetBlockHeadersPacket66 packet;

    packet.requestId = 0x6b1a456ba6e2f81d;
    packet.request.origin = BlockNum{12189695};
    packet.request.amount = 1;
    packet.request.skip = 0;
    packet.request.reverse = false;

    Bytes encoded;
    rlp::encode(encoded, packet);

    REQUIRE(to_hex(encoded) == "d1886b1a456ba6e2f81dc783b9ffff018080");

    auto len = rlp::length(packet);

    REQUIRE(len == encoded.size());
}

}