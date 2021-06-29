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

#include <silkworm/common/log.hpp>

#include "RandomNumber.hpp"
#include "WorkingChain.hpp"

namespace silkworm {

WorkingChain::WorkingChain(BlockNum highestInDb, BlockNum topSeenHeight): highestInDb_(highestInDb), topSeenHeight_(topSeenHeight) {
}

void WorkingChain::highest_block_in_db(BlockNum n) {
    highestInDb_ = n;
}
BlockNum WorkingChain::highest_block_in_db() {
    return highestInDb_;
}
void WorkingChain::top_seen_block_height(BlockNum n) {
    topSeenHeight_ = n;
}
BlockNum WorkingChain::top_seen_block_height() {
    return topSeenHeight_;
}

std::optional<GetBlockHeadersPacket66> WorkingChain::headers_forward() {
    // todo: implements!
    // ...
    // only for test:
    return request_skeleton();
}

// Request skeleton - Request "seed" headers
// It requests N headers starting at highestInDb with step = stride up to topSeenHeight

std::optional<GetBlockHeadersPacket66> WorkingChain::request_skeleton() {
    if (anchors_.size() > 16) return std::nullopt;

    if (topSeenHeight_ < highestInDb_ + stride) return std::nullopt;

    BlockNum length = (topSeenHeight_ - highestInDb_) / stride;
    if (length > max_len)
        length = max_len;

    GetBlockHeadersPacket66 packet;
    packet.requestId = RANDOM_NUMBER.generate_one();
    packet.request.origin = highestInDb_ + stride;
    packet.request.amount = length;
    packet.request.skip = stride;
    packet.request.reverse = false;

    return {packet};
}

void WorkingChain::save_external_announce(Hash) {
    // Erigon implementation:
    // hd.seenAnnounces.Add(hash)
    // todo: implement!
    SILKWORM_LOG(LogLevel::Warn) << "SelfExtendingChain::save_external_announce() not implemented yet\n";
}

bool WorkingChain::has_link(Hash hash) {
    return (links_.find(hash) != links_.end());
}

/* from Erigon
if segments, penalty, err := cs.Hd.SplitIntoSegments(headersRaw, headers); err == nil {
    if penalty == headerdownload.NoPenalty {
        var canRequestMore bool
        for _, segment := range segments {
            newBlock = false;
            requestMore := cs.Hd.ProcessSegment(segment, newBlock, string(gointerfaces.ConvertH512ToBytes(in.PeerId)))
            canRequestMore = canRequestMore || requestMore
        }

        if canRequestMore {
                currentTime := uint64(time.Now().Unix())
                req, penalties := cs.Hd.RequestMoreHeaders(currentTime)
                if req != nil {
                    if peer := cs.SendHeaderRequest(ctx, req); peer != nil {
                        timeOut = 5;
                        cs.Hd.SentRequest(req, currentTime, timeOut)
                        log.Debug("Sent request", "height", req.Number)
                    }
                }
                cs.Penalize(ctx, penalties)
            }
    } else {
        outreq := proto_sentry.PenalizePeerRequest{
            PeerId:  in.PeerId,
            Penalty: proto_sentry.PenaltyKind_Kick,
        }
        if _, err1 := sentry.PenalizePeer(ctx, &outreq, &grpc.EmptyCallOption{}); err1 != nil {
            log.Error("Could not send penalty", "err", err1)
        }
    }
} else {
    return fmt.Errorf("singleHeaderAsSegment failed: %v", err)
}

*/
auto WorkingChain::accept_headers(const std::vector<BlockHeader>& headers) -> std::tuple<Penalty,RequestMoreHeaders> {
    bool requestMoreHeaders = false;

    auto [segments, penalty] = split_into_segments(headers); // todo: Erigon here pass also headerRaw

    if (penalty != Penalty::NoPenalty)
        return {penalty, requestMoreHeaders};

    for(auto& segment: segments) {
        requestMoreHeaders |= process_segment(segment);
    }

    return {Penalty::NoPenalty, requestMoreHeaders};
}

auto WorkingChain::split_into_segments(const std::vector<BlockHeader>&) -> std::tuple<std::vector<Segment>, Penalty> {
    std::vector<Segment> segments;

    // todo: implement!

    return {segments, Penalty::NoPenalty};
}

auto WorkingChain::process_segment(Segment) -> RequestMoreHeaders {
    // todo: implement!

    return false;
}

}