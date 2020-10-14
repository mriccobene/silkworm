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

#ifndef SILKWORM_DB_TABLES_H_
#define SILKWORM_DB_TABLES_H_

#include <libmdbx/mdbx.h++>
#include <silkworm/db/chaindb.hpp>

/*
Part of the compatibility layer with the Turbo-Geth DB format;
see its common/dbutils/bucket.go.
*/
namespace silkworm::db::table {

constexpr Config kPlainState{"PLAIN-CST2", /*multi_val=*/true};
constexpr Config kAccountChanges{"PLAIN-ACS"};
constexpr Config kStorageChanges{"PLAIN-SCS"};
constexpr Config kAccountHistory{"hAT"};
constexpr Config kStorageHistory{"hST"};
constexpr Config kCode{"CODE"};
constexpr Config kCodeHash{"PLAIN-contractCode"};
constexpr Config kBlockHeaders{"h"};
constexpr Config kBlockBodies{"b"};
constexpr Config kSenders{"txSenders"};
constexpr Config kIncarnations{"incarnationMap"};
constexpr Config kSyncStageProgress{"SSP2"};

constexpr Config kTables[]{
    kPlainState, kAccountChanges, kStorageChanges, kAccountHistory, kStorageHistory, kCode,
    kCodeHash,   kBlockHeaders,   kBlockBodies,    kSenders,        kIncarnations,   kSyncStageProgress,
};

// Create all tables that do not yet exist.
void create_all(lmdb::Transaction& txn);
void create_all(mdbx::txn& txn);

inline mdbx::map_handle open(const mdbx::txn& txn, const Config& config) {
    auto key_mode{mdbx::key_mode::usual};
    auto value_mode{config.multi_val ? mdbx::value_mode::multi : mdbx::value_mode::single};
    return txn.open_map(config.name, key_mode, value_mode);
}

}  // namespace silkworm::db::table

#endif  // SILKWORM_DB_TABLES_H_
