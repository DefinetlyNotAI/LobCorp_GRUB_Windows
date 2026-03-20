cmake_minimum_required(VERSION 3.25)

project(TrumpetProject VERSION 1.0 LANGUAGES CXX)
enable_testing()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build settings
set(TRUMPET_BUILD_DIR "${CMAKE_SOURCE_DIR}/build")
set(TRUMPET_EMBED_DIR "${CMAKE_SOURCE_DIR}/trumpet/generated")
set(TRUMPET_ZIP_DIR "${CMAKE_SOURCE_DIR}/cmake/zip")
file(MAKE_DIRECTORY "${TRUMPET_BUILD_DIR}")
file(MAKE_DIRECTORY "${TRUMPET_EMBED_DIR}")
file(MAKE_DIRECTORY "${TRUMPET_ZIP_DIR}")
file(REMOVE "${TRUMPET_EMBED_DIR}/customTrumpets.zip")
file(REMOVE "${TRUMPET_EMBED_DIR}/CustomTrumpetsZip.h")

option(TRUMPET_BUILD_ALL_DEFAULT "Build all targets in order by default (Trumpet -> Uninstaller -> Installer)" OFF)
option(TRUMPET_RECREATE_ALL_GENERATED "Force regeneration of all embedded *.h assets" OFF)

# ----------------------
# Main Trumpet executable
# ----------------------
set(TRUMPET_SOURCES
        trumpet/main.cpp
        trumpet/resources/resources.rc
)

add_executable(Trumpet EXCLUDE_FROM_ALL ${TRUMPET_SOURCES})

# Output directory for main exe
set_target_properties(Trumpet PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TRUMPET_BUILD_DIR}"
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
        trumpet/resources/resources.rc
)

add_executable(TrumpetUninstaller EXCLUDE_FROM_ALL ${UNINSTALLER_SOURCES})

# Output directory for uninstaller exe
set_target_properties(TrumpetUninstaller PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TRUMPET_BUILD_DIR}"
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

# ----------------------
# Installer executable
# ----------------------
set(INSTALLER_SOURCES
        trumpet/installer.cpp
        trumpet/resources/resources.rc
)

set(LICENSE_FILE "${CMAKE_SOURCE_DIR}/LICENSE")
set(README_FILE "${CMAKE_SOURCE_DIR}/docs/Trumpet Info.md")
set(CUSTOM_TRUMPETS_DIR "${CMAKE_SOURCE_DIR}/trumpet/.customTrumpets")

if (NOT EXISTS "${LICENSE_FILE}")
    message(FATAL_ERROR "Required file not found: ${LICENSE_FILE}")
endif()

if (NOT EXISTS "${README_FILE}")
    message(FATAL_ERROR "Required file not found: ${README_FILE}")
endif()

if (NOT EXISTS "${CUSTOM_TRUMPETS_DIR}")
    message(FATAL_ERROR "Required folder not found: ${CUSTOM_TRUMPETS_DIR}")
endif()

find_program(TRUMPET_POWERSHELL_EXECUTABLE NAMES pwsh.exe powershell.exe)
if (TRUMPET_POWERSHELL_EXECUTABLE)
    message(STATUS "PowerShell detected: ${TRUMPET_POWERSHELL_EXECUTABLE}")
else()
    message(STATUS "PowerShell not found in PATH. Installer extraction may fail at runtime.")
endif()

set(TRUMPET_EXE_HEADER "${TRUMPET_EMBED_DIR}/TrumpetExe.h")
set(UNINSTALLER_EXE_HEADER "${TRUMPET_EMBED_DIR}/UninstallerExe.h")
set(LICENSE_HEADER "${TRUMPET_EMBED_DIR}/LicenseTxt.h")
set(README_HEADER "${TRUMPET_EMBED_DIR}/ReadmeMd.h")
set(CUSTOM_TRUMPETS_ZIP "${TRUMPET_ZIP_DIR}/customTrumpets.zip")
set(INSTALLER_ASSETS_RC "${CMAKE_BINARY_DIR}/installer_assets.rc")

# Track files inside the custom media folder so ZIP regeneration is accurate.
file(GLOB_RECURSE CUSTOM_TRUMPETS_FILES CONFIGURE_DEPENDS "${CUSTOM_TRUMPETS_DIR}/*")

add_custom_command(
        OUTPUT "${CUSTOM_TRUMPETS_ZIP}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TRUMPET_ZIP_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar cf "${CUSTOM_TRUMPETS_ZIP}" --format=zip -- .
        DEPENDS ${CUSTOM_TRUMPETS_FILES}
        WORKING_DIRECTORY "${CUSTOM_TRUMPETS_DIR}"
        COMMENT "Packaging ${CUSTOM_TRUMPETS_DIR} -> ${CUSTOM_TRUMPETS_ZIP}"
        VERBATIM
)

file(TO_CMAKE_PATH "${CUSTOM_TRUMPETS_ZIP}" CUSTOM_TRUMPETS_ZIP_CMAKE)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/trumpet/resources/resource_ids.h" RESOURCE_IDS_HEADER_CMAKE)
configure_file(
        "${CMAKE_SOURCE_DIR}/trumpet/resources/installer_assets.rc.in"
        "${INSTALLER_ASSETS_RC}"
        @ONLY
)

add_custom_command(
        OUTPUT "${TRUMPET_EXE_HEADER}"
        COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=$<TARGET_FILE:Trumpet>
        -DOUTPUT_FILE=${TRUMPET_EXE_HEADER}
        -DSYMBOL_NAME=TrumpetExe
        -DFORCE_RECREATE_ALL=${TRUMPET_RECREATE_ALL_GENERATED}
        -P ${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake
        DEPENDS Trumpet
        COMMENT "Embedding Trumpet.exe -> TrumpetExe.h"
        VERBATIM
)

add_custom_command(
        OUTPUT "${UNINSTALLER_EXE_HEADER}"
        COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=$<TARGET_FILE:TrumpetUninstaller>
        -DOUTPUT_FILE=${UNINSTALLER_EXE_HEADER}
        -DSYMBOL_NAME=UninstallerExe
        -DFORCE_RECREATE_ALL=${TRUMPET_RECREATE_ALL_GENERATED}
        -P ${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake
        DEPENDS TrumpetUninstaller
        COMMENT "Embedding Uninstaller.exe -> UninstallerExe.h"
        VERBATIM
)

add_custom_command(
        OUTPUT "${LICENSE_HEADER}"
        COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${LICENSE_FILE}
        -DOUTPUT_FILE=${LICENSE_HEADER}
        -DSYMBOL_NAME=LicenseTxt
        -DFORCE_RECREATE_ALL=${TRUMPET_RECREATE_ALL_GENERATED}
        -P ${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake
        DEPENDS "${LICENSE_FILE}"
        COMMENT "Embedding LICENSE -> LicenseTxt.h"
        VERBATIM
)

add_custom_command(
        OUTPUT "${README_HEADER}"
        COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${README_FILE}
        -DOUTPUT_FILE=${README_HEADER}
        -DSYMBOL_NAME=ReadmeMd
        -DFORCE_RECREATE_ALL=${TRUMPET_RECREATE_ALL_GENERATED}
        -P ${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake
        DEPENDS "${README_FILE}"
        COMMENT "Embedding README.md -> ReadmeMd.h"
        VERBATIM
)

set(TRUMPET_INSTALLER_ASSETS
        "${TRUMPET_EXE_HEADER}"
        "${UNINSTALLER_EXE_HEADER}"
        "${LICENSE_HEADER}"
        "${README_HEADER}"
        "${CUSTOM_TRUMPETS_ZIP}"
)

set_source_files_properties(${TRUMPET_INSTALLER_ASSETS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(Installer EXCLUDE_FROM_ALL ${INSTALLER_SOURCES} "${INSTALLER_ASSETS_RC}" ${TRUMPET_INSTALLER_ASSETS})

set_target_properties(Installer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TRUMPET_BUILD_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TRUMPET_BUILD_DIR}"
)

target_include_directories(Installer PRIVATE "${TRUMPET_EMBED_DIR}")

if (TRUMPET_POWERSHELL_EXECUTABLE)
    file(TO_CMAKE_PATH "${TRUMPET_POWERSHELL_EXECUTABLE}" TRUMPET_POWERSHELL_EXECUTABLE_CMAKE)
    target_compile_definitions(Installer PRIVATE TRUMPET_POWERSHELL_EXE=L"${TRUMPET_POWERSHELL_EXECUTABLE_CMAKE}")
endif()

# Windows subsystem + Unicode for GNU
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Installer PRIVATE -municode -mwindows)
    target_link_options(Installer PRIVATE -municode -mwindows)
endif()

# Link required libraries for installer
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Installer PRIVATE
            user32
            shell32
            advapi32
    )
elseif (MSVC)
    target_link_libraries(Installer PRIVATE
            user32
            shell32
            advapi32
    )
endif()

# Ordered build pipeline: Trumpet -> Uninstaller -> Installer
add_custom_target(trumpet-only DEPENDS Trumpet)
add_custom_target(full-build DEPENDS Installer)
add_dependencies(TrumpetUninstaller Trumpet)

add_custom_target(full-build-fresh
        COMMAND ${CMAKE_COMMAND} -E rm -f
        "${TRUMPET_EXE_HEADER}"
        "${UNINSTALLER_EXE_HEADER}"
        "${LICENSE_HEADER}"
        "${README_HEADER}"
        "${CUSTOM_TRUMPETS_ZIP}"
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target full-build
        USES_TERMINAL
)

if (TRUMPET_BUILD_ALL_DEFAULT)
    add_custom_target(build_all_default ALL DEPENDS full-build)
endif()

# ----------------------
# Autotest (non-interactive smoke checks)
# ----------------------
add_test(NAME artifact_trumpet_exists
        COMMAND ${CMAKE_COMMAND} -E md5sum $<TARGET_FILE:Trumpet>)

add_test(NAME artifact_uninstaller_exists
        COMMAND ${CMAKE_COMMAND} -E md5sum $<TARGET_FILE:TrumpetUninstaller>)

add_test(NAME artifact_installer_exists
        COMMAND ${CMAKE_COMMAND} -E md5sum $<TARGET_FILE:Installer>)

add_test(NAME generated_trumpet_header_nonzero
        COMMAND ${CMAKE_COMMAND}
        -DHEADER_PATH=${TRUMPET_EXE_HEADER}
        -DSYMBOL_NAME=TrumpetExe
        -P ${CMAKE_SOURCE_DIR}/cmake/AssertEmbedLen.cmake)

add_test(NAME generated_uninstaller_header_nonzero
        COMMAND ${CMAKE_COMMAND}
        -DHEADER_PATH=${UNINSTALLER_EXE_HEADER}
        -DSYMBOL_NAME=UninstallerExe
        -P ${CMAKE_SOURCE_DIR}/cmake/AssertEmbedLen.cmake)

add_test(NAME generated_license_header_nonzero
        COMMAND ${CMAKE_COMMAND}
        -DHEADER_PATH=${LICENSE_HEADER}
        -DSYMBOL_NAME=LicenseTxt
        -P ${CMAKE_SOURCE_DIR}/cmake/AssertEmbedLen.cmake)

add_test(NAME generated_readme_header_nonzero
        COMMAND ${CMAKE_COMMAND}
        -DHEADER_PATH=${README_HEADER}
        -DSYMBOL_NAME=ReadmeMd
        -P ${CMAKE_SOURCE_DIR}/cmake/AssertEmbedLen.cmake)

add_test(NAME packaged_custom_zip_nonzero
        COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${CUSTOM_TRUMPETS_ZIP}
        -P ${CMAKE_SOURCE_DIR}/cmake/AssertFileNonZero.cmake)

add_custom_target(basic-test
        COMMAND ${CMAKE_CTEST_COMMAND} -R "artifact_" --output-on-failure
        COMMAND ${CMAKE_CTEST_COMMAND} -R "packaged_custom_zip_nonzero" --output-on-failure
        DEPENDS full-build
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

