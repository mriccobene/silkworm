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
#include <iostream>
#include <chrono>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <grpcpp/grpcpp.h>
#include <boost/endian/conversion.hpp>

#include <silkworm/db/access_layer.hpp>
#include <silkworm/db/stages.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/db/tables.hpp>
#include <silkworm/types/transaction.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/chain/config.hpp>

#include <interfaces/sentry.grpc.pb.h>

using namespace silkworm;

// Definitions --------------------------------------------------------------------------------------------------------
class Hash: public evmc::bytes32 {
  public:
    using evmc::bytes32::bytes32;

    Hash() {}
    Hash(ByteView bv) {std::memcpy(bytes, bv.data(), 32);}

    operator Bytes() {return {bytes, 32};}
    operator ByteView() {return {bytes, 32};}

    uint8_t* raw_bytes() {return bytes;}
    int length() {return 32;}

    std::string to_hex() { return silkworm::to_hex(*this); }
    static Hash from_hex(std::string hex) { return Hash(evmc::literals::internal::from_hex<bytes32>(hex.c_str())); }

    static_assert(sizeof(evmc::bytes32) == 32);
};

using Header = BlockHeader;
using BlockNum = uint64_t;
using BigInt = intx::uint256; // use intx::to_string, from_string, ...
//using Bytes = std::basic_string<uint8_t>; already defined elsewhere

// LMDB access --------------------------------------------------------------------------------------------------------
class Db {
    std::shared_ptr<lmdb::Environment> env;
    std::unique_ptr<lmdb::Transaction> txn;

  public:
    explicit Db(std::string db_path) {
        lmdb::DatabaseConfig db_config{db_path};
        //db_config.set_readonly(false);
        env = lmdb::get_env(db_config);
        txn = env->begin_ro_transaction();
    }

    std::optional<Hash> read_canonical_hash(BlockNum b) {  // throws db exceptions // todo: add to db::access_layer.hpp?
        auto header_table = txn->open(db::table::kBlockHeaders);
        // accessing this table with only b we will get the hash of the canonical block at height b
        std::optional<ByteView> hash = header_table->get(db::header_hash_key(b));
        if (!hash) return std::nullopt; // not found
        assert(hash->size() == kHashLength);
        return hash.value(); // copy
    }

    static Bytes head_header_key() { // todo: add to db::util.h?
        std::string table_name = db::table::kHeadHeader.name;
        Bytes key{table_name.begin(), table_name.end()};
        return key;
    }

    std::optional<Hash> read_head_header_hash() { // todo: add to db::access_layer.hpp?
        auto head_header_table = txn->open(db::table::kHeadHeader);
        std::optional<ByteView> hash = head_header_table->get(head_header_key());
        if (!hash) return std::nullopt; // not found
        assert(hash->size() == kHashLength);
        return hash.value(); // copy
    }

    std::optional<BlockHeader> read_header(BlockNum b, Hash h)  {
        return db::read_header(*txn, b, h.bytes);
    }

    std::optional<ByteView> read_rlp_encoded_header(BlockNum b, Hash h)  {
        auto header_table = txn->open(db::table::kBlockHeaders);
        std::optional<ByteView> rlp = header_table->get(db::block_key(b, h.bytes));
        return rlp;
    }

    static Bytes header_numbers_key(Hash h) { // todo: add to db::util.h?
        return {h.bytes, 32};
    }

    std::optional<BlockHeader> read_header(Hash h) { // todo: add to db::access_layer.hpp?
        auto blockhashes_table = txn->open(db::table::kHeaderNumbers);
        auto encoded_block_num = blockhashes_table->get(header_numbers_key(h));
        if (!encoded_block_num) return {};
        BlockNum block_num = boost::endian::load_big_u64(encoded_block_num->data());
        return read_header(block_num, h);
    }

    std::optional<intx::uint256> read_total_difficulty(BlockNum b, Hash h) {
        return db::read_total_difficulty(*txn, b, h.bytes);
    }

    BlockNum stage_progress(const char* stage_name) {
        return db::stages::get_stage_progress(*txn, stage_name);
    }

};

// SentryClient -------------------------------------------------------------------------------------------------------
class SentryClient {
  public:
    SentryClient(ChainConfig chain, Hash genesis, std::vector<BlockNum> hard_forks, std::shared_ptr<grpc::Channel> channel):
        chain_{chain}, genesis_(genesis), hard_forks_(std::move(hard_forks)), stub_{sentry::Sentry::NewStub(channel)}
    {
    }

    /* Sends a
        message StatusData {
            uint64 network_id = 1;
            bytes total_difficulty = 2;
            bytes best_hash = 3;
            Forks fork_data = 4;
        }
    */
    bool setStatus(Hash best_hash, BigInt total_difficulty)
    {
        grpc::ClientContext context;
        sentry::StatusData request;
        google::protobuf::Empty response;

        request.set_network_id(chain_.chain_id);

        ByteView td = rlp::big_endian(total_difficulty);    // remove trailing zeros - WARNING it uses thread_local var
        request.set_total_difficulty(td.data(), td.length());

        request.set_best_hash(best_hash.raw_bytes(), best_hash.length());

        auto* forks = new sentry::Forks{};
        forks->set_genesis(genesis_.raw_bytes(), genesis_.length());
        for(uint64_t block: hard_forks_)
            forks->add_forks(block);
        request.set_allocated_fork_data(forks); // take ownership

        grpc::Status invocation = stub_->SetStatus(&context, request, &response);
        if (!invocation.ok()) {
            std::cout << "SetStatus rpc failed: " << invocation.error_code() << ", " << invocation.error_message() << std::endl;
            return false;
        }

        return true;
    }
  private:
    // chain identification
    ChainConfig chain_;
    Hash genesis_;
    std::vector<BlockNum> hard_forks_;

    // communication stuff
    std::unique_ptr<sentry::Sentry::Stub> stub_;
};

// Main
int main(int argc, char* argv[]) {
    using std::string, std::cout, std::cerr, std::optional, std::vector;
    using namespace std::chrono;

    // Command line parsing
    CLI::App app{"Download Headers. Connect to p2p sentry and start header downloading process (stage 1)"};

    string db_path = db::default_path();
    string temporary_file_path = ".";
    string sentry_addr = "127.0.0.1:9091";

    app.add_option("-d,--datadir", db_path, "Path to the chain database", true)
        ->check(CLI::ExistingDirectory);
    app.add_option("-s,--sentryaddr", sentry_addr, "address:port of sentry", true);
    //  todo ->check?
    app.add_option("-f,--filesdir", temporary_file_path, "Path to a temp files dir", true)
        ->check(CLI::ExistingDirectory);

    CLI11_PARSE(app, argc, argv);

    // Main loop
    try {
        Db db{db_path};

        // EIP-2124 based chain identity scheme (networkId + genesis + forks)
        ChainConfig chain = kMainnetConfig;  // todo: parameterize
        Hash genesis_hash = Hash::from_hex("d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3"); // mainnet genesis hash in hex // todo: parameterize
        vector<BlockNum> forks; // todo: improve, but how???
        forks.push_back(*chain.homestead_block);
        forks.push_back(*chain.dao_block);
        forks.push_back(*chain.tangerine_whistle_block);
        forks.push_back(*chain.spurious_dragon_block);
        forks.push_back(*chain.byzantium_block);
        forks.push_back(*chain.constantinople_block);
        //forks.push_back(*chain.petersburg_block);     // todo: uses all the forks but erase block numbers with the same value
        forks.push_back(*chain.istanbul_block);
        forks.push_back(*chain.muir_glacier_block);
        //forks.push_back(*chain.berlin_block); // next april 2021

        // Node current status
        BlockNum head_height = db.stage_progress(db::stages::kBlockBodiesKey);
        auto head_hash = db.read_canonical_hash(head_height);
        if (!head_hash) throw std::logic_error("canonical hash at height " + std::to_string(head_height) + " not found in db");
        std::optional<BigInt> head_td = db.read_total_difficulty(head_height, *head_hash);
        if (!head_td) throw std::logic_error("total difficulty of canonical hash at height " + std::to_string(head_height) + " not found in db");

        cerr << "head_hash = " << head_hash->to_hex() << "\n";
        cerr << "head_td   = " << intx::to_string(*head_td) << "\n";

        // Connect to p2p sentry
        auto channel = grpc::CreateChannel(sentry_addr, grpc::InsecureChannelCredentials());
        //auto ch_stat = channel->GetState(true);
        //cout << ch_stat << "\n";

        SentryClient client(chain, genesis_hash, forks, channel);

        // Send Status to p2p sentry
        bool ok = client.setStatus(*head_hash, *head_td);

        return ok;
    }
    catch(std::exception& e) {
        cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}


