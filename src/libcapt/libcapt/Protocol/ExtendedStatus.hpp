#pragma once
#include "Enums.hpp"
#include "ReprintStatus.hpp"
#include <cstdint>
#include <ostream>

namespace Capt {
    struct ExtendedStatus {
        BasicStatus Basic;
        uint8_t Changed;
        AuxStatus Aux;
        ControllerStatus Controller;
        uint8_t PaperAvailableBits;
        EngineReadyStatus Engine;

        uint16_t Start;
        uint16_t Printing;
        uint16_t Shipped;
        uint16_t Printed;

        [[nodiscard]] constexpr bool CheckSlotPaper(int slot) const noexcept {
            return (this->PaperAvailableBits & (0x80 >> (slot & 0x1f))) != 0;
        }

        [[nodiscard]] constexpr bool IsPrinting() const noexcept {
            return (this->Aux & AuxStatus::PAPER_DELIVERY) != 0 || (this->Aux & AuxStatus::SAFE_TIMER) != 0
                || (this->Online() && this->Start != this->Printing)
                || (this->Engine & EngineReadyStatus::CLEANING) != 0;
        }

        [[nodiscard]] constexpr bool Ready() const noexcept {
            return (this->Basic & BasicStatus::NOT_READY) == 0;
        }

        [[nodiscard]] constexpr bool UnitReserved() const noexcept {
            return (this->Basic & BasicStatus::UNIT_FREE) == 0;
        }

        [[nodiscard]] constexpr bool VideoDataError() const noexcept {
            return (this->Controller & ControllerStatus::OVERRUN) != 0
                || (this->Controller & ControllerStatus::UNDERRUN) != 0
                || (this->Controller & ControllerStatus::INVALID_DATA) != 0
                || (this->Controller & ControllerStatus::MISSING_EOP) != 0;
        }

        [[nodiscard]] constexpr bool Rejected() const noexcept {
            return (this->Controller & ControllerStatus::PRINT_REJECTED) != 0;
        }

        [[nodiscard]] constexpr ReprintStatus GetReprintStatus() const noexcept {
            if (!this->Rejected()) {
                return ReprintStatus::None;
            }
            return (this->Printed + 1 != this->Start) ? ReprintStatus::Prev : ReprintStatus::Current;
        }

        [[nodiscard]] constexpr bool Misprint() const noexcept {
            return (this->Engine & EngineReadyStatus::MIS_PRINT) != 0
                || (this->Engine & EngineReadyStatus::MIS_PRINT_2) != 0;
        }

        [[nodiscard]] constexpr bool ClearErrorNeeded() const noexcept {
            return this->VideoDataError()
                || this->Rejected()
                || this->Misprint()
                || (this->Controller & ControllerStatus::ENGINE_COMM_ERROR) != 0;
        }

        [[nodiscard]] constexpr bool Online() const noexcept {
            return (this->Basic & BasicStatus::OFFLINE) == 0;
        }

        [[nodiscard]] constexpr bool ServiceCall() const noexcept {
            return (this->Engine & EngineReadyStatus::SERVICE_CALL) != 0;
        }

        [[nodiscard]] constexpr bool FatalError() const noexcept {
            return (this->Basic & BasicStatus::ERROR_BIT) != 0
                || (this->Basic & BasicStatus::CMD_BUSY) != 0
                || (this->Engine & EngineReadyStatus::SERVICE_CALL) != 0;
        }
    };

    std::ostream& operator<<(std::ostream& os, const ExtendedStatus& status);
}
