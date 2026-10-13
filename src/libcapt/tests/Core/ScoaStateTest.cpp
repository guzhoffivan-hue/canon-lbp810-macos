#include "libcapt/Compression/ScoaState.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <ostream>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

using namespace Capt::Compression;

namespace Capt::Compression {
    std::ostream& operator<<(std::ostream& os, const ScoaFuncType& type) {
        std::string_view s = "UNK";
        switch (type) {
        case ScoaFuncType::Copy: s = "CPY"; break;
        case ScoaFuncType::Raw: s = "RAW"; break;
        case ScoaFuncType::Repeat: s = "REP"; break;
        }
        os << s;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const ScoaFunc& func) {
        os << "ScoaFunc[" << func.Type << " idx=" << func.Index << " count=" << func.Count << "]";
        return os;
    }
}

static void processLine(std::vector<ScoaFunc>& funcs, ScoaState& state, std::span<const uint8_t> line, std::span<const uint8_t> prevLine) {
    while (!state.Empty()) {
        ScoaFunc f = state.Get();
        funcs.push_back(f);
    }

    std::size_t count = 0;
    std::size_t index = 0;
    for (const ScoaFunc& f : funcs) {
        ASSERT_NE(f.Count, 0);
        count += f.Count;
        ASSERT_EQ(index, f.Index);
        index = f.Index + f.Count;
        auto payload = f.Payload(line);
        ASSERT_EQ(payload.size(), f.Count);
        if (f.Type == ScoaFuncType::Repeat) {
            ASSERT_GE(f.Count, 2);
            for (std::size_t i = 0; i < f.Count; i++) {
                ASSERT_EQ(payload[i], line[f.Index]);
            }
        } else if (f.Type == ScoaFuncType::Copy) {
            for (std::size_t i = 0; i < f.Count; i++) {
                ASSERT_EQ(payload[i], prevLine[f.Index + i]);
            }
        }
    }
    ASSERT_EQ(count, line.size());
}

TEST(ScoaStateTest, SingleLine) {
    std::vector<std::pair<std::vector<uint8_t>, std::vector<ScoaFunc>>> cases = {
        {
            {1, 2, 3, 4, 5},
            {{ScoaFuncType::Raw, 0, 5}},
        },
        {
            {1, 2, 3, 4, 4},
            {{ScoaFuncType::Raw, 0, 3}, {ScoaFuncType::Repeat, 3, 2}},
        },
        {
            {1, 2, 4, 4, 4},
            {{ScoaFuncType::Raw, 0, 2}, {ScoaFuncType::Repeat, 2, 3}},
        },
        {
            {4, 2, 4, 4, 4},
            {{ScoaFuncType::Raw, 0, 2}, {ScoaFuncType::Repeat, 2, 3}},
        },
        {
            {4, 4, 4, 4, 4},
            {{ScoaFuncType::Repeat, 0, 5}},
        },
        {
            {4, 1, 4, 1, 4},
            {{ScoaFuncType::Raw, 0, 5}},
        },
        {
            {4, 1, 1, 1, 4},
            {{ScoaFuncType::Raw, 0, 1}, {ScoaFuncType::Repeat, 1, 3}, {ScoaFuncType::Raw, 4, 1}},
        },
        {
            {1, 1, 3, 4, 5},
            {{ScoaFuncType::Repeat, 0, 2}, {ScoaFuncType::Raw, 2, 3}},
        },
        {
            {3, 1, 1, 5, 5},
            {{ScoaFuncType::Raw, 0, 1}, {ScoaFuncType::Repeat, 1, 2}, {ScoaFuncType::Repeat, 3, 2}},
        },
    };

    for (std::size_t i = 0; i < cases.size(); i++) {
        ScoaState state(cases[i].first, {});
        std::vector<ScoaFunc> funcs;
        processLine(funcs, state, cases[i].first, {});
        EXPECT_THAT(funcs, testing::ElementsAreArray(cases[i].second)) << "failed case #" << i;
    }
}

TEST(ScoaStateTest, MultiLine) {
    std::vector<std::pair<std::vector<std::vector<uint8_t>>, std::vector<std::vector<ScoaFunc>>>> cases = {
        {
            {
                /* 0 */ {1, 2, 3, 4, 5},
                /* 1 */ {1, 2, 3, 4, 5},
                /* 2 */ {1, 2, 3, 4, 4},
                /* 3 */ {1, 2, 4, 4, 4},
                /* 4 */ {1, 4, 4, 4, 4},
                /* 5 */ {4, 3, 4, 3, 4},
                /* 6 */ {5, 5, 4, 5, 5},
            },
            {
                /* 0 */ {{ScoaFuncType::Raw, 0, 5}},
                /* 1 */ {{ScoaFuncType::Copy, 0, 5}},
                /* 2 */ {{ScoaFuncType::Copy, 0, 4}, {ScoaFuncType::Raw, 4, 1}},
                /* 3 */ {{ScoaFuncType::Copy, 0, 2}, {ScoaFuncType::Repeat, 2, 3}},
                /* 4 */ {{ScoaFuncType::Copy, 0, 1}, {ScoaFuncType::Repeat, 1, 4}},
                /* 5 */ {{ScoaFuncType::Raw, 0, 2}, {ScoaFuncType::Copy, 2, 1}, {ScoaFuncType::Raw, 3, 1}, {ScoaFuncType::Copy, 4, 1}},
                /* 6 */ {{ScoaFuncType::Repeat, 0, 2}, {ScoaFuncType::Copy, 2, 1}, {ScoaFuncType::Repeat, 3, 2}},
            },
        },
        {
            {
                /* 0 */ {0, 0, 0, 0, 0},
                /* 1 */ {0, 0, 0, 0, 0},
                /* 2 */ {1, 0, 0, 0, 0},
                /* 3 */ {1, 1, 0, 0, 0},
            },
            {
                /* 0 */ {{ScoaFuncType::Repeat, 0, 5}},
                /* 1 */ {{ScoaFuncType::Copy, 0, 5}},
                /* 2 */ {{ScoaFuncType::Raw, 0, 1}, {ScoaFuncType::Copy, 1, 4}},
                /* 3 */ {{ScoaFuncType::Copy, 0, 1}, {ScoaFuncType::Raw, 1, 1}, {ScoaFuncType::Copy, 2, 3}},
            },
        },
        {
            {
                /* 0 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                /* 1 */ {0, 1, 2, 1, 4, 0, 1, 2, 4, 0},
            },
            {
                /* 0 */ {{ScoaFuncType::Repeat, 0, 10}},
                /* 1 */ {{ScoaFuncType::Copy, 0, 1}, {ScoaFuncType::Raw, 1, 4}, {ScoaFuncType::Copy, 5, 1}, {ScoaFuncType::Raw, 6, 3}, {ScoaFuncType::Copy, 9, 1}},
            },
        },
    };

    for (std::size_t i = 0; i < cases.size(); i++) {
        for (std::size_t j = 0; j < cases[i].first.size(); j++) {
            auto prevLine = j != 0 ? cases[i].first[j - 1] : std::span<const uint8_t>{};
            ScoaState state(cases[i].first[j], prevLine);
            std::vector<ScoaFunc> funcs;
            processLine(funcs, state, cases[i].first[j], prevLine);
            EXPECT_THAT(funcs, testing::ElementsAreArray(cases[i].second[j])) << "failed case #" << i << " line #" << j;
        }
    }
}
