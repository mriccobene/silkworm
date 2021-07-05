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

// Auxiliary types ----------------------------------------------------------------------------------------------------
using Header_Ref = std::vector<BlockHeader>::const_iterator; // todo: check what is better among const_iterator or shared_ptr or hash

inline std::vector<Header_Ref> to_ref(const std::vector<BlockHeader>& headers) {
    std::vector<Header_Ref> refs;
    for(Header_Ref i = headers.begin(); i < headers.end(); i++)
        refs.push_back(i);
    return refs;
}

// Segment, a sequence of headers connected to one another (with parent-hash relationship),
// without any branching, ordered from high block number to lower block number
struct Segment {
    std::vector<Header_Ref> headers;
    //std::vector<something> headersRaw;    // todo: do we need this?
    //shared_ptr<Bundle> storage; // todo: do we need this to bundle Header_Ref referred objects?
};

// ---------------
namespace sperimental {

using Bundle_Ref = std::vector<BlockHeader>::const_iterator;

struct Bundle {
    std::vector<BlockHeader> headers;
};

using Segment_Ref = std::vector<BlockHeader>::const_iterator;

struct Segment {

    static auto split_into_segment(const Bundle& headers) -> std::tuple<std::vector<Segment>, Penalty>;

    Segment_Ref lowest_header() {return *headers.rbegin();}

    auto attach(Anchor_Map, Link_Map) -> std::tuple<Segment_Ref,Segment_Ref>;

    auto find_anchor_in(Anchor_Map) -> Segment_Ref;
    auto find_link_in(Link_Map, Segment_Ref anchor) -> Segment_Ref;

    std::vector<Bundle_Ref> headers;

  private:
    Segment();

    //std::vector<something> headersRaw;    // todo: do we need this?
    Bundle* bundle;
};

}

// WorkingChain -------------------------------------------------------------------------------------------------------

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
    std::tuple<Penalty,RequestMoreHeaders> accept_headers(const std::vector<BlockHeader>&, PeerId);

    void save_external_announce(Hash hash);
    bool has_link(Hash hash);

  private:
    static constexpr BlockNum max_len = 192;
    static constexpr BlockNum stride = 8 * max_len;
    static constexpr size_t anchor_limit = 512;
    static constexpr size_t link_total = 1024*1024;
    static constexpr size_t persistent_link_limit = link_total / 16;
    static constexpr size_t link_limit = link_total - persistent_link_limit;

    std::optional<GetBlockHeadersPacket66> request_skeleton(); // anchor collection
    std::optional<GetBlockHeadersPacket66> request_more_headers(); // anchor extension

    using IsANewBlock = bool;
    auto split_into_segments(const std::vector<BlockHeader>&) -> std::tuple<std::vector<Segment>, Penalty>;
    auto process_segment(Segment, IsANewBlock, PeerId) -> RequestMoreHeaders;

    using Found = bool; using Start = int; using End = int;
    auto find_anchor(Segment)                                 -> std::tuple<Found, Start>;
    auto find_link(Segment segment, int start)                -> std::tuple<Found, End>;
    auto get_link(Hash hash)                                  -> std::optional<std::shared_ptr<Link>>;
    void reduce_links();

    using Error = int;
    void connect(Segment, Start, End);                          // throw SegmentCutAndPasteException
    auto extendDown(Segment, Start, End) -> RequestMoreHeaders; // throw SegmentCutAndPasteException
    void extendUp(Segment, Start, End);                         // throw SegmentCutAndPasteException
    auto newAnchor(Segment, Start, End, PeerId) -> RequestMoreHeaders; // throw SegmentCutAndPasteException

    Oldest_First_Link_Queue persistedLinkQueue_;
    Youngest_First_Link_Queue linkQueue_;
    Oldest_First_Anchor_Queue anchorQueue_;
    Link_Map links_;
    Anchor_Map anchors_;
    BlockNum highestInDb_;
    BlockNum topSeenHeight_;
    std::set<Hash> badHeaders_;
};

}

#endif  // SILKWORM_WORKINGCHAIN_HPP
