#include "libcapt/Core/PacketBuilder.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <array>

using namespace Capt;

TEST(PacketBuilderTest, AppendByte) {
    constexpr auto payload = PacketBuilder()
        .AppendByte(1)
        .AppendByte(2)
        .AppendByte(3)
        .Payload;
    EXPECT_THAT(payload, testing::ElementsAre(1, 2, 3));
}

TEST(PacketBuilderTest, AppendUint16) {
    constexpr auto payload = PacketBuilder()
        .AppendUint16(0x1234)
        .AppendUint16(0x1200)
        .AppendUint16(0x0034)
        .Payload;
    EXPECT_THAT(payload, testing::ElementsAre(0x34, 0x12, 0x00, 0x12, 0x34, 0x00));
}

TEST(PacketBuilderTest, AppendUint32) {
    constexpr auto payload = PacketBuilder()
        .AppendUint32(0xffaabbcc)
        .AppendUint32(0x00fccf00)
        .Payload;
    EXPECT_THAT(payload, testing::ElementsAre(0xcc, 0xbb, 0xaa, 0xff, 0x00, 0xcf, 0xfc, 0x00));
}

TEST(PacketBuilderTest, AppendBytes) {
    std::array<uint8_t, 3> data = {0xff, 0x0f, 0xaa};
    auto payload = PacketBuilder()
        .AppendByte(1)
        .AppendByte(2)
        .AppendBytes(data)
        .AppendByte(3)
        .Payload;
    EXPECT_THAT(payload, testing::ElementsAre(1, 2, 0xff, 0x0f, 0xaa, 3));
}

/*
// Compile-time error expected

TEST(PacketBuilderTest, Overflow) {
    EXPECT_THROW({
        uint8_t data[0xffff - 7];
        PacketBuilder()
            .AppendBytes<sizeof(data)>(data)
            .AppendUint32(1);
    }, std::overflow_error);
}

TEST(PacketBuilderTest, AppendBytesOverflow) {
    EXPECT_THROW({
        uint8_t data[0xffff];
        PacketBuilder()
            .AppendBytes<sizeof(data)>(data);
    }, std::overflow_error);
}
*/
