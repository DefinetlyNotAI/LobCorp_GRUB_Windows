cmake_minimum_required(VERSION 3.25)

project(TrumpetOverlay VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
        trumpet/main.cpp
        trumpet/resources/resources.rc
)

# Add executable and rename output to Trumpet.exe
add_executable(Trumpet ${SOURCES})

# Set output directory for the executable to the build folder
set_target_properties(Trumpet PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build"
)

# Tell g++ to use Windows subsystem and Unicode wWinMain
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Trumpet PRIVATE -municode -mwindows)
    target_link_options(Trumpet PRIVATE -municode -mwindows)
endif()

# Link libraries explicitly for g++
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Trumpet PRIVATE
            gdiplus
            winmm
            user32
            gdi32
            ole32
    )
endif()
