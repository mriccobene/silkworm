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

#include "account.hpp"

#include "../rlp/decode.hpp"
#include "../rlp/encode.hpp"

namespace silkworm {

namespace eth {

bool operator==(const Account& a, const Account& b) {
  return a.nonce == b.nonce && a.balance == b.balance && a.storage_root == b.storage_root &&
         a.code_hash == b.code_hash && a.incarnation == b.incarnation;
}

}  // namespace eth

namespace rlp {
void encode(std::ostream& to, const eth::Account& account) {
  Header h{.list = true, .length = 0};
  h.length += length(account.nonce);
  h.length += length(account.balance);
  h.length += eth::kHashLength + 1;
  h.length += eth::kHashLength + 1;

  encode(to, h);
  encode(to, account.nonce);
  encode(to, account.balance);
  encode(to, account.storage_root.bytes);
  encode(to, account.code_hash.bytes);
}

eth::Account decode_account(std::istream& from) {
  Header h = decode_header(from);
  if (!h.list) {
    throw DecodingError("unexpected string");
  }

  eth::Account account;
  account.nonce = decode_uint64(from);
  account.balance = decode_uint256(from);
  decode_bytes(from, account.storage_root.bytes);
  decode_bytes(from, account.code_hash.bytes);

  return account;
}
}  // namespace rlp
}  // namespace silkworm