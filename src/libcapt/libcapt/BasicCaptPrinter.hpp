#pragma once
#include "Protocol/ExtendedStatus.hpp"
#include "Protocol/PageParams.hpp"
#include "Protocol/PrinterInfo.hpp"
#include "Protocol/Protocol.hpp"
#include "UnexpectedBehaviourError.hpp"
#include <concepts>
#include <istream>
#include <chrono>
#include <optional>
#include <thread>
#include <cstddef>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <vector>
#include <cstdint>
#include <span>
#include <string_view>

#define CHECK_RETCODE(EXP) impl::checkRetcode(EXP, #EXP)

namespace impl {
    static inline void checkRetcode(uint8_t cmdResult, std::string_view paramName) {
        if (cmdResult != 0) {
            std::ostringstream ss;
            ss << paramName << " returned non-successfull code (0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(cmdResult) << ')';
            throw Capt::UnexpectedBehaviourError(ss.str());
        }
    }
}

namespace Capt {
    template<typename T>
    concept StopToken = requires(T stopToken) {
        { stopToken.stop_requested() } -> std::same_as<bool>;
    } && std::is_default_constructible_v<T>;

    template<StopToken StopTokenT>
    class BasicCaptPrinter {
    protected:
        std::iostream& stream;
        std::optional<PrinterInfo> cachedInfo;
    public:
        using StopTokenType = StopTokenT;

        explicit BasicCaptPrinter(std::iostream& stream) noexcept : stream(stream) {}
        virtual ~BasicCaptPrinter() noexcept = default;

        virtual ExtendedStatus GetStatus() {
            return Protocol::PC_GET_EXTENDED_STATUS(this->stream);
        }

        virtual PrinterInfo GetPrinterInfo() {
            if (!this->cachedInfo) {
                this->cachedInfo = Protocol::PC_GET_PRINTER_INFO(this->stream);
            }
            return *this->cachedInfo;
        }

        virtual void ReserveUnit() {
            CHECK_RETCODE(Protocol::PC_RESERVE_UNIT(this->stream));
            ExtendedStatus ex = this->GetStatus();
            if (!ex.UnitReserved()) {
                throw UnexpectedBehaviourError("failed to reserve unit");
            }
        }

        virtual void ClearError(const ExtendedStatus* status = nullptr) {
            ExtendedStatus ex = status == nullptr ? this->GetStatus() : *status;
            assert(ex.UnitReserved());
            CHECK_RETCODE(Protocol::PCR_CLEAR_ERROR(this->stream));
            CHECK_RETCODE(Protocol::PCR_DISCARD_DATA(this->stream));
            if (ex.Misprint()) {
                CHECK_RETCODE(Protocol::PCR_CLEAR_MISPRINT(this->stream));
            }
            if ((ex.Controller & ControllerStatus::ENGINE_COMM_ERROR) != 0) {
                CHECK_RETCODE(Protocol::PCR_RESET_ENGINE(this->stream));
            }
        }

        virtual bool GoOnline(unsigned page) {
            CHECK_RETCODE(Protocol::PCR_GO_ONLINE(this->stream, page));
            ExtendedStatus ex = this->GetStatus();
            return ex.Online();
        }

        virtual bool Cleaning() {
            return Protocol::PCR_CLEANING(this->stream) == 0;
        }

        // If blockSize is zero, it will be taken from PrinterInfo
        virtual bool WriteVideoData(StopTokenType stopToken, const PageParams& params, std::streambuf& videoStream, std::size_t blockSize = 0) {
            if (blockSize == 0) {
                blockSize = this->GetPrinterInfo().BlockSize;
            }
            assert((blockSize + 4) <= UINT16_MAX);

            Protocol::IC_BEGIN_PAGE(this->stream, params);
            Protocol::IC_BEGIN_DATA(this->stream);
            std::vector<uint8_t> buffer(blockSize);
            while (true) {
                std::streamsize read = videoStream.sgetn(reinterpret_cast<char*>(buffer.data()), buffer.size());
                if (read <= 0) {
                    break;
                }
                while (!stopToken.stop_requested()) {
                    BasicStatus bs = Protocol::PCR_GET_BASIC_STATUS(this->stream);
                    if ((bs & BasicStatus::NOT_READY) != 0) {
                        return false;
                    }
                    if ((bs & BasicStatus::IM_DATA_BUSY) == 0) {
                        break;
                    }
                }
                if (stopToken.stop_requested()) {
                    return false;
                }
                Protocol::IC_VIDEO_DATA(this->stream, {buffer.data(), static_cast<std::size_t>(read)});
            }
            Protocol::IC_END_PAGE(this->stream);
            return true;
        }
        virtual inline bool WriteVideoData(const PageParams& params, std::streambuf& videoStream, std::size_t blockSize = 0) {
            return this->WriteVideoData({}, params, videoStream, blockSize);
        }

        virtual void GoOffline() {
            CHECK_RETCODE(Protocol::PCR_GO_OFFLINE(this->stream));
            // TODO: delay needed in some cases
            ExtendedStatus ex = this->GetStatus();
            if (ex.Online()) {
                throw UnexpectedBehaviourError("failed to offline");
            }
        }

        virtual void ReleaseUnit() {
            ExtendedStatus ex = this->GetStatus();
            if (!ex.UnitReserved()) {
                return;
            }
            CHECK_RETCODE(Protocol::PCR_RELEASE_UNIT(this->stream));
            ex = this->GetStatus();
            if (ex.UnitReserved()) {
                throw UnexpectedBehaviourError("failed to release unit");
            }
        }

        // nullopt if stop requested
        template<typename TFunc, typename Rep, typename Period>
        std::optional<ExtendedStatus> WaitStatus(StopTokenType stopToken, TFunc func, const std::chrono::duration<Rep, Period>& delay) {
            while (!stopToken.stop_requested()) {
                ExtendedStatus ex = this->GetStatus();
                if (func(ex)) {
                    return ex;
                }
                std::this_thread::sleep_for(delay);
            }
            return std::nullopt;
        }

        template<typename TFunc, typename Rep, typename Period>
        ExtendedStatus WaitStatus(TFunc func, const std::chrono::duration<Rep, Period>& delay) {
            return this->WaitStatus({}, func, delay).value();
        }

        // nullopt if stop requested
        virtual inline std::optional<ExtendedStatus> WaitPrintEnd(StopTokenType stopToken) {
            return this->WaitStatus(stopToken, [](const ExtendedStatus& ex) {
                return !ex.IsPrinting();
            }, std::chrono::seconds(1));
        }

        virtual inline ExtendedStatus WaitPrintEnd() {
            return this->WaitPrintEnd({}).value();
        }
    };
}

#undef CHECK_RETCODE
