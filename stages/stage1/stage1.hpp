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
#include <atomic>
#include <chrono>

#include "Types.hpp"
#include "DbTx.hpp"
#include "SentryClient.hpp"
#include "Singleton.hpp"

namespace silkworm {

class Stage {
  public:
    virtual void execution_loop() = 0;

    void need_exit() { exiting_.store(true); }

  protected:
    std::atomic<bool> exiting_{false};
};

class Stage1 : public Stage {
    ChainConfig chain_;
    Hash genesis_hash_;
    std::vector<BlockNum> hard_forks_;
    DbTx db_;
    std::shared_ptr<grpc_impl::Channel> channel_;
    SentryClient sentry_;

  public:

    Stage1(ChainConfig chain, Hash genesis_hash, std::vector<BlockNum> hard_forks, std::string db_path, std::string sentry_addr);

    DbTx& db_tx() {return db_;}

    SentryClient& sentry() {return sentry_;}

    void execution_loop() override;

};

#define STAGE1 Singleton<Stage1>::instance()

}  // namespace silkworm