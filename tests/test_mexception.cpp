#include "gtest/gtest.h"
#include "mexception.hpp"
#include <string>
#include <variant>

class MexceptionCustomExceptionsTest : public ::testing::Test {
protected:
    const std::string test_message = "Test exception message";
};

TEST_F(MexceptionCustomExceptionsTest, InvalidValueException) {
    try {
        throw common_utils::exception::InvalidValueException(test_message);
    } catch (const common_utils::exception::InvalidValueException& e) {
        EXPECT_STREQ(e.what(), ("Invalid value: " + test_message).c_str());
    } catch (...) {
        FAIL() << "Expected InvalidValueException";
    }
}

TEST_F(MexceptionCustomExceptionsTest, OutOfRangeException) {
    try {
        throw common_utils::exception::OutOfRangeException(test_message);
    } catch (const common_utils::exception::OutOfRangeException& e) {
        EXPECT_STREQ(e.what(), ("Out of range: " + test_message).c_str());
    } catch (...) {
        FAIL() << "Expected OutOfRangeException";
    }
}

TEST_F(MexceptionCustomExceptionsTest, NullPointerException) {
    try {
        throw common_utils::exception::NullPointerException(test_message);
    } catch (const common_utils::exception::NullPointerException& e) {
        EXPECT_STREQ(e.what(), ("Null pointer: " + test_message).c_str());
    } catch (...) {
        FAIL() << "Expected NullPointerException";
    }
}

TEST_F(MexceptionCustomExceptionsTest, FileOperationException) {
    try {
        throw common_utils::exception::FileOperationException(test_message);
    } catch (const common_utils::exception::FileOperationException& e) {
        EXPECT_STREQ(e.what(), ("File operation error: " + test_message).c_str());
    } catch (...) {
        FAIL() << "Expected FileOperationException";
    }
}

TEST_F(MexceptionCustomExceptionsTest, NetworkException) {
    try {
        throw common_utils::exception::NetworkException(test_message);
    } catch (const common_utils::exception::NetworkException& e) {
        EXPECT_STREQ(e.what(), ("Network error: " + test_message).c_str());
    } catch (...) {
        FAIL() << "Expected NetworkException";
    }
}

class GetOrThrowTest : public ::testing::Test {};

TEST_F(GetOrThrowTest, GetExistingType) {
    std::variant<int, std::string> v_int = 42;
    std::variant<int, std::string> v_str = "hello";

    EXPECT_EQ(common_utils::exception::get_or_throw<int>(v_int), 42);
    EXPECT_EQ(common_utils::exception::get_or_throw<std::string>(v_str), "hello");
}

TEST_F(GetOrThrowTest, ThrowOnMissingType) {
    std::variant<int, std::string> v_int = 42;
    std::variant<double, char> v_double = 3.14;

    EXPECT_THROW(common_utils::exception::get_or_throw<std::string>(v_int), std::runtime_error);
    EXPECT_THROW(common_utils::exception::get_or_throw<int>(v_double), std::runtime_error);
}
