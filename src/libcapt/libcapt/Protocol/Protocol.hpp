#pragma once
#include "ExtendedStatus.hpp"
#include "PageParams.hpp"
#include "PrinterInfo.hpp"
#include <iostream>
#include <cstdint>
#include <span>

namespace Capt::Protocol {
    void IC_BEGIN_PAGE(std::ostream& stream, const PageParams& params);
    void IC_BEGIN_DATA(std::ostream& stream);
    void IC_END_PAGE(std::ostream& stream);
    void IC_VIDEO_DATA(std::ostream& stream, std::span<const uint8_t> data);
    ExtendedStatus PC_GET_EXTENDED_STATUS(std::iostream& stream);
    PrinterInfo PC_GET_PRINTER_INFO(std::iostream& stream);
    BasicStatus PCR_GET_BASIC_STATUS(std::iostream& stream, uint8_t* changed = nullptr);
    uint8_t PCR_GO_ONLINE(std::iostream& stream, uint16_t pageNumber);
    uint8_t PCR_CLEANING(std::iostream& stream);
    uint8_t PC_RESERVE_UNIT(std::iostream& stream);
    uint8_t PCR_DISCARD_DATA(std::iostream& stream);
    uint8_t PCR_CLEAR_ERROR(std::iostream& stream);
    uint8_t PCR_GO_OFFLINE(std::iostream& stream);
    uint8_t PCR_RELEASE_UNIT(std::iostream& stream);
    uint8_t PCR_CLEAR_MISPRINT(std::iostream& stream);
    uint8_t PCR_RESET_ENGINE(std::iostream& stream);
}
