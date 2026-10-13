cmake_minimum_required(VERSION 3.21)

project(
    CanonLBP810Driver
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS "Build unit tests" ON)
set(CAPTPPD_BACKEND_NAME "captusb" CACHE STRING "Name of CUPS backend executable")
set(CAPTBACKEND_NAME "${CAPTPPD_BACKEND_NAME}")
set(CAPTPPD_DITHERING_OPT ON)

# Configure version macros
set(PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(LIBCAPT_VERSION_STRING "${PROJECT_VERSION}")
set(CAPTBACKEND_VERSION_STRING "${PROJECT_VERSION}")

# Check stop_token support
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <stop_token>
int main() {
    [[maybe_unused]] std::stop_token token;
    return 0;
}
" HAVE_STOP_TOKEN)
if(NOT HAVE_STOP_TOKEN)
    set(HAVE_STOP_TOKEN 0)
endif()

check_cxx_source_compiles("
#include <bit>
int main() {
    [[maybe_unused]] int a = std::byteswap(123);
    return 0;
}
" HAVE_BYTESWAP)
if(NOT HAVE_BYTESWAP)
    set(HAVE_BYTESWAP 0)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend/Config.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend/Config.hpp"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt/libcapt/Config.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt/libcapt/Config.hpp"
    @ONLY
)

# Find CUPS
find_program(CUPS_CONFIG NAMES cups-config REQUIRED)
function(cups_config_opt VAR OPT)
    execute_process(
        COMMAND "${CUPS_CONFIG}" ${OPT}
        OUTPUT_VARIABLE out
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${VAR} ${out} PARENT_SCOPE)
endfunction()

cups_config_opt(CUPS_SERVER_BIN --serverbin)
cups_config_opt(CUPS_DATA_DIR --datadir)
cups_config_opt(CUPS_CFLAGS --cflags)
cups_config_opt(CUPS_LDFLAGS --ldflags)
cups_config_opt(CUPS_LIBS --libs)

# Find libusb-1.0 (prefer static archive on macOS for a self-contained binary)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

set(STATIC_LIBUSB "/opt/homebrew/lib/libusb-1.0.a")
if(APPLE AND EXISTS "${STATIC_LIBUSB}")
    message(STATUS "Using static libusb: ${STATIC_LIBUSB}")
    set(LIBUSB_LINK_TARGET "${STATIC_LIBUSB}")
    set(EXTRA_SYSTEM_FRAMEWORKS "-framework IOKit -framework CoreFoundation -framework Security")
else()
    message(STATUS "Using dynamic libusb from pkg-config")
    set(LIBUSB_LINK_TARGET ${LIBUSB_LIBRARIES})
    set(EXTRA_SYSTEM_FRAMEWORKS "")
endif()

# Build libcapt static library
set(LIBCAPT_SOURCES
    src/libcapt/libcapt/Compression/ScoaCmd.cpp
    src/libcapt/libcapt/Compression/ScoaStreambuf.cpp
    src/libcapt/libcapt/Compression/Decoder/DecoderStreambuf.cpp
    src/libcapt/libcapt/Core/PacketHeader.cpp
    src/libcapt/libcapt/Core/PacketReader.cpp
    src/libcapt/libcapt/Core/StreamPacket.cpp
    src/libcapt/libcapt/Protocol/ExtendedStatus.cpp
    src/libcapt/libcapt/Protocol/Protocol.cpp
    src/libcapt/libcapt/Utility/BufferedPage.cpp
    src/libcapt/libcapt/Utility/CropStreambuf.cpp
)

add_library(libcapt STATIC ${LIBCAPT_SOURCES})
target_include_directories(libcapt PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt/libcapt"
)

# Build captbackend static helper
set(CAPTBACKEND_LIB_SOURCES
    src/backend/Core/BufferedWriter.cpp
    src/backend/Core/CaptPrinter.cpp
    src/backend/Core/Log.cpp
    src/backend/Core/PrinterInfo.cpp
    src/backend/Core/StateReporter.cpp
    src/backend/Cups/CupsRasterStreambuf.cpp
    src/backend/UsbBackend/UsbBackend.cpp
    src/backend/UsbBackend/UsbPrinter.cpp
    src/backend/UsbBackend/UsbStreambuf.cpp
)

add_library(libcaptusb STATIC ${CAPTBACKEND_LIB_SOURCES})
target_include_directories(libcaptusb PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt"
    ${LIBUSB_INCLUDE_DIRS}
)
target_link_libraries(libcaptusb PUBLIC libcapt ${LIBUSB_LINK_TARGET})

# Build captusb backend executable
add_executable(captusb src/backend/main.cpp)
target_include_directories(captusb PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libcapt"
    ${LIBUSB_INCLUDE_DIRS}
)
target_link_libraries(captusb PRIVATE
    libcaptusb
    libcapt
    ${LIBUSB_LINK_TARGET}
    ${CUPS_LIBS}
    ${CUPS_LDFLAGS}
)
if(EXTRA_SYSTEM_FRAMEWORKS)
    target_link_libraries(captusb PRIVATE ${EXTRA_SYSTEM_FRAMEWORKS})
endif()

# Build USB backend filter to hide non-working Apple USB device in Add Printer dialog
add_executable(usb_filter src/backend/usb_filter.c)

# PPD generation
find_program(PPDC NAMES ppdc REQUIRED)
find_package(Python REQUIRED)

set(PPD_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ppd/LBP800CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/ppd/LBP810CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/ppd/LBP1120CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/ppd/LBP1210CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/ppd/LBP3200CAPTPPD.ppd"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/captmedia.defs"
    COMMAND "${Python_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/genmedia.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/captmedia.defs"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/genmedia.py"
    VERBATIM
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ppd")

add_custom_command(
    OUTPUT ${PPD_FILES}
    COMMAND "${PPDC}"
        -D "CAPTBACKEND_VERSION=${PROJECT_VERSION}"
        -D "HAVE_DITHERING_TYPE=1"
        -d "${CMAKE_CURRENT_BINARY_DIR}/ppd"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/captppd.drv"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/captppd.drv"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ppd/captmedia.defs"
    VERBATIM
)

add_custom_target(ppds ALL DEPENDS ${PPD_FILES})

# Testing
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(CORE_TESTS
        BufferedPageTest
        CropStreambufTest
        PacketBuilderTest
        PacketHeaderTest
        PacketReaderTest
        ProtocolTest
        ScoaStateTest
        ScoaStreambufTest
        StreamPacketTest
    )

    foreach(TEST_NAME ${CORE_TESTS})
        add_executable(${TEST_NAME} "src/libcapt/tests/Core/${TEST_NAME}.cpp")
        target_link_libraries(${TEST_NAME} PRIVATE libcapt GTest::gtest_main GTest::gmock)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()
