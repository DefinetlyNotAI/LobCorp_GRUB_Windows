cmake_minimum_required(VERSION 3.25)

project(TrumpetProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------
# Main Trumpet executable
# ----------------------
set(TRUMPET_SOURCES
        trumpet/main.cpp
        trumpet/resources/resources.rc
)

add_executable(Trumpet ${TRUMPET_SOURCES})

# Output directory for main exe
set_target_properties(Trumpet PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build"
)

# Windows subsystem + Unicode for GNU
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Trumpet PRIVATE -municode -mwindows)
    target_link_options(Trumpet PRIVATE -municode -mwindows)
endif()

# Link required libraries for Trumpet
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Trumpet PRIVATE
            gdiplus
            winmm
            user32
            gdi32
            ole32
    )
elseif (MSVC)
    target_link_libraries(Trumpet PRIVATE
            gdiplus
            winmm
            user32
            gdi32
            ole32
    )
endif()

# ----------------------
# Uninstaller executable
# ----------------------
set(UNINSTALLER_SOURCES
        trumpet/uninstaller.cpp
)

add_executable(TrumpetUninstaller ${UNINSTALLER_SOURCES})

# Output directory for uninstaller exe
set_target_properties(TrumpetUninstaller PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/build"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build"
)

# Windows subsystem + Unicode for GNU
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(TrumpetUninstaller PRIVATE -municode -mwindows)
    target_link_options(TrumpetUninstaller PRIVATE -municode -mwindows)
endif()

# Link required libraries for uninstaller
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(TrumpetUninstaller PRIVATE
            user32
            advapi32
            shell32
    )
elseif (MSVC)
    target_link_libraries(TrumpetUninstaller PRIVATE
            user32
            advapi32
            shell32
    )
endif()