if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE OR NOT DEFINED SYMBOL_NAME)
    message(FATAL_ERROR "EmbedFile.cmake requires INPUT_FILE, OUTPUT_FILE, and SYMBOL_NAME")
endif()

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "Input file for embedding not found: ${INPUT_FILE}")
endif()

get_filename_component(_out_dir "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

file(READ "${INPUT_FILE}" _hex HEX)
string(LENGTH "${_hex}" _hex_len)

set(_bytes "")
set(_line "")
set(_line_count 0)

if(_hex_len GREATER 0)
    math(EXPR _last "${_hex_len} - 2")
    foreach(_i RANGE 0 ${_last} 2)
        string(SUBSTRING "${_hex}" ${_i} 2 _byte)
        if(_line STREQUAL "")
            set(_line "0x${_byte}")
        else()
            set(_line "${_line}, 0x${_byte}")
        endif()

        math(EXPR _line_count "${_line_count} + 1")
        if(_line_count EQUAL 12)
            if(_bytes STREQUAL "")
                set(_bytes "  ${_line}")
            else()
                set(_bytes "${_bytes},\n  ${_line}")
            endif()
            set(_line "")
            set(_line_count 0)
        endif()
    endforeach()
endif()

if(NOT _line STREQUAL "")
    if(_bytes STREQUAL "")
        set(_bytes "  ${_line}")
    else()
        set(_bytes "${_bytes},\n  ${_line}")
    endif()
endif()

if(_bytes STREQUAL "")
    set(_bytes "  0x00")
endif()

math(EXPR _bin_len "${_hex_len} / 2")

set(_header "#pragma once\n#include <cstddef>\n\nconst unsigned char ${SYMBOL_NAME}[] = {\n${_bytes}\n};\nconst std::size_t ${SYMBOL_NAME}_len = ${_bin_len};\n")

file(WRITE "${OUTPUT_FILE}" "${_header}")

