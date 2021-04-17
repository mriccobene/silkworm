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
#ifndef SILKWORM_SENTRYCLIENT_HPP
#define SILKWORM_SENTRYCLIENT_HPP

#include <iostream>
#include <chrono>
#include <string>
#include <vector>

#include <silkworm/db/access_layer.hpp>
#include <silkworm/db/stages.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/db/tables.hpp>
#include <silkworm/chain/config.hpp>
#include <interfaces/sentry.grpc.pb.h>
#include "gRPCAsyncClient.hpp"
#include "Types.hpp"

namespace silkworm {

class SentryClient: public rpc::AsyncClient<sentry::Sentry> {
  public:
    using client_base_t = rpc::AsyncClient<sentry::Sentry>;

    SentryClient(std::shared_ptr<grpc::Channel> channel):
        client_base_t(channel)
    {}

    void exec_remotely(std::shared_ptr<call_t> call) {
        client_base_t::exec_remotely(call.get()); // todo: UNSAFE! avoid it passing shared_ptr and in AsyncClient retain shared_ptr to control lifetime of call
    }

};

}

#endif  // SILKWORM_SENTRYCLIENT_HPP
