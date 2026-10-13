#include "PacketReader.hpp"
#include <cstddef>
#include <iterator>
#include <stdexcept>

namespace Capt {
    PacketReader::PacketReader(std::span<const uint8_t> payload) noexcept : payload(payload), iter(payload.begin()) {}

    uint8_t PacketReader::ReadByte() {
        if (this->iter == this->payload.end()) {
            throw std::out_of_range("packet payload EOF");
        }
        uint8_t value = *this->iter;
        ++this->iter;
        return value;
    }

    uint16_t PacketReader::ReadUint16() {
        uint16_t lo = this->ReadByte();
        uint16_t hi = this->ReadByte();
        return (hi << 8) | lo;
    }

    uint32_t PacketReader::ReadUint32() {
        uint32_t value = 0;
        value |= this->ReadByte();
        value |= this->ReadByte() << 8;
        value |= this->ReadByte() << 16;
        value |= this->ReadByte() << 24;
        return value;
    }

    std::span<const uint8_t> PacketReader::ReadBytes(std::size_t count) {
        auto dist = std::distance(this->iter, this->payload.end());
        if (dist < 0 || static_cast<std::size_t>(dist) < count) {
            throw std::out_of_range("packet payload EOF");
        }
        this->iter += count;
        return {this->iter - count, this->iter};
    }
}
