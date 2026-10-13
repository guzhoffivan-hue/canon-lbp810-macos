#include "libcapt/Core/PacketReader.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <span>
#include <stdexcept>

using namespace Capt;

TEST(PacketReaderTest, ReadByte) {
    uint8_t payload[] = {1, 250, 0};
    PacketReader reader(payload);
    ASSERT_EQ(reader.ReadByte(), 1);
    ASSERT_EQ(reader.ReadByte(), 250);
    ASSERT_EQ(reader.ReadByte(), 0);
}

TEST(PacketReaderTest, ReadUint16) {
    uint8_t payload[] = {0x42, 0xff, 0xfe, 0};
    PacketReader reader(payload);
    ASSERT_EQ(reader.ReadUint16(), 0xff42);
    ASSERT_EQ(reader.ReadUint16(), 0x00fe);
}

TEST(PacketReaderTest, ReadUint32) {
    uint8_t payload[] = {0x42, 0xff, 0xfe, 0};
    PacketReader reader(payload);
    ASSERT_EQ(reader.ReadUint32(), 0x00feff42);
}

TEST(PacketReaderTest, Basic) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    PacketReader reader(payload);
    ASSERT_EQ(reader.ReadByte(), 1);
    ASSERT_EQ(reader.ReadUint16(), 0x42f2);
    ASSERT_EQ(reader.ReadUint32(), 0x0100feff);
}

TEST(PacketReaderTest, ReadByteEOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        PacketReader reader(payload);
        reader.ReadByte();
        reader.ReadByte();
    }, std::out_of_range);
}

TEST(PacketReaderTest, ReadUint16EOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        PacketReader reader(payload);
        reader.ReadUint16();
    }, std::out_of_range);
}

TEST(PacketReaderTest, ReadUint32EOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        PacketReader reader(payload);
        reader.ReadUint32();
    }, std::out_of_range);
}

TEST(PacketReaderTest, ReadBytesZero) {
    PacketReader reader({});
    ASSERT_EQ(reader.ReadBytes(0).size(), 0);
}

TEST(PacketReaderTest, ReadBytes) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    PacketReader reader(payload);
    ASSERT_THAT(reader.ReadBytes(1), testing::ElementsAre(1));
    ASSERT_THAT(reader.ReadBytes(2), testing::ElementsAre(0xf2, 0x42));
    ASSERT_THAT(reader.ReadBytes(4), testing::ElementsAre(0xff, 0xfe, 0, 1));
}

TEST(PacketReaderTest, ReadBytesEOF) {
    {
        uint8_t payload[] = {0x12};
        PacketReader reader(payload);
        ASSERT_THAT(reader.ReadBytes(1), testing::ElementsAre(0x12));
        ASSERT_EQ(reader.ReadBytes(0).size(), 0);
        EXPECT_THROW({
            reader.ReadBytes(1);
        }, std::out_of_range);
    }

    EXPECT_THROW({
        uint8_t payload[] = {0};
        PacketReader reader(payload);
        reader.ReadBytes(2);
    }, std::out_of_range);
}
