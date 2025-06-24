#include "gtest/gtest.h"
#include "param_center.hpp"
#include <string>
#include <vector>
#include <variant>

// Define some example parameter structs
struct ParamsTypeA {
    int id;
    std::string name;
    bool operator==(const ParamsTypeA& other) const {
        return id == other.id && name == other.name;
    }
};

struct ParamsTypeB {
    double value;
    std::vector<int> data;
    bool operator==(const ParamsTypeB& other) const {
        return value == other.value && data == other.data;
    }
};

// Define the variant type for ParamCenter
using MyParamsVariant = std::variant<ParamsTypeA, ParamsTypeB>;

class ParamCenterTest : public ::testing::Test {
protected:
    common_utils::ParamCenter<MyParamsVariant> param_center;
};

TEST_F(ParamCenterTest, SetAndGetParamsTypeA) {
    ParamsTypeA params_a{1, "TestA"};
    param_center.setParams(params_a);

    ParamsTypeA* retrieved_params = param_center.getParams<ParamsTypeA>();
    ASSERT_NE(retrieved_params, nullptr);
    EXPECT_EQ(*retrieved_params, params_a);

    // Try to get the wrong type
    ParamsTypeB* wrong_type_params = param_center.getParams<ParamsTypeB>();
    EXPECT_EQ(wrong_type_params, nullptr);
}

TEST_F(ParamCenterTest, SetAndGetParamsTypeB) {
    ParamsTypeB params_b{3.14, {1, 2, 3}};
    param_center.setParams(params_b);

    ParamsTypeB* retrieved_params = param_center.getParams<ParamsTypeB>();
    ASSERT_NE(retrieved_params, nullptr);
    EXPECT_EQ(*retrieved_params, params_b);

    // Try to get the wrong type
    ParamsTypeA* wrong_type_params = param_center.getParams<ParamsTypeA>();
    EXPECT_EQ(wrong_type_params, nullptr);
}

TEST_F(ParamCenterTest, VisitParamsTypeA) {
    ParamsTypeA params_a{2, "VisitTestA"};
    param_center.setParams(params_a);

    bool visited_correct_type = false;
    param_center.visitParams([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ParamsTypeA>) {
            visited_correct_type = true;
            EXPECT_EQ(arg, params_a);
        } else {
            FAIL() << "Visited wrong type in variant";
        }
    });
    EXPECT_TRUE(visited_correct_type);
}

TEST_F(ParamCenterTest, VisitParamsTypeB) {
    ParamsTypeB params_b{2.71, {4, 5, 6}};
    param_center.setParams(params_b);

    bool visited_correct_type = false;
    param_center.visitParams([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ParamsTypeB>) {
            visited_correct_type = true;
            EXPECT_EQ(arg, params_b);
        } else {
            FAIL() << "Visited wrong type in variant";
        }
    });
    EXPECT_TRUE(visited_correct_type);
}

TEST_F(ParamCenterTest, GetParamsWhenEmpty) {
    // Note: A default-constructed variant holds the first type by default.
    // So, getting ParamsTypeA might not be nullptr if it's the first type.
    // This test checks getting a type that is *not* the default-constructed one.
    if constexpr (std::variant_size_v<MyParamsVariant> > 1) {
         // Assuming ParamsTypeB is not the first type in MyParamsVariant
        ParamsTypeB* params_b = param_center.getParams<ParamsTypeB>();
        EXPECT_EQ(params_b, nullptr);
    } else {
        // If only one type, this test is less meaningful as is,
        // but getParams<CorrectType> should return a valid pointer.
        ParamsTypeA* params_a = param_center.getParams<ParamsTypeA>();
         ASSERT_NE(params_a, nullptr); // Default constructed variant will hold ParamsTypeA
    }
}

TEST_F(ParamCenterTest, OverwriteParams) {
    ParamsTypeA params_a{10, "InitialA"};
    param_center.setParams(params_a);

    ParamsTypeA* retrieved_a = param_center.getParams<ParamsTypeA>();
    ASSERT_NE(retrieved_a, nullptr);
    EXPECT_EQ(*retrieved_a, params_a);

    ParamsTypeB params_b{1.618, {7, 8, 9}};
    param_center.setParams(params_b); // Overwrite with TypeB

    ParamsTypeB* retrieved_b = param_center.getParams<ParamsTypeB>();
    ASSERT_NE(retrieved_b, nullptr);
    EXPECT_EQ(*retrieved_b, params_b);

    // Ensure TypeA is no longer accessible
    retrieved_a = param_center.getParams<ParamsTypeA>();
    EXPECT_EQ(retrieved_a, nullptr);
}
