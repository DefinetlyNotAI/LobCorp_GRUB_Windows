if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "AssertFileNonZero.cmake requires INPUT_FILE")
endif()

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "Required file missing: ${INPUT_FILE}")
endif()

file(SIZE "${INPUT_FILE}" _size)
if(_size EQUAL 0)
    message(FATAL_ERROR "File is empty: ${INPUT_FILE}")
endif()

message(STATUS "Verified non-empty file: ${INPUT_FILE} (${_size} bytes)")

