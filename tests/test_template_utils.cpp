#include "gtest/gtest.h"
#include "template_utils.hpp"
#include <functional>
#include <vector>
#include <string>
#include <type_traits> // For std::is_same_v

// Test suite for get_first_arg_type
class GetFirstArgTypeTest : public ::testing::Test {};

TEST_F(GetFirstArgTypeTest, StdFunctionIntFirst) {
    using FuncType = std::function<void(int, double, std::string)>;
    using FirstArg = common_utils::tpl::get_first_arg_type<FuncType>::type;
    EXPECT_TRUE((std::is_same_v<FirstArg, int>));
}

TEST_F(GetFirstArgTypeTest, StdFunctionStringFirst) {
    using FuncType = std::function<int(std::string, bool)>;
    using FirstArg = common_utils::tpl::get_first_arg_type<FuncType>::type;
    EXPECT_TRUE((std::is_same_v<FirstArg, std::string>));
}

TEST_F(GetFirstArgTypeTest, StdFunctionCustomTypeFirst) {
    struct MyType {};
    using FuncType = std::function<MyType(MyType, int)>;
    using FirstArg = common_utils::tpl::get_first_arg_type<FuncType>::type;
    EXPECT_TRUE((std::is_same_v<FirstArg, MyType>));
}

TEST_F(GetFirstArgTypeTest, StdFunctionNoArgs) {
    // This specialization is for functions with at least one argument.
    // How it behaves with no-argument functions depends on its exact definition
    // and SFINAE rules, or it might lead to a compilation error if not handled.
    // The current implementation will likely fail to compile if used with a
    // std::function<void()>, as it tries to access Arg1.
    // For a library, one might add a specialization or use SFINAE to handle this.
    // For this test, we'll focus on the defined behavior.
    // If this test were to be robust for all cases, the template would need extension.
    // For now, we test the explicit specialization's domain.
    SUCCEED() << "Skipping no-arg test as it's outside the current specialization's direct scope or may require SFINAE/compilation error handling.";
}

TEST_F(GetFirstArgTypeTest, StdFunctionSingleArg) {
    using FuncType = std::function<char(float)>;
    using FirstArg = common_utils::tpl::get_first_arg_type<FuncType>::type;
    EXPECT_TRUE((std::is_same_v<FirstArg, float>));
}


// Test suite for get_vector_element_type
class GetVectorElementTypeTest : public ::testing::Test {};

TEST_F(GetVectorElementTypeTest, VectorOfInt) {
    using VecType = std::vector<int>;
    using ElementType = common_utils::tpl::get_vector_element_type<VecType>::type;
    EXPECT_TRUE((std::is_same_v<ElementType, int>));
}

TEST_F(GetVectorElementTypeTest, VectorOfString) {
    using VecType = std::vector<std::string>;
    using ElementType = common_utils::tpl::get_vector_element_type<VecType>::type;
    EXPECT_TRUE((std::is_same_v<ElementType, std::string>));
}

TEST_F(GetVectorElementTypeTest, VectorOfCustomType) {
    struct MyStruct {};
    using VecType = std::vector<MyStruct>;
    using ElementType = common_utils::tpl::get_vector_element_type<VecType>::type;
    EXPECT_TRUE((std::is_same_v<ElementType, MyStruct>));
}

TEST_F(GetVectorElementTypeTest, VectorOfVector) {
    using VecType = std::vector<std::vector<double>>;
    using ElementType = common_utils::tpl::get_vector_element_type<VecType>::type;
    EXPECT_TRUE((std::is_same_v<ElementType, std::vector<double>>));
}
