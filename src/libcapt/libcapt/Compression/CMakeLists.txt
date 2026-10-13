target_sources(
    libcapt
    PRIVATE
    ScoaCmd.cpp
    ScoaStreambuf.cpp
)

add_subdirectory(Decoder)
