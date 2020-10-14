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

#include "access_layer.hpp"

#include <catch2/catch.hpp>
#include <silkworm/common/temp_dir.hpp>

#include "tables.hpp"

namespace silkworm::db {

TEST_CASE("LMDB read header") {
    TemporaryDirectory tmp_dir{};

    lmdb::options opts{};
    opts.map_size = 32 << 10;  //  32KiB
    opts.read_only = false;
    std::shared_ptr<lmdb::Environment> env{lmdb::get_env(tmp_dir.path(), opts)};
    std::unique_ptr<lmdb::Transaction> txn{env->begin_rw_transaction()};
    table::create_all(*txn);

    BlockHeader header{};
    header.number = 11'054'435;
    header.beneficiary = 0x09ab1303d3ccaf5f018cd511146b07a240c70294_address;
    header.gas_limit = 12'451'080;
    header.gas_used = 12'443'619;

    Bytes rlp{};
    rlp::encode(rlp, header);
    ethash::hash256 hash{keccak256(rlp)};

    CHECK(!read_header(*txn, header.number, hash.bytes));

    auto table{txn->open(table::kBlockHeaders)};
    Bytes key{block_key(header.number, hash.bytes)};
    table->put(key, rlp);

    std::optional<BlockHeader> header_from_db{read_header(*txn, header.number, hash.bytes)};
    REQUIRE(header_from_db);
    CHECK(*header_from_db == header);
}

TEST_CASE("MDBX read header") {
    TemporaryDirectory tmp_dir{};

    mdbx::env_managed::create_parameters create{};
    create.geometry.make_dynamic(0, 32 * mdbx::env::geometry::KiB);
    mdbx::env_managed env{tmp_dir.path(), create, kDefaultMdbxParameters};
    mdbx::txn_managed txn{env.start_write()};

    table::create_all(txn);

    BlockHeader header{};
    header.number = 11'054'435;
    header.beneficiary = 0x09ab1303d3ccaf5f018cd511146b07a240c70294_address;
    header.gas_limit = 12'451'080;
    header.gas_used = 12'443'619;

    Bytes rlp{};
    rlp::encode(rlp, header);
    ethash::hash256 hash{keccak256(rlp)};

    CHECK(!read_header(txn, header.number, hash.bytes));

    auto table{table::open(txn, table::kBlockHeaders)};
    Bytes key{block_key(header.number, hash.bytes)};
    txn.put(table, mdbx::slice{key}, mdbx::slice{rlp}, mdbx::put_mode::upsert);

    std::optional<BlockHeader> header_from_db{read_header(txn, header.number, hash.bytes)};
    REQUIRE(header_from_db);
    CHECK(*header_from_db == header);
}

}  // namespace silkworm::db
