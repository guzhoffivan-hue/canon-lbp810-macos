#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

namespace Capt {
    struct PacketHeader {
        uint16_t Opcode = 0;
        uint16_t PayloadSize = 0;

        constexpr PacketHeader() noexcept = default;
        constexpr explicit PacketHeader(uint16_t opcode, uint16_t payloadSize) noexcept
            : Opcode(opcode), PayloadSize(payloadSize) {}

        [[nodiscard]] constexpr std::size_t Size() const noexcept {
            return this->PayloadSize + 4;
        }

        static std::ostream& WriteTo(std::ostream& stream, uint16_t opcode, std::span<const uint8_t> payload = {});
    };

    std::istream& operator>>(std::istream& stream, PacketHeader& header);
}
