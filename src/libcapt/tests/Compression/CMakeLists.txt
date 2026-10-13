find_program(PDFSEPARATE NAMES pdfseparate REQUIRED)
find_program(BASH NAMES bash REQUIRED)

add_executable(compressor compressor.cpp)
target_link_libraries(compressor libcapt)
target_include_directories(compressor PRIVATE $<TARGET_PROPERTY:libcapt,INCLUDE_DIRECTORIES>)

add_executable(decoder decoder.cpp)
target_link_libraries(decoder libcapt)
target_include_directories(decoder PRIVATE $<TARGET_PROPERTY:libcapt,INCLUDE_DIRECTORIES>)

file(GLOB PBM_FILES "${CMAKE_CURRENT_LIST_DIR}/data/*.pbm")
list(LENGTH PBM_FILES PBM_FILES_COUNT)
message(STATUS "Total PBM files count: ${PBM_FILES_COUNT}")

file(GLOB PDF_RAW_FILES "${CMAKE_CURRENT_LIST_DIR}/data/*.pdf")
list(LENGTH PDF_RAW_FILES PDF_RAW_FILES_COUNT)
message(STATUS "Total raw PDF files count: ${PDF_RAW_FILES_COUNT}")

set(DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")
set(PDF_SEP_DIR "${DATA_DIR}/pdf")
file(MAKE_DIRECTORY "${DATA_DIR}")
file(MAKE_DIRECTORY "${PDF_SEP_DIR}")

foreach(pdf ${PDF_RAW_FILES})
    get_filename_component(filename "${pdf}" NAME_WE)
    message(STATUS "Separating ${filename}.pdf to ${PDF_SEP_DIR}")
    execute_process(
        COMMAND "${PDFSEPARATE}" "${pdf}" "${PDF_SEP_DIR}/${filename}-page%04d.pdf"
        COMMAND_ERROR_IS_FATAL ANY
        OUTPUT_QUIET ERROR_QUIET
    )
endforeach()

file(GLOB PDF_FILES "${PDF_SEP_DIR}/*.pdf")
list(LENGTH PDF_FILES PDF_FILES_COUNT)
message(STATUS "Total PDF files count: ${PDF_FILES_COUNT}")

set(PDF_RASTERIZED)
foreach(pdf ${PDF_FILES})
    get_filename_component(filename "${pdf}" NAME_WE)
    set(pbmfile "${PDF_SEP_DIR}/${filename}.pbm")
    add_custom_command(
        OUTPUT "${pbmfile}"
        COMMAND "${PROJECT_SOURCE_DIR}/scripts/pdf2pbm.sh" "${pdf}" "${pbmfile}"
        VERBATIM
    )
    list(APPEND PDF_RASTERIZED "${pbmfile}")
endforeach()
add_custom_target(pdf_rasterize ALL DEPENDS ${PDF_RASTERIZED})

set(FILTER_FILES)
foreach(pbm ${PBM_FILES} ${PDF_RASTERIZED})
    get_filename_component(filename "${pbm}" NAME_WE)
    set(filterlog "${DATA_DIR}/${filename}.log")
    set(filterbin "${DATA_DIR}/${filename}.bin")
    add_custom_command(
        OUTPUT "${filterlog}" "${filterbin}"
        COMMAND "${PROJECT_SOURCE_DIR}/scripts/filter/filter.sh" "${pbm}" "${filterlog}" "${filterbin}"
        COMMENT "Filtering ${filename}.pbm"
        VERBATIM
    )
    list(APPEND FILTER_FILES "${filterlog}" "${filterbin}")

    set(declog "${DATA_DIR}/${filename}-decoded.log")
    set(decpbm "${DATA_DIR}/${filename}-decoded.pbm")
    add_test(
        NAME "DecoderTest.${filename}"
        COMMAND "${BASH}" -c "'$<TARGET_FILE:decoder>' '${filterbin}' '${decpbm}' '${declog}' && cmp '${filterlog}' '${declog}' && cmp '${pbm}' '${decpbm}'"
    )
    set_tests_properties(
        "DecoderTest.${filename}"
        PROPERTIES
        REQUIRED_FILES "$<TARGET_FILE:decoder>;${pbm};${filterlog};${filterbin}"
        TIMEOUT 0
        LABELS "compression;decoder"
    )

    add_test(
        NAME "CompressorTest.${filename}"
        COMMAND "${BASH}" -c "cmp '${pbm}' <('$<TARGET_FILE:decoder>' <('$<TARGET_FILE:compressor>' '${pbm}' /dev/stdout) /dev/stdout /dev/null)"
    )
    set_tests_properties(
        "CompressorTest.${filename}"
        PROPERTIES
        REQUIRED_FILES "$<TARGET_FILE:decoder>;$<TARGET_FILE:compressor>;${pbm}"
        TIMEOUT 0
        LABELS "compression;compressor"
    )
endforeach()
add_custom_target(filter ALL DEPENDS ${FILTER_FILES})
