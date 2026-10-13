#include "MemoryStream.hpp"
#include "libcapt/Core/PacketHeader.hpp"
#include "libcapt/Core/StreamPacket.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace Capt;

TEST(StreamPacketTest, Discard) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    EXPECT_EQ(packet.Remain(), sizeof(payload));
    ASSERT_EQ(packet.ReadByte(), 1);
    EXPECT_EQ(packet.Remain(), sizeof(payload)-1);
    ASSERT_EQ(packet.ReadByte(), 0xf2);
    EXPECT_EQ(packet.Remain(), sizeof(payload)-2);
    packet.Discard();
    EXPECT_EQ(packet.Remain(), 0);
    EXPECT_EQ(stream.Buffer().size(), 0);
}

TEST(StreamPacketTest, DtorDiscard) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);
    {
        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        EXPECT_EQ(packet.Remain(), sizeof(payload));
        ASSERT_EQ(packet.ReadByte(), 1);
        EXPECT_EQ(packet.Remain(), sizeof(payload)-1);
        ASSERT_EQ(packet.ReadByte(), 0xf2);
        EXPECT_EQ(packet.Remain(), sizeof(payload)-2);
    }
    EXPECT_EQ(stream.Buffer().size(), 0);
}

TEST(StreamPacketTest, MoveCtorDiscard) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    EXPECT_EQ(packet.Remain(), sizeof(payload));
    ASSERT_EQ(packet.ReadByte(), 1);
    EXPECT_EQ(packet.Remain(), sizeof(payload)-1);

    StreamPacket packet2(std::move(packet));
    EXPECT_EQ(packet.Remain(), 0);
    ASSERT_EQ(packet2.ReadByte(), 0xf2);
    EXPECT_EQ(packet2.Remain(), sizeof(payload)-2);
    packet2.Discard();
    EXPECT_EQ(packet2.Remain(), 0);
    EXPECT_EQ(stream.Buffer().size(), 0);
}

TEST(StreamPacketTest, MoveAssignDiscard) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    EXPECT_EQ(packet.Remain(), sizeof(payload));
    ASSERT_EQ(packet.ReadByte(), 1);
    EXPECT_EQ(packet.Remain(), sizeof(payload)-1);

    StreamPacket packet2(stream, PacketHeader{0x1234, 0});
    packet2 = std::move(packet);
    EXPECT_EQ(packet.Remain(), 0);
    ASSERT_EQ(packet2.ReadByte(), 0xf2);
    EXPECT_EQ(packet2.Remain(), sizeof(payload)-2);
    packet2.Discard();
    EXPECT_EQ(packet2.Remain(), 0);
    EXPECT_EQ(stream.Buffer().size(), 0);
}

TEST(StreamPacketTest, ReadByte) {
    uint8_t payload[] = {1, 250, 0};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    ASSERT_EQ(packet.ReadByte(), 1);
    ASSERT_EQ(packet.ReadByte(), 250);
    ASSERT_EQ(packet.ReadByte(), 0);
}

TEST(StreamPacketTest, ReadUint16) {
    uint8_t payload[] = {0x42, 0xff, 0xfe, 0};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    ASSERT_EQ(packet.ReadUint16(), 0xff42);
    ASSERT_EQ(packet.ReadUint16(), 0x00fe);
}

TEST(StreamPacketTest, ReadUint32) {
    uint8_t payload[] = {0x42, 0xff, 0xfe, 0};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    ASSERT_EQ(packet.ReadUint32(), 0x00feff42);
}

TEST(StreamPacketTest, Basic) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    ASSERT_EQ(packet.ReadByte(), 1);
    ASSERT_EQ(packet.ReadUint16(), 0x42f2);
    ASSERT_EQ(packet.ReadUint32(), 0x0100feff);
}

TEST(StreamPacketTest, ReadByteEOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        MemoryStream stream;
        PacketHeader::WriteTo(stream, 0x1234, payload);

        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        packet.ReadByte();
        packet.ReadByte();
    }, std::out_of_range);
}

TEST(StreamPacketTest, ReadUint16EOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        MemoryStream stream;
        PacketHeader::WriteTo(stream, 0x1234, payload);

        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        packet.ReadUint16();
    }, std::out_of_range);
}

TEST(StreamPacketTest, ReadUint32EOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        MemoryStream stream;
        PacketHeader::WriteTo(stream, 0x1234, payload);

        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        packet.ReadUint32();
    }, std::out_of_range);
}

TEST(StreamPacketTest, ReadBytes) {
    uint8_t payload[] = {1, 0xf2, 0x42, 0xff, 0xfe, 0, 1};
    MemoryStream stream;
    PacketHeader::WriteTo(stream, 0x1234, payload);

    StreamPacket packet;
    ASSERT_TRUE(stream >> packet);
    {
        uint8_t buff[1];
        packet.ReadBytes(buff);
        ASSERT_THAT(buff, testing::ElementsAre(1));
    }
    {
        uint8_t buff[2];
        packet.ReadBytes(buff);
        ASSERT_THAT(buff, testing::ElementsAre(0xf2, 0x42));
    }
    {
        uint8_t buff[4];
        packet.ReadBytes(buff);
        ASSERT_THAT(buff, testing::ElementsAre(0xff, 0xfe, 0, 1));
    }
}

TEST(StreamPacketTest, ReadBytesEOF) {
    EXPECT_THROW({
        uint8_t payload[] = {0};
        MemoryStream stream;
        PacketHeader::WriteTo(stream, 0x1234, payload);

        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        uint8_t buff[1];
        packet.ReadBytes(buff);
        packet.ReadBytes(buff);
    }, std::out_of_range);

    EXPECT_THROW({
        uint8_t payload[] = {0};
        MemoryStream stream;
        PacketHeader::WriteTo(stream, 0x1234, payload);

        StreamPacket packet;
        ASSERT_TRUE(stream >> packet);
        uint8_t buff[2];
        packet.ReadBytes(buff);
    }, std::out_of_range);
}
