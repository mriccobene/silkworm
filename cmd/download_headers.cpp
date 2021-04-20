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

#include <CLI/CLI.hpp>

#include <stages/stage1/stage1.hpp>
#include <stages/stage1/HeaderLogic.hpp>

using namespace silkworm;


// Main
int main(int argc, char* argv[]) {
    using std::string, std::cout, std::cerr, std::optional;
    using namespace std::chrono;

    // Command line parsing
    CLI::App app{"Download Headers. Connect to p2p sentry and start header downloading process (stage 1)"};

    string db_path = db::default_path();
    string temporary_file_path = ".";
    string sentry_addr = "127.0.0.1:9091";

    app.add_option("--chaindata", db_path, "Path to the chain database", true)
        ->check(CLI::ExistingDirectory);
    app.add_option("-s,--sentryaddr", sentry_addr, "address:port of sentry", true);
    //  todo ->check?
    app.add_option("-f,--filesdir", temporary_file_path, "Path to a temp files dir", true)
        ->check(CLI::ExistingDirectory);

    CLI11_PARSE(app, argc, argv);

    try {
        // EIP-2124 based chain identity scheme (networkId + genesis + forks)
        ChainIdentity chain_identity{ChainIdentity::mainnet};   // todo: parameterize

        cout << "Download Headers - Silkworm\n"
             << "   chain-id: " << chain_identity.chain.chain_id << "\n"
             << "   genesis-hash: " << chain_identity.genesis_hash << "\n"
             << "   hard-forks: " << chain_identity.hard_forks.size() << "\n\n";

        // Stage1
        Stage1 stage1{chain_identity, db_path, sentry_addr};

        // Node current status
        auto [head_hash, head_td] = HeaderLogic::head_hash_and_total_difficulty(stage1.db_tx());
        cout << "head_hash = " << head_hash.to_hex() << "\n";
        cout << "head_td   = " << intx::to_string(head_td) << "\n";

        // Stage1 main loop
        stage1.execution_loop();    // blocking

        return 0;
    }
    catch(std::exception& e) {
        cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}


