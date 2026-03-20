if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE OR NOT DEFINED SYMBOL_NAME)
    message(FATAL_ERROR "EmbedFile.cmake requires INPUT_FILE, OUTPUT_FILE, and SYMBOL_NAME")
endif()

if(NOT DEFINED FORCE_RECREATE_ALL)
    set(FORCE_RECREATE_ALL OFF)
endif()

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "Input file for embedding not found: ${INPUT_FILE}")
endif()

if(EXISTS "${OUTPUT_FILE}" AND NOT FORCE_RECREATE_ALL)
    message(STATUS "[Embed] ${SYMBOL_NAME}: output exists, skipping (set TRUMPET_RECREATE_ALL_GENERATED=ON to regenerate)")
    file(TOUCH_NOCREATE "${OUTPUT_FILE}")
    return()
endif()

get_filename_component(_out_dir "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

message(STATUS "[Embed] ${SYMBOL_NAME}: [#---] 25% reading input")

file(READ "${INPUT_FILE}" _hex HEX)
string(LENGTH "${_hex}" _hex_len)

if(_hex_len EQUAL 0)
    set(_bytes "  0x00")
else()
    message(STATUS "[Embed] ${SYMBOL_NAME}: [##--] 50% converting bytes")
    # Fast path for large files: convert all bytes with regex and wrap lines.
    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " _bytes "${_hex}")
    string(REGEX REPLACE "((0x[0-9a-fA-F][0-9a-fA-F], ){16})" "\\1\n  " _bytes "${_bytes}")
    set(_bytes "  ${_bytes}")
endif()

math(EXPR _bin_len "${_hex_len} / 2")

set(_header "#pragma once\n#include <cstddef>\n\nconstexpr unsigned char ${SYMBOL_NAME}[] = {\n${_bytes}\n};\nconstexpr std::size_t ${SYMBOL_NAME}_len = ${_bin_len};\n")

set(_tmp_output "${OUTPUT_FILE}.tmp")
message(STATUS "[Embed] ${SYMBOL_NAME}: [###-] 75% writing header")
file(WRITE "${_tmp_output}" "${_header}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_tmp_output}" "${OUTPUT_FILE}")
file(REMOVE "${_tmp_output}")
message(STATUS "[Embed] ${SYMBOL_NAME}: [####] 100% done")

