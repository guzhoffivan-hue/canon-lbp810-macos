#pragma once
#include <cstdint>

namespace Capt {
    struct PrinterInfo {
        uint8_t DeviceId;
        uint8_t Type;
        uint8_t VersionMajor;
        uint8_t VersionMinor;
        uint16_t BlockSize;
        uint16_t Buffers;
    };
}
