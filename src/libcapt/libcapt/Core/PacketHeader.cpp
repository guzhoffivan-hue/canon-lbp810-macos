#include "PacketHeader.hpp"
#include "Internal/Integer.hpp"
#include <stdexcept>

namespace Capt {
    std::ostream& PacketHeader::WriteTo(std::ostream& stream, uint16_t opcode, std::span<const uint8_t> payload) {
        if (!stream.good()) {
            return stream;
        }
        std::size_t size = 4 + payload.size();
        if (size > UINT16_MAX) {
            throw std::overflow_error("packet size overflow");
        }
        Internal::WriteLittle<uint16_t>(stream, opcode);
        Internal::WriteLittle<uint16_t>(stream, size);
        stream.write(reinterpret_cast<const char*>(payload.data()), payload.size());
        return stream;
    }

    std::istream& operator>>(std::istream& stream, PacketHeader& header) {
        header.Opcode = Internal::ReadLittle<uint16_t>(stream);
        uint16_t size = Internal::ReadLittle<uint16_t>(stream);
        if (!stream.good()) {
            return stream;
        }
        if (size < 4) {
            throw std::runtime_error("impossible packet size");
        }
        header.PayloadSize = size - 4;
        return stream;
    }
}
