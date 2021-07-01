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

#include <functional>

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

/*
 * From Erigon:
		currentTime := uint64(time.Now().Unix())
		req, penalties := cfg.hd.RequestMoreHeaders(currentTime)
		if req != nil {
			peer = cfg.headerReqSend(ctx, req)
			if peer != nil {
				cfg.hd.SentRequest(req, currentTime, 5 / timeout /)
				log.Debug("Sent request", "height", req.Number)
			}
		}
		cfg.penalize(ctx, penalties)
		maxRequests := 64 // Limit number of requests sent per round to let some headers to be inserted into the database
		for req != nil && peer != nil && maxRequests > 0 {
			req, penalties = cfg.hd.RequestMoreHeaders(currentTime)
			if req != nil {
				peer = cfg.headerReqSend(ctx, req)
				if peer != nil {
					cfg.hd.SentRequest(req, currentTime, 5 /timeout /)
					log.Debug("Sent request", "height", req.Number)
				}
			}
			cfg.penalize(ctx, penalties)
			maxRequests--
		}

		// Send skeleton request if required
		req = cfg.hd.RequestSkeleton()
		if req != nil {
			peer = cfg.headerReqSend(ctx, req)
			if peer != nil {
				log.Debug("Sent skeleton request", "height", req.Number)
			}
		}
		// Load headers into the database
		var inSync bool
		if inSync, err = cfg.hd.InsertHeaders(headerInserter.FeedHeaderFunc(tx), logPrefix, logEvery.C); err != nil {
			return err
		}
		announces := cfg.hd.GrabAnnounces()
		if len(announces) > 0 {
			cfg.announceNewHashes(ctx, announces)
		}
		if headerInserter.BestHeaderChanged() { // We do not break unless there best header changed
			if !initialCycle {
				// if this is not an initial cycle, we need to react quickly when new headers are coming in
				break
			}
			// if this is initial cycle, we want to make sure we insert all known headers (inSync)
			if inSync {
				break
			}
		}

 */
std::optional<GetBlockHeadersPacket66> WorkingChain::headers_forward() {
    // todo: implements!

    //auto [packet, penalties] = request_more_headers();
    // when the packet is sent we need to update some structures here

    // ...

    // only for test:
    return request_skeleton();
}

/*
 * Skeleton query.
 * Request "seed" headers that can became anchors.
 * It requests N headers starting at highestInDb with step = stride up to topSeenHeight.
 * Note that skeleton queries are only generated when current number of non-persisted chain bundles (which is equal
 * to number of anchors) is below certain threshold (currently 16). This is because processing an answer to a skeleton
 * request would normally create up to 192 new anchors, and then it will take some time for the second type of queries
 * (anchor extension queries) to fill the gaps and so reduce the number of anchors.
*/
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

/*
 * Anchor extension query.
 * The function uses an auxiliary data structure, anchorQueue to decide which anchors to select for queries first.
 * anchorQueue is a priority queue of anchors, priorities by the timestamp of latest anchor extension query issued
 * for an anchor. Anchors for which the extension queries were not issued for the longest time, come on top.
 * The anchor on top gets repeated query, but only after certain timeout (currently 5 second) since the last query,
 * and only of the anchor still exists (i.e. it has not been extended yet). Also, if an anchor gets certain number
 * of extension requests issued (currently 10), but without luck of being extended, that anchor gets invalidated,
 * and all its descendants get deleted from consideration (invalidate_anchor function). This would happen if anchor
 * was "fake", i.e. it corresponds to a header without existing ancestors.
 */
std::optional<GetBlockHeadersPacket66> WorkingChain::request_more_headers() {
    // time = currentTime
    // todo: implement!
    return {};
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

/*
// SplitIntoSegments converts message containing headers into a collection of chain segments
func (hd *HeaderDownload) SplitIntoSegments(headersRaw [][]byte, msg []*types.Header) ([]*ChainSegment, Penalty, error) {
	hd.lock.RLock()
	defer hd.lock.RUnlock()
	sort.Sort(HeadersByBlockHeight(msg))
	// Now all headers are order from the highest block height to the lowest
	var segments []*ChainSegment                         // Segments being built
	segmentMap := make(map[common.Hash]int)              // Mapping of the header hash to the index of the chain segment it belongs
	childrenMap := make(map[common.Hash][]*types.Header) // Mapping parent hash to the children
	dedupMap := make(map[common.Hash]struct{})           // Map used for detecting duplicate headers
	for i, header := range msg {
		headerHash := header.Hash()
		if _, bad := hd.badHeaders[headerHash]; bad {
			return nil, BadBlockPenalty, nil
		}
		if _, duplicate := dedupMap[headerHash]; duplicate {
			return nil, DuplicateHeaderPenalty, nil
		}
		dedupMap[headerHash] = struct{}{}
		var segmentIdx int
		children := childrenMap[headerHash]
		for _, child := range children {
			if valid, penalty := hd.childParentValid(child, header); !valid {
				return nil, penalty, nil
			}
		}
		if len(children) == 1 {
			// Single child, extract segmentIdx
			segmentIdx = segmentMap[headerHash]
		} else {
			// No children, or more than one child, create new segment
			segmentIdx = len(segments)
			segments = append(segments, &ChainSegment{})
		}
		segments[segmentIdx].Headers = append(segments[segmentIdx].Headers, header)
		segments[segmentIdx].HeadersRaw = append(segments[segmentIdx].HeadersRaw, headersRaw[i])
		segmentMap[header.ParentHash] = segmentIdx
		siblings := childrenMap[header.ParentHash]
		siblings = append(siblings, header)
		childrenMap[header.ParentHash] = siblings
	}
	return segments, NoPenalty, nil
}
 */

std::tuple<bool,Penalty> childParentValidity(Header_Ref child, Header_Ref parent) {
    if (parent->number + 1 != child->number)
        return {false, Penalty::WrongChildBlockHeightPenalty};
    return {true, NoPenalty};
}

std::tuple<bool,Penalty> childrenParentValidity(const std::vector<Header_Ref>& children, Header_Ref parent) {
    for(auto& child: children) {
        auto [valid, penalty] = childParentValidity(child, parent);
        if (!valid)
            return {false, penalty};
    }
    return {true, Penalty::NoPenalty};
}

/*
 * SplitIntoSegments converts message containing headers into a collection of chain segments.
 * A message received from a peer may contain a collection of disparate headers (for example, in a response to the
 * skeleton query), or any branched chain bundle. So it needs to be split into chain segments.
 * SplitIntoSegments takes a collection of headers and return a collection of chain segments in a specific order.
 * This order is the ascending order of the lowest block height in the segment.
 * There may be many possible ways to split a chain bundle into segments, we choose one that is simple and that assures
 * this properties:
 *    - segments form a partial order
 *    - whatever part of the chain that becomes canonical it is not necessary to redo the process of division into segments
 */
auto WorkingChain::split_into_segments(const std::vector<BlockHeader>& full_headers) -> std::tuple<std::vector<Segment>, Penalty> {
    std::vector<Header_Ref> headers = to_ref(full_headers);
    std::sort(headers.begin(), headers.end(), [](auto& h1, auto& h2){return h1->number > h2->number;}); // sort headers from the highest block height to the lowest

    std::vector<Segment> segments;
    std::map<Hash, int> segmentMap;
    std::map<Hash, std::vector<Header_Ref>> childrenMap;
    std::set<Hash> dedupMap;
    int segmentIdx = 0;

    for(auto& header: headers) {
        Hash header_hash = header->hash();
        if (badHeaders_.contains(header_hash))
            return {{}, Penalty::BadBlockPenalty};
        if (dedupMap.contains(header_hash))
            return {{}, Penalty::DuplicateHeaderPenalty};
        dedupMap.insert(header_hash);
        auto children = childrenMap[header_hash];

        auto [valid, penalty] = childrenParentValidity(children, header);
        if (!valid) return {{}, penalty};

        if (children.size() == 1) {
            // Single child, extract segmentIdx
            segmentIdx = segmentMap[header_hash];
        }
        else {
            // No children, or more than one child, create new segment
            segmentIdx = segments.size();
            segments.emplace_back();    // add a void segment
        }

        segments[segmentIdx].headers.push_back(header);
        //segments[segmentIdx].headersRaw.push_back(headersRaw[i]); // todo: do we need this?

        segmentMap[header->parent_hash] = segmentIdx;

        auto& siblings = childrenMap[header->parent_hash];
        siblings.push_back(header);
    }

    return {segments, Penalty::NoPenalty};
}
/* implementation without Header_Ref
std::tuple<bool,Penalty> childParentValidity(const BlockHeader& child, const BlockHeader& parent) {
    if (parent.number + 1 != child.number)
        return {false, Penalty::WrongChildBlockHeightPenalty};
    return {true, NoPenalty};
}

std::tuple<bool,Penalty> childrenParentValidity(const std::vector<BlockHeader>& children, const BlockHeader& parent) {
    for(auto& child: children) {
        auto [valid, penalty] = childParentValidity(child, parent);
        if (!valid)
            return {false, penalty};
    }
    return {true, Penalty::NoPenalty};
}

auto WorkingChain::split_into_segments(const std::vector<BlockHeader>& headers) -> std::tuple<std::vector<Segment>, Penalty> {

    std::sort(headers.begin(), headers.end(), [](auto& h1, auto& h2){return h1.number > h2.number;}); // sort headers from the highest block height to the lowest

    std::vector<Segment> segments;
    std::map<Hash, int> segmentMap;
    std::map<Hash, std::vector<BlockHeader>> childrenMap;
    std::set<Hash> dedupMap;
    int segmentIdx = 0;

    for(auto& header: headers) {
        if (badHeaders_.contains(header.hash()))
            return {{}, Penalty::BadBlockPenalty};
        if (dedupMap.contains(header.hash()))
            return {{}, Penalty::DuplicateHeaderPenalty};
        dedupMap.insert(header.hash());
        auto children = childrenMap[header.hash()];

        auto [valid, penalty] = childrenParentValidity(children, header);
        if (!valid) return {{}, penalty};

        if (children.size() == 1) {
            // Single child, extract segmentIdx
            segmentIdx = segmentMap[header.hash()];
        }
        else {
            // No children, or more than one child, create new segment
            segmentIdx = segments.size();
            segments.emplace_back();    // add a void segment
        }

        segments[segmentIdx].headers.push_back(header); // todo: copy o reference?
        //segments[segmentIdx].headersRaw.push_back(headersRaw[i]); // todo: do we need this?

        segmentMap[header.parent_hash] = segmentIdx;

        auto& siblings = childrenMap[header.parent_hash];
        siblings.push_back(header);     // todo: copy o reference?
    }

    return {segments, Penalty::NoPenalty};
}
*/

/*
// ProcessSegment - handling single segment.
// If segment were processed by extendDown or newAnchor method, then it returns `requestMore=true`
// it allows higher-level algo immediately request more headers without waiting all stages precessing,
// speeds up visibility of new blocks
// It remember peerID - then later - if anchors created from segments will abandoned - this peerID gonna get Penalty
func (hd *HeaderDownload) ProcessSegment(segment *ChainSegment, newBlock bool, peerID string) (requestMore bool) {
	log.Debug("processSegment", "from", segment.Headers[0].Number.Uint64(), "to", segment.Headers[len(segment.Headers)-1].Number.Uint64())
	hd.lock.Lock()
	defer hd.lock.Unlock()
	foundAnchor, start := hd.findAnchors(segment)
	foundTip, end := hd.findLink(segment, start) // We ignore penalty because we will check it as part of PoW check
	if end == 0 {
		log.Debug("Duplicate segment")
		return
	}
	height := segment.Headers[len(segment.Headers)-1].Number.Uint64()
	hash := segment.Headers[len(segment.Headers)-1].Hash()
	if newBlock || hd.seenAnnounces.Seen(hash) {
		if height > hd.topSeenHeight {
			hd.topSeenHeight = height
		}
	}
	startNum := segment.Headers[start].Number.Uint64()
	endNum := segment.Headers[end-1].Number.Uint64()
	// There are 4 cases
	if foundAnchor {
		if foundTip {
			// Connect
			if err := hd.connect(segment, start, end); err != nil {
				log.Debug("Connect failed", "error", err)
				return
			}
			log.Debug("Connected", "start", startNum, "end", endNum)
		} else {
			// ExtendDown
			var err error
			if requestMore, err = hd.extendDown(segment, start, end); err != nil {
				log.Debug("ExtendDown failed", "error", err)
				return
			}
			log.Debug("Extended Down", "start", startNum, "end", endNum)
		}
	} else if foundTip {
		if end > 0 {
			// ExtendUp
			if err := hd.extendUp(segment, start, end); err != nil {
				log.Debug("ExtendUp failed", "error", err)
				return
			}
			log.Debug("Extended Up", "start", startNum, "end", endNum)
		}
	} else {
		// NewAnchor
		var err error
		if requestMore, err = hd.newAnchor(segment, start, end, peerID); err != nil {
			log.Debug("NewAnchor failed", "error", err)
			return
		}
		log.Debug("NewAnchor", "start", startNum, "end", endNum)
	}
	//log.Info(hd.anchorState())
	log.Debug("Link queue", "size", hd.linkQueue.Len())
	if hd.linkQueue.Len() > hd.linkLimit {
		log.Debug("Too many links, cutting down", "count", hd.linkQueue.Len(), "tried to add", end-start, "limit", hd.linkLimit)
	}
	for hd.linkQueue.Len() > hd.linkLimit {
		link := heap.Pop(hd.linkQueue).(*Link)
		delete(hd.links, link.hash)
		if parentLink, ok := hd.links[link.header.ParentHash]; ok {
			for i, n := range parentLink.next {
				if n == link {
					if i == len(parentLink.next)-1 {
						parentLink.next = parentLink.next[:i]
					} else {
						parentLink.next = append(parentLink.next[:i], parentLink.next[i+1:]...)
					}
					break
				}
			}
		}
		if anchor, ok := hd.anchors[link.header.ParentHash]; ok {
			for i, n := range anchor.links {
				if n == link {
					if i == len(anchor.links)-1 {
						anchor.links = anchor.links[:i]
					} else {
						anchor.links = append(anchor.links[:i], anchor.links[i+1:]...)
					}
					break
				}
			}
		}
	}
	select {
	case hd.DeliveryNotify <- struct{}{}:
	default:
	}

	return hd.requestChaining && requestMore
}

*/
auto WorkingChain::process_segment(Segment segment) -> RequestMoreHeaders {

    auto [foundAnchor, start] = find_anchor(segment);
    auto [foundTip, end] = find_link(segment, start);

    if (end == 0) {
        SILKWORM_LOG(LogLevel::Debug) << "WorkingChain: duplicate segment\n";
        return false;
    }

    auto lowest_header = *segment.headers.rbegin();
    [[maybe_unused]] auto height = lowest_header->number;
    [[maybe_unused]] auto hash = lowest_header->hash();

    // todo: implement!

    return false;
}

/*
// FindAnchors attempts to find anchors to which given chain segment can be attached to
func (hd *HeaderDownload) findAnchors(segment *ChainSegment) (found bool, start int) {
	// Walk the segment from children towards parents
	for i, header := range segment.Headers {
		// Check if the header can be attached to an anchor of a working tree
		if _, attaching := hd.anchors[header.Hash()]; attaching {
			return true, i
		}
	}
	return false, 0
}
*/

// find_anchors tries to finds the highest link the in the new segment that can be attached to an existing anchor
auto WorkingChain::find_anchor(Segment segment) -> std::tuple<Found, Start> { // todo: do we need a span?
    for (size_t i = 0; i < segment.headers.size(); i++)
        if (anchors_.find(segment.headers[i]->hash()) != anchors_.end()) // todo: hash() compute the value
            return {true, i};

    return {false, 0};
}

/*
// FindLink attempts to find a non-persisted link that given chain segment can be attached to.
func (hd *HeaderDownload) findLink(segment *ChainSegment, start int) (found bool, end int) {
	if _, duplicate := hd.getLink(segment.Headers[start].Hash()); duplicate {
		return false, 0
	}
	// Walk the segment from children towards parents
	for i, header := range segment.Headers[start:] {
		// Check if the header can be attached to any links
		if _, attaching := hd.getLink(header.ParentHash); attaching {
			return true, start + i + 1
		}
	}
	return false, len(segment.Headers)
}
 */
// find_link find the highest existing link (from start) that the new segment can be attached to
auto WorkingChain::find_link(Segment segment, int start) -> std::tuple<Found, End> {
    auto duplicate_link = get_link(segment.headers[start]->hash());
    if (duplicate_link)
        return {false, 0};
    for (size_t i = start; i < segment.headers.size(); i++) {
        // Check if the header can be attached to any links
        auto attaching_link = get_link(segment.headers[i]->parent_hash);
        if (attaching_link)
            return {true, i + 1}; // return the ordinal of the next link
    }
    return {false, segment.headers.size()};
}

auto WorkingChain::get_link(Hash hash) -> std::optional<std::shared_ptr<Link>> {
    auto it = links_.find(hash);
    if (it != links_.end())
        return {it->second};
    return {};
}

}