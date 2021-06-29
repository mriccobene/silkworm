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

#ifndef SILKWORM_WORKINGCHAIN_HPP
#define SILKWORM_WORKINGCHAIN_HPP

#include "HeaderLogic.hpp"
#include "stages/stage1/packets/GetBlockHeadersPacket.hpp"

namespace silkworm {

class WorkingChain {  // tentative name - todo: improve!
  public:
    WorkingChain(BlockNum highestInDb, BlockNum topSeenHeight);

    void highest_block_in_db(BlockNum n);
    BlockNum highest_block_in_db();
    void top_seen_block_height(BlockNum n);
    BlockNum top_seen_block_height();

    std::optional<GetBlockHeadersPacket66> headers_forward(); // progresses Headers stage in the forward direction
    void request_ack(GetBlockHeadersPacket66 packet, time_point_t tp, time_dur_t timeout);

    using RequestMoreHeaders = bool;
    std::tuple<Penalty,RequestMoreHeaders> accept_headers(const std::vector<BlockHeader>&);

    void save_external_announce(Hash hash);
    bool has_link(Hash hash);

  private:
    static constexpr BlockNum max_len = 192;
    static constexpr BlockNum stride = 8 * max_len;

    std::optional<GetBlockHeadersPacket66> request_more_headers();
    std::optional<GetBlockHeadersPacket66> request_skeleton();

    std::tuple<std::vector<Segment>, Penalty> split_into_segments(const std::vector<BlockHeader>&);
    RequestMoreHeaders process_segment(Segment);

    Oldest_First_Link_Queue persistedLinkQueue_;
    Youngest_First_Link_Queue linkQueue_;
    Oldest_First_Anchor_Queue anchorQueue_;
    Link_Map links_;
    Anchor_Map anchors_;
    BlockNum highestInDb_;
    BlockNum topSeenHeight_;

};

}

#endif  // SILKWORM_WORKINGCHAIN_HPP
