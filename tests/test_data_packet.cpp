#include "gtest/gtest.h"
#include "data_packet.hpp"
#include <string>
#include <stdexcept>

TEST(DataPacketTest, ConstructorAndId) {
    common_utils::DataPacket packet;
    packet.id = 123;
    EXPECT_EQ(packet.id, 123);
}

TEST(DataPacketTest, SetAndGetParam) {
    common_utils::DataPacket packet;
    packet.setParam<int>("int_param", 42);
    packet.setParam<std::string>("string_param", "hello");
    packet.setParam<double>("double_param", 3.14);

    EXPECT_EQ(packet.getParam<int>("int_param"), 42);
    EXPECT_EQ(packet.getParam<std::string>("string_param"), "hello");
    EXPECT_DOUBLE_EQ(packet.getParam<double>("double_param"), 3.14);
}

TEST(DataPacketTest, GetParamMissing) {
    common_utils::DataPacket packet;
    EXPECT_THROW(packet.getParam<int>("non_existent_param"), std::runtime_error);
}

TEST(DataPacketTest, GetParamWrongType) {
    common_utils::DataPacket packet;
    packet.setParam<int>("int_param", 42);
    EXPECT_THROW(packet.getParam<std::string>("int_param"), std::runtime_error);
}

TEST(DataPacketTest, GetOptionalParamPresent) {
    common_utils::DataPacket packet;
    packet.setParam<std::string>("opt_param", "optional_value");
    std::optional<std::string> val = packet.getOptionalParam<std::string>("opt_param");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "optional_value");
}

TEST(DataPacketTest, GetOptionalParamMissing) {
    common_utils::DataPacket packet;
    std::optional<int> val = packet.getOptionalParam<int>("missing_opt_param");
    EXPECT_FALSE(val.has_value());
}

TEST(DataPacketTest, GetOptionalParamWrongType) {
    common_utils::DataPacket packet;
    packet.setParam<int>("opt_int_param", 123);
    EXPECT_THROW(packet.getOptionalParam<std::string>("opt_int_param"), std::runtime_error);
}

TEST(DataPacketTest, OverwriteParam) {
    common_utils::DataPacket packet;
    packet.setParam<int>("value", 100);
    EXPECT_EQ(packet.getParam<int>("value"), 100);
    packet.setParam<int>("value", 200);
    EXPECT_EQ(packet.getParam<int>("value"), 200);
    packet.setParam<std::string>("value", "new_string");
    EXPECT_EQ(packet.getParam<std::string>("value"), "new_string");
}
