#include "Protocol.hpp"
#include "Core/PacketHeader.hpp"
#include "Core/PacketBuilder.hpp"
#include "Enums.hpp"
#include "ExtendedStatus.hpp"
#include "Core/StreamPacket.hpp"
#include "ProtocolError.hpp"
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace Capt::Protocol {
    static inline void checkOpcode(uint16_t actual, uint16_t expected) {
        if (actual != expected) {
            std::ostringstream ss;
            ss << "unexpected response opcode: 0x";
            ss << std::hex << std::setfill('0') << std::uppercase << std::setw(4) << actual << std::nouppercase;
            ss << " (expected 0x" << std::uppercase << std::setw(4) << expected << ')';
            throw ProtocolError(ss.str());
        }
    }

    void IC_BEGIN_PAGE(std::ostream& stream, const PageParams& params) {
        PacketBuilder()
            .AppendUint16(0) // const uint16
            .AppendUint16(0x03fc) // TargetModel
            .AppendByte(params.PaperSize)
            .AppendByte(1) // const zero in captfilter, captmon makes it |= 1
            .AppendByte(1) // InputSlot: const zero in captfilter, const 1 in captmon
            .AppendByte(0) // const byte
            .AppendByte(params.TonerDensity)
            .AppendByte(params.TonerDensity)
            .AppendByte(params.TonerDensity)
            .AppendByte(params.TonerDensity)
            .AppendByte(params.Mode)
            .AppendByte(params.Resolution)
            .AppendByte(3) // const byte
            .AppendByte(1) // const byte
            .AppendByte(1) // const byte
            .AppendByte(1) // const byte
            .AppendByte(params.SmoothEnable ? 2 : 0)
            .AppendByte(params.TonerSaving ? 1 : 0)
            .AppendByte(0) // const byte
            .AppendByte(0) // const byte
            .AppendUint16(params.MarginLeft)
            .AppendUint16(params.MarginTop)
            .AppendUint16(params.ImageLineSize)
            .AppendUint16(params.ImageLines)
            .AppendUint16(params.PaperWidth)
            .AppendUint16(params.PaperHeight)
            .WriteTo(stream, 0xD0A0) << std::flush;
    }

    void IC_BEGIN_DATA(std::ostream& stream) {
        PacketHeader::WriteTo(stream, 0xD0A1) << std::flush;
    }

    void IC_END_PAGE(std::ostream& stream) {
        PacketHeader::WriteTo(stream, 0xD0A2) << std::flush;
    }

    void IC_VIDEO_DATA(std::ostream& stream, std::span<const uint8_t> data) {
        PacketHeader::WriteTo(stream, 0xC0A0, data) << std::flush;
    }

    ExtendedStatus PC_GET_EXTENDED_STATUS(std::iostream& stream) {
        PacketHeader::WriteTo(stream, 0xA0A0) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, 0xA0A0);

        ExtendedStatus result;
        result.Basic = static_cast<BasicStatus>(packet.ReadByte());
        if ((result.Basic & BasicStatus::ERROR_BIT) != 0) {
            std::ostringstream ss;
            ss << "PC_GET_EXTENDED_STATUS returned error: 0x";
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(result.Basic);
            throw ProtocolError(ss.str());
        }
        result.Changed = packet.ReadByte();
        result.Aux = static_cast<AuxStatus>(packet.ReadByte());
        result.Controller = static_cast<ControllerStatus>(packet.ReadByte());
        result.PaperAvailableBits = packet.ReadByte();
        packet.ReadByte(); // param_1 + 0x27d
        result.Engine = static_cast<EngineReadyStatus>(packet.ReadUint16());
        result.Start = packet.ReadUint16();
        result.Printing = packet.ReadUint16();
        result.Shipped = packet.ReadUint16();
        result.Printed = packet.ReadUint16();
        return result;
    }

    PrinterInfo PC_GET_PRINTER_INFO(std::iostream& stream) {
        PacketHeader::WriteTo(stream, 0xA1A1) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, 0xA1A1);

        PrinterInfo result;
        packet.ReadByte(); // local_15
        packet.ReadByte(); // local_16
        result.DeviceId = packet.ReadByte();
        result.Type = packet.ReadByte();
        result.VersionMajor = packet.ReadByte();
        result.VersionMinor = packet.ReadByte();
        result.BlockSize = packet.ReadUint16();
        result.Buffers = packet.ReadUint16();
        // other unknown fields
        return result;
    }

    BasicStatus PCR_GET_BASIC_STATUS(std::iostream& stream, uint8_t* changed) {
        PacketHeader::WriteTo(stream, 0xE0A0) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, 0xE0A0);
        BasicStatus status = static_cast<BasicStatus>(packet.ReadByte());
        if (changed != nullptr) {
            *changed = packet.ReadByte();
        }
        return status;
    }

    uint8_t PCR_GO_ONLINE(std::iostream& stream, uint16_t pageNumber) {
        PacketBuilder()
            .AppendUint32(0xadeadbee)
            .AppendUint16(pageNumber)
            .AppendByte(0) // const byte
            .AppendByte(0) // const byte
            .WriteTo(stream, 0xE0A5) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, 0xE0A5);
        uint8_t err = packet.ReadByte();
        return err;
    }

    uint8_t PCR_CLEANING(std::iostream& stream) {
        PacketBuilder()
            .AppendUint32(0xadeadbee)
            .AppendUint16(1) // const byte
            .AppendByte(0) // const byte
            .AppendByte(0) // const byte
            .WriteTo(stream, 0xE0AD) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, 0xE0AD);
        uint8_t err = packet.ReadByte();
        // there is one more byte ignored by the original software
        return err;
    }

    static uint8_t execCmd(std::iostream& stream, uint16_t opcode) {
        PacketHeader::WriteTo(stream, opcode) << std::flush;

        StreamPacket packet;
        stream >> packet;
        checkOpcode(packet.Header.Opcode, opcode);
        uint8_t err = packet.ReadByte();
        return err;
    }

    uint8_t PC_RESERVE_UNIT(std::iostream& stream) { return execCmd(stream, 0xA2A0); }
    uint8_t PCR_DISCARD_DATA(std::iostream& stream) { return execCmd(stream, 0xE0A4); }
    uint8_t PCR_CLEAR_ERROR(std::iostream& stream) { return execCmd(stream, 0xE0A2); }
    uint8_t PCR_GO_OFFLINE(std::iostream& stream) { return execCmd(stream, 0xE0A6); }
    uint8_t PCR_RELEASE_UNIT(std::iostream& stream) { return execCmd(stream, 0xE0A9); }
    uint8_t PCR_CLEAR_MISPRINT(std::iostream& stream) { return execCmd(stream, 0xE0A3); }
    uint8_t PCR_RESET_ENGINE(std::iostream& stream) { return execCmd(stream, 0xE0A1); }
}
