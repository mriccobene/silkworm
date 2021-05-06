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
#include "ChainIdentity.hpp"

namespace silkworm {

static ChainIdentity mainnet_identity() {
    ChainIdentity id;

    id.name = "mainnet";

    id.chain = kMainnetConfig;
    id.genesis_hash = Hash::from_hex("d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3"); // mainnet genesis hash in hex

    id.hard_forks.push_back(*id.chain.homestead_block);
    id.hard_forks.push_back(*id.chain.dao_block);
    id.hard_forks.push_back(*id.chain.tangerine_whistle_block);
    id.hard_forks.push_back(*id.chain.spurious_dragon_block);
    id.hard_forks.push_back(*id.chain.byzantium_block);
    id.hard_forks.push_back(*id.chain.constantinople_block);
    //id.hard_forks.push_back(*chain.petersburg_block);     // todo: uses all the forks but erase block numbers with the same value or zero
    id.hard_forks.push_back(*id.chain.istanbul_block);
    id.hard_forks.push_back(*id.chain.muir_glacier_block);
    id.hard_forks.push_back(*id.chain.berlin_block);

    return id;
}

static ChainIdentity goerli_identity() {
    ChainIdentity id;

    id.name = "goerli";

    id.chain = kGoerliConfig;
    id.genesis_hash = Hash::from_hex("bf7e331f7f7c1dd2e05159666b3bf8bc7a8a3a9eb1d518969eab529dd9b88c1a"); // goerli genesis hash in hex

    id.hard_forks.push_back(*id.chain.istanbul_block); // todo: uses all the forks but erase block numbers with the same value or zero
    id.hard_forks.push_back(*id.chain.berlin_block);

    return id;
}

ChainIdentity ChainIdentity::mainnet = mainnet_identity();
ChainIdentity ChainIdentity::goerli = goerli_identity();

}