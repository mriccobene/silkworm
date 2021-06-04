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

#ifndef SILKWORM_RLPVECTOR_HPP
#define SILKWORM_RLPVECTOR_HPP

#include "stages/stage1/Types.hpp"

/*
 * decode a generic vector
 *
 * This is a forward declaration. Please note that the implementation need to know how to decode concrete T elements,
 * so headers organization is critical. For this reason it is hard to use the decode_vector() func defined in core
 * module because it is in a header with other decode(T) functions and it is not possible to insert other decode(T) in
 * the middle. So we use this trick.
 */
namespace silkworm::rlp {
    template <class T>
    void encode_vec(Bytes& to, const std::vector<T>& v);

    template <class T>
    size_t length_vec(const std::vector<T>& v);

    template <class T>
    DecodingResult decode_vec(ByteView& from, std::vector<T>& to);
}

#endif  // SILKWORM_RLPVECTOR_HPP
