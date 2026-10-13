#include "libcapt/Core/PacketHeader.hpp"
#include "MemoryStream.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

using testing::ElementsAreArray;
using testing::ElementsAre;
using namespace Capt;

class PacketHeaderTest : public testing::Test {
protected:
    MemoryStream stream;
    PacketHeader header;

    void WriteOpcode(uint16_t opcode) {
        PacketHeader::WriteTo(this->stream, opcode);
        ASSERT_EQ(this->stream.Buffer().size(), 4);
    }

    void WritePayload(uint16_t opcode, const std::vector<uint8_t>& payload) {
        PacketHeader::WriteTo(this->stream, opcode, payload);
        ASSERT_EQ(this->stream.Buffer().size(), 4 + payload.size());
    }

    void Read(const std::vector<uint8_t>& data) {
        this->stream = MemoryStream(data);
        ASSERT_TRUE(this->stream >> this->header);
        ASSERT_EQ(this->header.Size(), data.size());
        ASSERT_EQ(this->header.PayloadSize, data.size() - 4);
    }
};

TEST_F(PacketHeaderTest, WriteNoPayload) {
    WriteOpcode(0x1234);
    EXPECT_THAT(stream.Buffer(), ElementsAre(0x34, 0x12, 0x04, 0x00));
}

TEST_F(PacketHeaderTest, WritePayload) {
    WritePayload(0x1234, {1, 2});
    EXPECT_THAT(stream.Buffer(), ElementsAre(0x34, 0x12, 6, 0, 1, 2));
}

TEST_F(PacketHeaderTest, WritePayload2) {
    WritePayload(0x1234, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    EXPECT_THAT(stream.Buffer(), ElementsAre(0x34, 0x12, 14, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST_F(PacketHeaderTest, WriteLongPayload) {
    std::vector<uint8_t> payload;
    std::vector<uint8_t> expected = {0x34, 0x12, 24, 2};
    for (std::size_t i = 0; i < 532; i++) {
        payload.push_back(i & 0xff);
        expected.push_back(i & 0xff);
    }
    WritePayload(0x1234, payload);
    EXPECT_THAT(stream.Buffer(), ElementsAreArray(expected));
}

TEST_F(PacketHeaderTest, WriteOverflow) {
    EXPECT_THROW({
        std::vector<uint8_t> payload(65535 - 4 + 1);
        WritePayload(0x1234, payload);
    }, std::overflow_error);
}

TEST_F(PacketHeaderTest, ReadInvalidSize) {
    EXPECT_THROW({
        stream.put(0x12);
        stream.put(0x34);
        stream.put(1);
        stream.put(0);
        ASSERT_TRUE(stream >> header);
    }, std::runtime_error);
}

TEST_F(PacketHeaderTest, ReadNoPayload) {
    Read({0x34, 0x12, 0x04, 0x00});
    ASSERT_EQ(header.Opcode, 0x1234);
}

TEST_F(PacketHeaderTest, ReadPayload) {
    Read({0x34, 0x12, 6, 0, 1, 2});
    ASSERT_EQ(header.Opcode, 0x1234);
    EXPECT_THAT(header.PayloadSize, 2);
}

TEST_F(PacketHeaderTest, ReadPayload2) {
    Read({0x34, 0x12, 14, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    ASSERT_EQ(header.Opcode, 0x1234);
    EXPECT_THAT(header.PayloadSize, 10);
}

TEST_F(PacketHeaderTest, ReadLongPayload) {
    std::vector<uint8_t> payload;
    std::vector<uint8_t> data = {0x34, 0x12, 24, 2};
    for (std::size_t i = 0; i < 532; i++) {
        payload.push_back(i & 0xff);
        data.push_back(i & 0xff);
    }
    Read(data);
    ASSERT_EQ(header.Opcode, 0x1234);
    EXPECT_THAT(header.PayloadSize, 532);
}
