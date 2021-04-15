//
// Created by miche on 13/04/2021.
//

#ifndef SILKWORM_HEADER_HPP
#define SILKWORM_HEADER_HPP

#include <vector>
#include "Types.hpp"

namespace silkworm {

class Header {
  public:
    static std::vector<Header> recoverByHash(Hash origin, uint64_t amount, uint64_t skip, bool reverse);
    static std::vector<Header> recoverByNumber(BlockNum origin, uint64_t amount, uint64_t skip, bool reverse);
};

}

#endif  // SILKWORM_HEADER_HPP
