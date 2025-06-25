cmake_minimum_required(VERSION 3.10)

add_library(common_utils INTERFACE)
add_library(common_utils::common_utils ALIAS common_utils)

target_include_directories(common_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/common_utils> # Path relative to CMAKE_INSTALL_PREFIX
)

# Installation rules for headers
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common_utils
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(TARGETS common_utils EXPORT common_utils_targets
    # This ensures that the INTERFACE_INCLUDE_DIRECTORIES property,
    # which includes $<INSTALL_INTERFACE:include/common_utils>,
    # is correctly exported for consumers.
)
