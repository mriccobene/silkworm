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

#include <absl/container/flat_hash_set.h>
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/time/time.h>

#include <boost/filesystem.hpp>
#include <iostream>
#include <silkworm/db/access_layer.hpp>
#include <silkworm/execution/execution.hpp>

ABSL_FLAG(std::string, datadir, silkworm::db::default_path(), "chain DB path");
ABSL_FLAG(uint64_t, from, 1, "start from block number (inclusive)");
ABSL_FLAG(uint64_t, to, UINT64_MAX, "check up to block number (exclusive)");

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Executes Ethereum blocks and compares resulting change sets against DB.");
    absl::ParseCommandLine(argc, argv);

    namespace fs = boost::filesystem;

    fs::path db_path(absl::GetFlag(FLAGS_datadir));
    if (!fs::exists(db_path) || !fs::is_directory(db_path) || db_path.empty()) {
        std::cerr << absl::GetFlag(FLAGS_datadir) << " does not exist.\n";
        std::cerr << "Use --db flag to point to a Turbo-Geth populated chaindata.\n";
        return -1;
    }

    using namespace silkworm;

    lmdb::options db_opts{};
    db_opts.read_only = true;
    std::shared_ptr<lmdb::Environment> env{lmdb::get_env(absl::GetFlag(FLAGS_datadir).c_str(), db_opts)};

    // counters
    uint64_t nTxs{0}, nErrors{0};

    const uint64_t from{absl::GetFlag(FLAGS_from)};
    const uint64_t to{absl::GetFlag(FLAGS_to)};

    uint64_t block_num{from};
    for (; block_num < to; ++block_num) {
        std::unique_ptr<lmdb::Transaction> txn{env->begin_ro_transaction()};

        // Read the block
        std::optional<BlockWithHash> bh{db::read_block(*txn, block_num, /*read_senders=*/true)};
        if (!bh) {
            break;
        }

        db::Buffer buffer{txn.get(), block_num};

        // Execute the block and retreive the receipts
        std::vector<Receipt> receipts = execute_block(bh->block, buffer);

        // There is one receipt per transaction
        assert(bh->block.transactions.size() == receipts.size());

        // TG returns success in the receipt even for pre-Byzantium txs.
        for (auto receipt : receipts) {
            nTxs++;
            nErrors += (!receipt.success);
        }

        // Report and reset counters
        if (!(block_num % 50000)) {
            std::cout << block_num << "," << nTxs << "," << nErrors << std::endl;
            nTxs = nErrors = 0;

        } else if (!(block_num % 100)) {
            // report progress
            std::cerr << block_num << "\r";
            std::cerr.flush();
        }
    }

    return 0;
}
