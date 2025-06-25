CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(Tests)

# DEPENDENCY_INCLUDES is no longer needed as common_utils target will provide includes.

SET(DEPENDENCY_LIBS
    common_utils::common_utils # Link against our interface library
    GTest::gtest
    GTest::gtest_main
)

FILE(GLOB CURRENT_DIR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

MESSAGE(STATUS "APP SOURCES: ${CURRENT_DIR_SRCS}")

# The PROJECT_OUTPUT_DIR variable is set in the root CMakeLists.txt
# It directs output to ${PROJECT_BINARY_DIR}/${TARGET_ARCH}
# We can keep the test executables in a subdirectory of that.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin/tests)
FILE(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) # Ensure the directory exists

ADD_EXECUTABLE(main ${CURRENT_DIR_SRCS})
# No need for target_include_directories for common_utils, it's handled by target_link_libraries.
# If there were other specific include directories for tests, they would go here.
# No need for target_include_directories for common_utils, it's handled by target_link_libraries.
# If there were other specific include directories for tests, they would go here.
TARGET_LINK_LIBRARIES(main PRIVATE ${DEPENDENCY_LIBS})

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(main)

INSTALL(TARGETS main DESTINATION tests) # Installs test executable to <prefix>/tests
