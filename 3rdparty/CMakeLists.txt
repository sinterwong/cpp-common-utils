CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(CppCommonUtils3rdparty)

IF(NOT TARGET gtest)
    MESSAGE(STATUS "gtest target not found. Adding subdirectory...")
    ADD_SUBDIRECTORY(googletest EXCLUDE_FROM_ALL)
ELSE()
    MESSAGE(STATUS "gtest target already exists. Skipping.")
ENDIF()
