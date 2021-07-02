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

#include "InboundBlockHeaders.hpp"
#include "stages/stage1/rpc/PeerMinBlock.hpp"
#include "stages/stage1/rpc/PenalizePeer.hpp"
#include "stages/stage1/HeaderLogic.hpp"
#include "stages/stage1/packets/RLPError.hpp"
#include "stages/stage1/stage1.hpp"

namespace silkworm {


InboundBlockHeaders::InboundBlockHeaders(const sentry::InboundMessage& msg): InboundMessage() {
    if (msg.id() != sentry::MessageId::BLOCK_HEADERS_66)
        throw std::logic_error("InboundBlockHeaders received wrong InboundMessage");

    peerId_ = string_from_H512(msg.peer_id());

    ByteView data = byte_view_of_string(msg.data()); // copy for consumption
    rlp::DecodingResult err = rlp::decode(data, packet_);
    if (err != rlp::DecodingResult::kOk)
        throw rlp::rlp_error("rlp decoding error decoding BlockHeaders");
}

/* blockHeaders66 processing from Erigon
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
 outreq := proto_sentry.PeerMinBlockRequest{
     PeerId:   in.PeerId,
     MinBlock: heighestBlock,
 }
 */

InboundMessage::reply_calls_t InboundBlockHeaders::execute() {
    using namespace std;

    BlockNum highestBlock = 0;
    for(Header& header: packet_.request) {
        highestBlock = std::max(highestBlock, header.number);
    }

    reply_calls_t reply_calls;

    auto [penalty, requestMoreHeaders] = STAGE1.working_chain().accept_headers(packet_.request, peerId_);

    /* really do we need to call request_more_headers() here? it will be called by header_forward()...
     * or do we only need to enable header_forward() if blocked?
     * todo: take a decision here!
     * If we need it:

    if (penalty == Penalty::NoPenalty && requestMoreHeaders) {
        auto [packet, penalties] = STAGE1.working_chain().request_more_headers();
        auto msg = new OutboundGetBlockHeaders(packet); // todo: modify OutboundGetBlockHeaders to handle this case
        STAGE1.add_to_message_queue(msg);

        if (penalties) {
            for(auto penalty: penalties)
                reply_calls.push_back(rpc::PenalizePeer::make(penalty.peerId, penalty.reason));
        }
    }

     * other way to implement it:

    if (penalty == Penalty::NoPenalty && requestMoreHeaders) {
        auto msg = new OutboundGetBlockHeaders(REQUEST_MORE_HEADERS); // calls request_more_headers() and PenalizePeer
        STAGE1.add_to_message_queue(msg);
    }
    */

    if (penalty != Penalty::NoPenalty)
        reply_calls.push_back(rpc::PenalizePeer::make(peerId_, penalty));

    reply_calls.push_back(rpc::PeerMinBlock::make(peerId_, highestBlock));

    return reply_calls;
}

void InboundBlockHeaders::handle_completion([[maybe_unused]] SentryRpc&) {
    // There is no replay
}

uint64_t InboundBlockHeaders::reqId() const {
    return packet_.requestId;
}

std::string InboundBlockHeaders::content() const {
    std::stringstream content;
    content << packet_;
    return content.str();
}


}