#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace Capt {
    class PacketReader {
    private:
        std::span<const uint8_t> payload;
        std::span<const uint8_t>::iterator iter;
    public:
        explicit PacketReader(std::span<const uint8_t> payload) noexcept;

        uint8_t ReadByte();
        uint16_t ReadUint16();
        uint32_t ReadUint32();
        std::span<const uint8_t> ReadBytes(std::size_t count);
    };
}
