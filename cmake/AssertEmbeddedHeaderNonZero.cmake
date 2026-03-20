if(NOT DEFINED HEADER_PATH OR NOT DEFINED SYMBOL_NAME)
    message(FATAL_ERROR "AssertEmbeddedHeaderNonZero.cmake requires HEADER_PATH and SYMBOL_NAME")
endif()

if(NOT EXISTS "${HEADER_PATH}")
    message(FATAL_ERROR "Embedded header missing: ${HEADER_PATH}")
endif()

file(READ "${HEADER_PATH}" _content)
string(REGEX MATCH "${SYMBOL_NAME}_len[ \t]*=[ \t]*([0-9]+)" _match "${_content}")

if(NOT _match)
    message(FATAL_ERROR "Could not find ${SYMBOL_NAME}_len in ${HEADER_PATH}")
endif()

set(_len "${CMAKE_MATCH_1}")
if(_len STREQUAL "0")
    message(FATAL_ERROR "${HEADER_PATH} has empty embedded data (${SYMBOL_NAME}_len=0)")
endif()

message(STATUS "Verified ${HEADER_PATH}: ${SYMBOL_NAME}_len=${_len}")

