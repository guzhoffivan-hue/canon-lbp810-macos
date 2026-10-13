#include "libcapt/Protocol/Protocol.hpp"
#include "libcapt/Core/PacketHeader.hpp"
#include "libcapt/Protocol/Enums.hpp"
#include "libcapt/Protocol/ExtendedStatus.hpp"
#include "libcapt/Protocol/PrinterInfo.hpp"
#include "libcapt/Protocol/ProtocolError.hpp"
#include "MemoryStream.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <cstdint>
#include <span>
#include <vector>

using namespace Capt;

class ProtocolTest : public testing::Test {
protected:
    MemoryStream stream;

    void WritePacket(uint16_t opcode, const std::vector<uint8_t>& payload = {}) {
        PacketHeader::WriteTo(this->stream, opcode, payload);
    }

    void ExpectPacket(uint16_t opcode, const std::vector<uint8_t>& payload = {}) {
        PacketHeader header;
        ASSERT_TRUE(this->stream >> header);
        EXPECT_EQ(header.Opcode, opcode);
        EXPECT_EQ(header.PayloadSize, payload.size());

        std::vector<uint8_t> buff(header.PayloadSize);
        ASSERT_TRUE(this->stream.read(reinterpret_cast<char*>(buff.data()), buff.size()));
        EXPECT_THAT(buff, testing::ElementsAreArray(payload));
    }
};

TEST_F(ProtocolTest, IC_BEGIN_PAGE) {
    Protocol::IC_BEGIN_PAGE(stream, PageParams{
        .PaperSize = 7,
        .TonerDensity = 0x1f,
        .Mode = 123,
        .Resolution = ResolutionIdx::RES_600,
        .SmoothEnable = true,
        .TonerSaving = true,
        .MarginLeft = 0x01fe,
        .MarginTop = 0x02ec,
        .ImageLineSize = 0x09ab,
        .ImageLines = 0x1b64,
        .PaperWidth = 0x5fef,
        .PaperHeight = 0xfc90,
    });

    ExpectPacket(0xD0A0, {
        0, 0, // const uint16
        0xfc, 0x03, // TargetModel
        7, // PaperSize
        1, // const byte
        1, // InputSlot
        0, // const byte
        0x1f, // Color1
        0x1f, // Color2
        0x1f, // Color3
        0x1f, // Color4
        123, // Mode
        0x11, // Resolution
        3, // const
        1, // const
        1, // const
        1, // const
        2, // SmoothEnable
        1, // TonerSaving
        0, // const
        0, // const
        0xfe, 0x01, // MarginLeft
        0xec, 0x02, // MarginTop
        0xab, 0x09, // ImageLineSize
        0x64, 0x1b, // ImageLines
        0xef, 0x5f, // PaperWidth
        0x90, 0xfc, // PaperHeight
    });

    Protocol::IC_BEGIN_PAGE(stream, PageParams{
        .PaperSize = 7,
        .TonerDensity = 0x1f,
        .Mode = 123,
        .Resolution = ResolutionIdx::RES_300,
        .SmoothEnable = false,
        .TonerSaving = false,
        .MarginLeft = 0x01fe,
        .MarginTop = 0x02ec,
        .ImageLineSize = 0x09ab,
        .ImageLines = 0x1b64,
        .PaperWidth = 0x5fef,
        .PaperHeight = 0xfc90,
    });

    ExpectPacket(0xD0A0, {
        0, 0, // const uint16
        0xfc, 0x03, // TargetModel
        7, // PaperSize
        1, // const byte
        1, // InputSlot
        0, // const byte
        0x1f, // Color1
        0x1f, // Color2
        0x1f, // Color3
        0x1f, // Color4
        123, // Mode
        0, // Resolution
        3, // const
        1, // const
        1, // const
        1, // const
        0, // SmoothEnable
        0, // TonerSaving
        0, // const
        0, // const
        0xfe, 0x01, // MarginLeft
        0xec, 0x02, // MarginTop
        0xab, 0x09, // ImageLineSize
        0x64, 0x1b, // ImageLines
        0xef, 0x5f, // PaperWidth
        0x90, 0xfc, // PaperHeight
    });
}

TEST_F(ProtocolTest, IC_BEGIN_DATA) {
    Protocol::IC_BEGIN_DATA(stream);
    ExpectPacket(0xD0A1);
}

TEST_F(ProtocolTest, IC_END_PAGE) {
    Protocol::IC_END_PAGE(stream);
    ExpectPacket(0xD0A2);
}

TEST_F(ProtocolTest, IC_VIDEO_DATA) {
    std::vector<uint8_t> buffer = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    Protocol::IC_VIDEO_DATA(stream, buffer);
    ExpectPacket(0xC0A0, buffer);
}

TEST_F(ProtocolTest, IC_VIDEO_DATA_Overflow) {
    EXPECT_THROW({
        std::vector<uint8_t> buffer(65535, 0xff);
        Protocol::IC_VIDEO_DATA(stream, buffer);
    }, std::overflow_error);
}

TEST_F(ProtocolTest, PC_GET_EXTENDED_STATUS) {
    WritePacket(0xA0A0, {
        0x1f, // Basic
        0x12,
        0x2e, // Aux
        0x3d, // Controller
        0xca, // PaperAvailableBits
        0x34,
        0x4c, 0xfe, // Engine
        0x12, 0x34, // Start
        0x23, 0x45, // Printing
        0x34, 0x56, // Shipped
        0x45, 0x67, // Printed
        0, 0,
    });
    ExtendedStatus status = Protocol::PC_GET_EXTENDED_STATUS(stream);
    ExpectPacket(0xA0A0);
    EXPECT_EQ(status.Basic, 0x1f);
    EXPECT_EQ(status.Aux, 0x2e);
    EXPECT_EQ(status.Controller, 0x3d);
    EXPECT_EQ(status.PaperAvailableBits, 0xca);
    EXPECT_EQ(status.Engine, 0xfe4c);
    EXPECT_EQ(status.Start, 0x3412);
    EXPECT_EQ(status.Printing, 0x4523);
    EXPECT_EQ(status.Shipped, 0x5634);
    EXPECT_EQ(status.Printed, 0x6745);
}

TEST_F(ProtocolTest, PC_GET_EXTENDED_STATUS_ErrorBit) {
    EXPECT_THROW({
        WritePacket(0xA0A0, {
            0x8f, // Basic
            0x12,
            0x2e, // Aux
            0x3d, // Controller
            0xca, // PaperAvailableBits
            0x34,
            0x4c, 0xfe, // Engine
            0x12, 0x34, // Start
            0x23, 0x45, // Printing
            0x34, 0x56, // Shipped
            0x45, 0x67, // Printed
            0, 0,
        });
        Protocol::PC_GET_EXTENDED_STATUS(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PC_GET_EXTENDED_STATUS_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {
            0x1f, // Basic
            0x12,
            0x2e, // Aux
            0x3d, // Controller
            0x4c, 0xfe, // Engine
            0x12, 0x34, // Start
            0x23, 0x45, // Printing
            0x34, 0x56, // Shipped
            0x45, 0x67, // Printed
            0, 0, 0, 0,
        });
        Protocol::PC_GET_EXTENDED_STATUS(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PC_GET_PRINTER_INFO) {
    WritePacket(0xA1A1, {
        0x00, 0x03,
        0xe3, // DevId
        0x00, // Type
        0x01, // VersionMajor
        0x02, // VersionMinor
        0xfe, 0x7f, // BlockSize
        0x40, 0x10, // Buffers
        0x10,
        0x01,
        0xff,
        0xff,
        0x02,
        0x00,
    });
    PrinterInfo info = Protocol::PC_GET_PRINTER_INFO(stream);
    ExpectPacket(0xA1A1);
    EXPECT_EQ(info.DeviceId, 0xe3);
    EXPECT_EQ(info.Type, 0);
    EXPECT_EQ(info.VersionMajor, 1);
    EXPECT_EQ(info.VersionMinor, 2);
    EXPECT_EQ(info.BlockSize, 0x7ffe);
    EXPECT_EQ(info.Buffers, 0x1040);
}

TEST_F(ProtocolTest, PC_GET_PRINTER_INFO_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {
            0x00, 0x03,
            0xe3, // DevId
            0x00, // Type
            0x01, // VersionMajor
            0x02, // VersionMinor
            0xfe, 0x7f, // BlockSize
            0x40, 0x10, // Buffers
            0x10,
            0x01,
            0xff,
            0xff,
            0x02,
            0x00,
        });
        Protocol::PC_GET_PRINTER_INFO(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_GET_BASIC_STATUS) {
    WritePacket(0xE0A0, {0x11, 0x22});
    uint8_t changed = 0xff;
    EXPECT_EQ(Protocol::PCR_GET_BASIC_STATUS(stream, &changed), 0x11);
    EXPECT_EQ(changed, 0x22);
    ExpectPacket(0xE0A0);
}

TEST_F(ProtocolTest, PCR_GET_BASIC_STATUS_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0x11, 0x22});
        Protocol::PCR_GET_BASIC_STATUS(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_GO_ONLINE) {
    WritePacket(0xE0A5, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_GO_ONLINE(stream, 0x1234), 0x11);
    ExpectPacket(0xE0A5, {
        0xee, 0xdb, 0xea, 0xad, // const uint32
        0x34, 0x12, // page
        0, // const byte
        0, // const byte
    });
}

TEST_F(ProtocolTest, PCR_GO_ONLINE_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_GO_ONLINE(stream, 0x1234);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_CLEANING) {
    WritePacket(0xE0AD, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_CLEANING(stream), 0x11);
    ExpectPacket(0xE0AD, {
        0xee, 0xdb, 0xea, 0xad, // const uint32
        1, 0, // const uint16
        0, // const byte
        0, // const byte
    });
}

TEST_F(ProtocolTest, PCR_CLEANING_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_CLEANING(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PC_RESERVE_UNIT) {
    WritePacket(0xA2A0, {0x11, 0x22});
    EXPECT_EQ(Protocol::PC_RESERVE_UNIT(stream), 0x11);
    ExpectPacket(0xA2A0);
}

TEST_F(ProtocolTest, PC_RESERVE_UNIT_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PC_RESERVE_UNIT(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_DISCARD_DATA) {
    WritePacket(0xE0A4, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_DISCARD_DATA(stream), 0x11);
    ExpectPacket(0xE0A4);
}

TEST_F(ProtocolTest, PCR_DISCARD_DATA_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_DISCARD_DATA(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_CLEAR_ERROR) {
    WritePacket(0xE0A2, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_CLEAR_ERROR(stream), 0x11);
    ExpectPacket(0xE0A2);
}

TEST_F(ProtocolTest, PCR_CLEAR_ERROR_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_CLEAR_ERROR(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_GO_OFFLINE) {
    WritePacket(0xE0A6, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_GO_OFFLINE(stream), 0x11);
    ExpectPacket(0xE0A6);
}

TEST_F(ProtocolTest, PCR_GO_OFFLINE_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_GO_OFFLINE(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_RELEASE_UNIT) {
    WritePacket(0xE0A9, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_RELEASE_UNIT(stream), 0x11);
    ExpectPacket(0xE0A9);
}

TEST_F(ProtocolTest, PCR_RELEASE_UNIT_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_RELEASE_UNIT(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_CLEAR_MISPRINT) {
    WritePacket(0xE0A3, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_CLEAR_MISPRINT(stream), 0x11);
    ExpectPacket(0xE0A3);
}

TEST_F(ProtocolTest, PCR_CLEAR_MISPRINT_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_CLEAR_MISPRINT(stream);
    }, ProtocolError);
}

TEST_F(ProtocolTest, PCR_RESET_ENGINE) {
    WritePacket(0xE0A1, {0x11, 0x22});
    EXPECT_EQ(Protocol::PCR_RESET_ENGINE(stream), 0x11);
    ExpectPacket(0xE0A1);
}

TEST_F(ProtocolTest, PCR_RESET_ENGINE_UnexpectedOpcodeResponse) {
    EXPECT_THROW({
        WritePacket(0x1234, {0, 0});
        Protocol::PCR_RESET_ENGINE(stream);
    }, ProtocolError);
}
