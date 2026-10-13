#pragma once
#include <cstdint>

namespace Capt {
    struct PageParams {
        uint8_t PaperSize;
        uint8_t TonerDensity;
        uint8_t Mode;
        uint8_t Resolution;
        bool SmoothEnable;
        bool TonerSaving;
        uint16_t MarginLeft;
        uint16_t MarginTop;
        uint16_t ImageLineSize;
        uint16_t ImageLines;
        uint16_t PaperWidth;
        uint16_t PaperHeight;
    };
}
