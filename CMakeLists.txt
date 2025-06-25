CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)

PROJECT(CppCommonUtils VERSION 1.0.0 LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 20)

OPTION(BUILD_TESTS "Build with tests" OFF)

MESSAGE(INFO "--------------------------------")
MESSAGE(STATUS "Build CppCommonUtils: ${AI_WORKFLOW_VERSION}")
MESSAGE(STATUS "Build with tests: ${BUILD_TESTS}")
MESSAGE(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

INCLUDE(GNUInstallDirs) # For standard installation directory variables

IF(MSVC)
    ADD_COMPILE_OPTIONS(/utf-8)
ENDIF()

IF(CMAKE_TOOLCHAIN_FILE)
    MESSAGE(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
ENDIF()

# Set default build to debug
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose Release, Debug, RelWithDebInfo or MinSizeRel" FORCE)
ENDIF()

IF(NOT TARGET_OS)
    SET(TARGET_OS ${CMAKE_SYSTEM_NAME})
ENDIF()

IF(NOT TARGET_ARCH)
    SET(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
ENDIF()


MESSAGE(STATUS "Your target os : ${TARGET_OS}")
MESSAGE(STATUS "Your target arch : ${TARGET_ARCH}")

MESSAGE(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

MESSAGE(INFO "--------------------------------")
# Set output directories for build artifacts
SET(PROJECT_OUTPUT_DIR ${PROJECT_BINARY_DIR}/${TARGET_ARCH}) # Custom output directory
FILE(MAKE_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin) # Ensure bin directory exists if other executables are defined here

MESSAGE("-- system arch:  ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE("-- output path:  ${PROJECT_OUTPUT_DIR}")

# Standard CMake variables for output locations.
# For a library, these are less critical if not building shared/static libs here,
# but good practice if other binaries/libraries are added to the root project.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build 3rdparty
MESSAGE(INFO "3rdparty libraries has loaded.")
MESSAGE(INFO "--------------------------------")
ADD_SUBDIRECTORY(3rdparty)

# Build application
MESSAGE(INFO "--------------------------------")
ADD_SUBDIRECTORY(src)

IF(BUILD_TESTS)
    enable_testing() # Enable testing framework
    MESSAGE(INFO "--------------------------------")
    ADD_SUBDIRECTORY(tests)
ENDIF()

# Installation of CMake package config files
# This allows other CMake projects to find and use this library via find_package(common_utils)
install(EXPORT common_utils_targets
    FILE
        common_utils-config.cmake
    NAMESPACE
        common_utils::
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/common_utils
)
