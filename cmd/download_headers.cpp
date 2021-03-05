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
#include <grpcpp/grpcpp.h>

#include <silkworm/db/access_layer.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/db/tables.hpp>
#include <silkworm/types/transaction.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/chain/config.hpp>

#include <interfaces/sentry.grpc.pb.h>

using namespace silkworm;

class SentryClient {
  public:
    SentryClient(std::shared_ptr<grpc::Channel> channel): stub_(sentry::Sentry::NewStub(channel))
    {

    }

    bool setStatus()
    {
        grpc::ClientContext context;
        sentry::StatusData request;
        google::protobuf::Empty response;

        // todo: fill request
        /*
        message StatusData {
            uint64 network_id = 1;
            bytes total_difficulty = 2;
            bytes best_hash = 3;
            Forks fork_data = 4;
        }
        */
        request.set_network_id(1);
        //request.set_...

        grpc::Status invocation = stub_->SetStatus(&context, request, &response);
        if (!invocation.ok()) {
            std::cout << "SetStatus rpc failed: " << invocation.error_code() << ", " << invocation.error_message() << std::endl;
            return false;
        }

        return true;
    }
  private:
    std::unique_ptr<sentry::Sentry::Stub> stub_;
};

// Main
int main(int argc, char* argv[]) {
    using std::string, std::cout, std::cerr, std::optional;
    using namespace std::chrono;

    // Command line parsing
    CLI::App app{"Download Headers. Connect to p2p sentry and start header downloading process (stage 1)"};

    string db_path = db::default_path();
    string temporary_file_path = ".";
    string sentry_addr = "localhost:9999";

    app.add_option("-d,--datadir", db_path, "Path to the chain database", true)
        ->check(CLI::ExistingDirectory);
    app.add_option("-s,--sentryaddr", sentry_addr, "address:port of sentry", true);
    //  todo ->check?
    app.add_option("-f,--filesdir", temporary_file_path, "Path to a temp files dir", true)
        ->check(CLI::ExistingDirectory);

    CLI11_PARSE(app, argc, argv);

    // Main loop
    try {
        auto channel = grpc::CreateChannel(sentry_addr, grpc::InsecureChannelCredentials());
        SentryClient client(channel);
        bool ok = client.setStatus();
        return ok;
    }
    catch(std::exception& e) {
        cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}


