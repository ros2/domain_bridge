// Copyright 2021, Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "rclcpp/context.hpp"
#include "rclcpp/executors/single_threaded_executor.hpp"

#include "domain_bridge/domain_bridge.hpp"

class TestDomainBridge : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestCase()
  {
    rclcpp::shutdown();
  }
};

TEST_F(TestDomainBridge, construction_destruction)
{
  // Default
  {
    domain_bridge::DomainBridge bridge;
  }
  // With options
  {
    domain_bridge::DomainBridge bridge(domain_bridge::DomainBridgeOptions());
  }
}

TEST_F(TestDomainBridge, get_options)
{
  domain_bridge::DomainBridgeOptions input_options;
  input_options.name("domain_bridge_test_name");
  domain_bridge::DomainBridge bridge(input_options);
  domain_bridge::DomainBridgeOptions output_options = bridge.get_domain_bridge_options();
  EXPECT_EQ(input_options.name(), output_options.name());
}

TEST_F(TestDomainBridge, bridge_topic_valid)
{
  // Nominal test
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
  }
  // Same domain ID
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 1);
  }
  // Topic with namespace
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo/bar/baz", "test_msgs/msg/BasicTypes", 1, 2);
  }
  // Multiple topics
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
    bridge.bridge_topic("bar", "test_msgs/msg/BasicTypes", 1, 2);
    bridge.bridge_topic("baz", "test_msgs/msg/BasicTypes", 1, 2);
  }
  // Same topic, different domains
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 3);
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 2, 1);
  }
  // Same topic, different types
  // This use-case isn't clearly supported by ROS 2, see https://github.com/ros2/ros2/issues/1095
  // {
  //   domain_bridge::DomainBridge bridge;
  //   bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
  //   bridge.bridge_topic("foo", "test_msgs/msg/Nested", 1, 2);
  //   bridge.bridge_topic("foo", "test_msgs/msg/Constants", 1, 2);
  // }
}

TEST_F(TestDomainBridge, bridge_topic_invalid)
{
  // Invalid topic name
  {
    domain_bridge::DomainBridge bridge;
    EXPECT_THROW(
      bridge.bridge_topic("Not a v@lid topic name!", "test_msgs/msg/BasicTypes", 1, 2),
      rclcpp::exceptions::InvalidTopicNameError);
  }
  // Invalid type name
  {
    domain_bridge::DomainBridge bridge;
    EXPECT_THROW(bridge.bridge_topic("foo", "not a valid message type", 1, 2), std::runtime_error);
  }
  // Same bridge twice
  {
    testing::internal::CaptureStderr();
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
    bridge.bridge_topic("foo", "test_msgs/msg/BasicTypes", 1, 2);
    std::string stderr_output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
      stderr_output,
      ::testing::HasSubstr(
        "Topic 'foo' with type 'test_msgs/msg/BasicTypes' already bridged from "
        "domain 1 to domain 2, ignoring\n"));
  }
}

TEST_F(TestDomainBridge, add_to_executor_valid)
{
  // No nodes
  {
    domain_bridge::DomainBridge bridge;
    rclcpp::executors::SingleThreadedExecutor executor;
    bridge.add_to_executor(executor);
  }
  // With nodes
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic(
      "foo",
      "test_msgs/msg/BasicTypes",
      1,
      2);
    rclcpp::executors::SingleThreadedExecutor executor;
    bridge.add_to_executor(executor);
  }
}

TEST_F(TestDomainBridge, add_to_executor_invalid)
{
  // Add twice
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic(
      "foo",
      "test_msgs/msg/BasicTypes",
      1,
      2);
    rclcpp::executors::SingleThreadedExecutor executor;
    bridge.add_to_executor(executor);
    EXPECT_THROW(bridge.add_to_executor(executor), std::runtime_error);
  }
  // Add to different executors
  {
    domain_bridge::DomainBridge bridge;
    bridge.bridge_topic(
      "foo",
      "test_msgs/msg/BasicTypes",
      1,
      2);
    rclcpp::executors::SingleThreadedExecutor executor1;
    rclcpp::executors::SingleThreadedExecutor executor2;
    bridge.add_to_executor(executor1);
    EXPECT_THROW(bridge.add_to_executor(executor2), std::runtime_error);
  }
}
