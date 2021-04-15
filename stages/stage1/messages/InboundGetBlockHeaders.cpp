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

#include "InboundGetBlockHeaders.hpp"
#include "../stage1.hpp"
#include "../Header.hpp"
#include "../gRPCAsyncClient.hpp"

namespace silkworm {


InboundGetBlockHeaders::InboundGetBlockHeaders(const sentry::InboundMessage& msg): InboundMessage() {
    if (msg.id() != sentry::MessageId::GetBlockHeaders)
        throw std::logic_error("InboundGetBlockHeaders received wrong InboundMessage");

    peerId_ = msg.peer_id();

    ByteView data = byte_view_of_string(msg.data()); // copy for consumption
    rlp::DecodingResult err = decode(data, packet_);
    if (err != rlp::DecodingResult::kOk)
        throw rlp::rlp_error("rlp decoding error decoding GetBlockHeaders");
}

void InboundGetBlockHeaders::execute() {
    using namespace std;
    vector<Header> headers;
    if (holds_alternative<Hash>(packet_.origin))
        headers = Header::recoverByHash(get<Hash>(packet_.origin), packet_.amount, packet_.skip, packet_.reverse);
    else
        headers = Header::recoverByNumber(get<BlockNum>(packet_.origin), packet_.amount, packet_.skip, packet_.reverse);

    auto rlp_encoding_len = rlp::length(headers);
    Bytes rlp_encoding(rlp_encoding_len, 0);
    rlp::encode(rlp_encoding, headers);

    auto msg_reply = new sentry::OutboundMessageData{};
    msg_reply->set_id(sentry::MessageId::BlockHeaders);
    msg_reply->set_data(rlp_encoding.data(), rlp_encoding.length()); // copy

    sentry::SendMessageByIdRequest reply; // todo: it is on the stack, is it ok?
    reply.set_peer_id(peerId_);
    reply.set_allocated_data(msg_reply);  // take ownership

    auto* call = new rpc::AsyncUnaryCall<sentry::Sentry, sentry::SendMessageByIdRequest, sentry::SentPeers>(&sentry::Sentry::Stub::PrepareAsyncSendMessageById, reply);
    // todo: who deletes call?

    STAGE1.sentry().send(call);
}

rlp::DecodingResult InboundGetBlockHeaders::decode(ByteView& from, GetBlockHeadersPacket& to) noexcept {
    using namespace rlp;

    auto [rlp_head, err]{decode_header(from)};
    if (err != DecodingResult::kOk) {
        return err;
    }
    if (!rlp_head.list) {
        return DecodingResult::kUnexpectedString;
    }

    uint64_t leftover{from.length() - rlp_head.payload_length};

    /* eth/66
    if (DecodingResult err{rlp::decode(from, to.requestId)}; err != DecodingResult::kOk) {
        return err;
    }
    */
    if (DecodingResult err{rlp::decode(from, to.origin)}; err != DecodingResult::kOk) {
        return err;
    }
    if (DecodingResult err{rlp::decode(from, to.amount)}; err != DecodingResult::kOk) {
        return err;
    }
    if (DecodingResult err{rlp::decode(from, to.skip)}; err != DecodingResult::kOk) {
        return err;
    }
    if (DecodingResult err{rlp::decode(from, to.reverse)}; err != DecodingResult::kOk) {
        return err;
    }

    return from.length() == leftover ? DecodingResult::kOk : DecodingResult::kListLengthMismatch;
}

}