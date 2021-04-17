//
// Created by miche on 16/04/2021.
//

#ifndef SILKWORM_BLOCKREQUESTLOGIC_HPP
#define SILKWORM_BLOCKREQUESTLOGIC_HPP

#include <memory>
#include "OutboundMessage.hpp"

namespace silkworm {

class BlockRequestLogic {  // todo: modularize this!
  public:
    static std::shared_ptr<OutboundMessage> execute() {
        return nullptr;  // todo: implements!
    }
};

}
#endif  // SILKWORM_BLOCKREQUESTLOGIC_HPP
