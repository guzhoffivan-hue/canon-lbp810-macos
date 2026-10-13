include(CheckCXXSourceCompiles)

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

configure_file(Config.hpp.in "${CMAKE_CURRENT_SOURCE_DIR}/Config.hpp" @ONLY)

add_library(libcapt)
add_library(libcapt::libcapt ALIAS libcapt)
set_target_properties(libcapt PROPERTIES PREFIX "")
target_include_directories(libcapt
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_subdirectory(Compression)
add_subdirectory(Core)
add_subdirectory(Protocol)
add_subdirectory(Utility)

if(LIBCAPT_INSTALL)
    install(
        TARGETS libcapt
        EXPORT libcaptExports
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING
        PATTERN "*.hpp"
    )
endif()
