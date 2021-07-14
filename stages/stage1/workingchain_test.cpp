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

#include <catch2/catch.hpp>
#include "WorkingChain.hpp"
#include <algorithm>

namespace silkworm {
    // Useful definitions
    // ----------------------------------------------------------------------------

    class WorkingChain_ForTest: public WorkingChain {
    public: // publication of internal members to test methods functioning
        using WorkingChain::WorkingChain;
        using WorkingChain::anchorQueue_;
        using WorkingChain::anchors_;
        using WorkingChain::linkQueue_;
        using WorkingChain::links_;
    };

/*
    long int difficulty(const BlockHeader& header, const BlockHeader& parent) {
        return static_cast<long int>(parent.difficulty) +
               static_cast<long int>(parent.difficulty / 2048) * std::max(1 - static_cast<long int>(header.timestamp - parent.timestamp) / 10, -99L)
               + int(2^((header.number / 100000) - 2));
    }
*/
    // TESTs related to HeaderList::split_into_segments
    // ----------------------------------------------------------------------------

    TEST_CASE("HeaderList::split_into_segments - No headers") {
        std::vector<BlockHeader> headers;

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(segments.size() == 0);
        REQUIRE(penalty == Penalty::NoPenalty);
    }

    TEST_CASE("HeaderList::split_into_segments - Single header") {
        std::vector<BlockHeader> headers;
        BlockHeader header;
        header.number = 5;
        headers.push_back(header);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(segments.size() == 1);
        REQUIRE(penalty == Penalty::NoPenalty);
    }

    TEST_CASE("HeaderList::split_into_segments - Single header repeated twice") {
        std::vector<BlockHeader> headers;
        BlockHeader header;
        header.number = 5;
        headers.push_back(header);
        headers.push_back(header);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(segments.size() == 0);
        REQUIRE(penalty == Penalty::DuplicateHeaderPenalty);
    }

    TEST_CASE("HeaderList::split_into_segments - Two connected headers") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 1);                     // 1 segment
        REQUIRE(segments[0].size() == 2);                  // 2 headers
        REQUIRE(segments[0][0]->number == header2.number); // the highest at the beginning
        REQUIRE(segments[0][1]->number == header1.number);
    }

    TEST_CASE("HeaderList::split_into_segments - Two connected headers with wrong numbers") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 3;  // Expected block-number = 2
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(segments.size() == 0);
        REQUIRE(penalty == Penalty::WrongChildBlockHeightPenalty);
    }

/* WrongChildDifficultyPenalty check is not implemented in Erigon/Silkworm code, so this test is commented
    TEST_CASE("HeaderList::split_into_segments - Two connected headers with wrong difficulty") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 2000; // Expected difficulty 10 + 1000 = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(segments.size() == 0);
        REQUIRE(penalty == Penalty::WrongChildDifficultyPenalty);
    }
*/

    /* input:
     *         h1 <----- h2
     *               |-- h3
     * output:
     *         3 segments: {h3}, {h2}, {h1}   (in this order)
     */
    TEST_CASE("HeaderList::split_into_segments - Two headers connected to the third header") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        BlockHeader header3;
        header3.number = 2;
        header3.difficulty = 1010;
        header3.parent_hash = header1.hash();
        header3.extra_data = bytes_of_string("I'm different"); // To make sure the hash of h3 is different from the hash of h2
        headers.push_back(header3);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 3);                     // 3 segment
        REQUIRE(segments[0].size() == 1);                  // 1 headers
        REQUIRE(segments[1].size() == 1);                  // 1 headers
        REQUIRE(segments[2].size() == 1);                  // 1 headers
        REQUIRE(segments[2][0]->number == header1.number); // expected h1 to be the root
        REQUIRE(segments[1][0]->number == header2.number);
        REQUIRE(segments[0][0]->number == header3.number);
    }

    TEST_CASE("HeaderList::split_into_segments - Same three headers, but in a reverse order") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();

        BlockHeader header3;
        header3.number = 2;
        header3.difficulty = 1010;
        header3.parent_hash = header1.hash();
        header3.extra_data = bytes_of_string("I'm different"); // To make sure the hash of h3 is different from the hash of h2

        headers.push_back(header3);
        headers.push_back(header2);
        headers.push_back(header1);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 3);                     // 3 segment
        REQUIRE(segments[0].size() == 1);                  // 1 headers
        REQUIRE(segments[2][0]->number == header1.number); // expected h1 to be the root
        REQUIRE(segments[1][0]->number == header2.number);
        REQUIRE(segments[0][0]->number == header3.number);
    }

    /* input:
     *         (...) <----- h2
     *                  |-- h3
     * output:
     *         2 segments: {h3?}, {h2?}
     */
    TEST_CASE("HeaderList::split_into_segments - Two headers not connected to each other") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();

        BlockHeader header3;
        header3.number = 2;
        header3.difficulty = 1010;
        header3.parent_hash = header1.hash();
        header3.extra_data = bytes_of_string("I'm different"); // To make sure the hash of h3 is different from the hash of h2

        headers.push_back(header3);
        headers.push_back(header2);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 2);                     // 1 segment
        REQUIRE(segments[0].size() == 1);                  // 1 header each
        REQUIRE(segments[1].size() == 1);                  // 1 header each
        REQUIRE(segments[0][0] != segments[1][0]);  // different headers
    }

    /* input:
     *         h1 <----- h2 <----- h3
     * output:
     *        1 segment: {h3, h2, h1}   (with header in this order)
     */
    TEST_CASE("HeaderList::split_into_segments - Three headers connected") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        BlockHeader header3;
        header3.number = 3;
        header3.difficulty = 101010;
        header3.parent_hash = header2.hash();
        headers.push_back(header3);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 1);                     // 1 segment
        REQUIRE(segments[0].size() == 3);                  // 3 headers
        REQUIRE(segments[0][0]->number == header3.number); // expected h3 at the top
        REQUIRE(segments[0][1]->number == header2.number);
        REQUIRE(segments[0][2]->number == header1.number); // expected h1 at the bottom
    }

    /* input:
     *         h1 <----- h2 <----- h3
     *                         |-- h4
     *
     * output:
     *        3 segments: {h3?}, {h4?}, {h2, h1}
     */
    TEST_CASE("HeaderList::split_into_segments - Four headers connected") {
        std::vector<BlockHeader> headers;

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;
        headers.push_back(header1);

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();
        headers.push_back(header2);

        BlockHeader header3;
        header3.number = 3;
        header3.difficulty = 101010;
        header3.parent_hash = header2.hash();
        headers.push_back(header3);

        BlockHeader header4;
        header4.number = 3;
        header4.difficulty = 101010;
        header4.parent_hash = header2.hash();
        header4.extra_data = bytes_of_string("I'm different");
        headers.push_back(header4);

        auto headerList = HeaderList::make(headers);

        auto [segments, penalty] = headerList->split_into_segments();

        REQUIRE(penalty == Penalty::NoPenalty);
        REQUIRE(segments.size() == 3);                     // 3 segment
        REQUIRE(segments[0].size() == 1);                  // segment 0 - 1 headers
        REQUIRE(segments[1].size() == 1);                  // segment 1 - 1 headers
        REQUIRE(segments[2].size() == 2);                  // segment 2 - 2 headers
        REQUIRE(segments[2][0]->number == header2.number);
        REQUIRE(segments[2][1]->number == header1.number);
        REQUIRE(segments[0][0] != segments[1][0]);
    }

    // TESTs related to WorkingChain::accept_headers (segment manipulation: connect, extend_down, extend_up, new_anchor
    // ----------------------------------------------------------------------------

    /* chain status:
     *         void
     *
     * input:
     *         h1 <----- h2
     *
     * output:
     *         1 anchor, ?
     */

    TEST_CASE("WorkingChain::process_segment - test1 - new_anchor + extend_up") {
        using namespace std;

        BlockNum highestInDb = 0;
        BlockNum topSeenHeight = 1'000'000;

        WorkingChain_ForTest chain(highestInDb, topSeenHeight);

        PeerId peerId = "1";

        BlockHeader header1;
        header1.number = 1;
        header1.difficulty = 10;

        BlockHeader header2;
        header2.number = 2;
        header2.difficulty = 1010;
        header2.parent_hash = header1.hash();

        INFO( "new_anchor" ) {
            auto[penalty, requestMoreHeaders] = chain.accept_headers({header1, header2}, peerId);

            REQUIRE(penalty == Penalty::NoPenalty);
            REQUIRE(requestMoreHeaders == true);
            REQUIRE(chain.anchorQueue_.size() == 1);
            REQUIRE(chain.anchors_.size() == 1);
            REQUIRE(chain.linkQueue_.size() == 2);
            REQUIRE(chain.links_.size() == 2);

            auto anchor = chain.anchors_[header1.parent_hash];
            REQUIRE(anchor != nullptr);
            REQUIRE(anchor->parentHash == header1.parent_hash);
            REQUIRE(anchor->blockHeight == header1.number);
            REQUIRE(anchor->peerId == peerId);

            REQUIRE(anchor->links.size() == 1);
            REQUIRE(anchor->links[0]->hash == header1.hash());
            REQUIRE(anchor->links[0]->next.size() == 1);
            REQUIRE(anchor->links[0]->next[0]->hash == header2.hash());
        }

        BlockHeader header3;
        header3.number = 3;
        header3.difficulty = 1010;
        header3.parent_hash = header2.hash();

        BlockHeader header4;
        header4.number = 4;
        header4.difficulty = 1010;
        header4.parent_hash = header3.hash();

        INFO( "extend_up" ) {
            auto[penalty, requestMoreHeaders] = chain.accept_headers({header3, header4}, peerId);

            REQUIRE(penalty == Penalty::NoPenalty);
            REQUIRE(requestMoreHeaders == false);
            REQUIRE(chain.anchorQueue_.size() == 1);
            REQUIRE(chain.anchors_.size() == 1);
            REQUIRE(chain.linkQueue_.size() == 4);
            REQUIRE(chain.links_.size() == 4);

            auto anchor = chain.anchors_[header1.parent_hash];
            REQUIRE(anchor != nullptr);
            REQUIRE(anchor->parentHash == header1.parent_hash);
            REQUIRE(anchor->blockHeight == header1.number);
            REQUIRE(anchor->links.size() == 1);

            REQUIRE(anchor->links[0]->hash == header1.hash());
            REQUIRE(anchor->links[0]->next.size() == 1);
            REQUIRE(anchor->links[0]->next[0]->hash == header2.hash());
            REQUIRE(anchor->links[0]->next[0]->next.size() == 1);
            REQUIRE(anchor->links[0]->next[0]->next[0]->hash == header3.hash());
            REQUIRE(anchor->links[0]->next[0]->next[0]->next.size() == 1);
            REQUIRE(anchor->links[0]->next[0]->next[0]->next[0]->hash == header4.hash());
        }

    }

}