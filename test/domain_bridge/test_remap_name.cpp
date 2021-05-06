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

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "rclcpp/context.hpp"
#include "rclcpp/node.hpp"
#include "test_msgs/msg/basic_types.hpp"

#include "domain_bridge/domain_bridge.hpp"

#include "wait_for_publisher.hpp"

class TestDomainBridgeRemapName : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    // Initialize contexts in different domains
    context_1_ = std::make_shared<rclcpp::Context>();
    rclcpp::InitOptions context_options_1;
    context_options_1.auto_initialize_logging(false).set_domain_id(domain_1_);
    context_1_->init(0, nullptr, context_options_1);

    context_2_ = std::make_shared<rclcpp::Context>();
    rclcpp::InitOptions context_options_2;
    context_options_2.auto_initialize_logging(false).set_domain_id(domain_2_);
    context_2_->init(0, nullptr, context_options_2);

    node_options_1_.context(context_1_);
    node_options_2_.context(context_2_);
  }

  static const std::size_t domain_1_{1u};
  static const std::size_t domain_2_{2u};
  static std::shared_ptr<rclcpp::Context> context_1_;
  static std::shared_ptr<rclcpp::Context> context_2_;
  static rclcpp::NodeOptions node_options_1_;
  static rclcpp::NodeOptions node_options_2_;
};

const std::size_t TestDomainBridgeRemapName::domain_1_;
const std::size_t TestDomainBridgeRemapName::domain_2_;
std::shared_ptr<rclcpp::Context> TestDomainBridgeRemapName::context_1_;
std::shared_ptr<rclcpp::Context> TestDomainBridgeRemapName::context_2_;
rclcpp::NodeOptions TestDomainBridgeRemapName::node_options_1_;
rclcpp::NodeOptions TestDomainBridgeRemapName::node_options_2_;

TEST_F(TestDomainBridgeRemapName, remap_topic_name)
{
  const std::string topic_name("test_remap_topic_before");
  const std::string remap_name("test_remap_topic_after");

  // Create a publisher on domain 1
  auto node_1 = std::make_shared<rclcpp::Node>("test_remap_topic_node_1", node_options_1_);
  auto pub = node_1->create_publisher<test_msgs::msg::BasicTypes>(topic_name, 1);

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridge bridge;
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.remap_name(remap_name);
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", domain_1_, domain_2_, topic_bridge_options);

  // Wait for bridge publisher to appear on domain 2
  auto node_2 = std::make_shared<rclcpp::Node>("test_remap_topic_node_2", node_options_2_);
  // Expect the remapped topic to appear
  ASSERT_TRUE(wait_for_publisher(node_2, remap_name));
  // Do not expect the original topic name
  ASSERT_FALSE(wait_for_publisher(node_2, topic_name, std::chrono::seconds(1)));
}

TEST_F(TestDomainBridgeRemapName, remap_topic_name_with_substitution)
{
  const std::string topic_name("test_remap_topic_with_sub");
  const std::string remap_name("~/test_remap_topic_with_sub");
  const std::string domain_bridge_name("my_test_name");
  const std::string expected_name("my_test_name/test_remap_topic_with_sub");

  // Create a publisher on domain 1
  auto node_1 = std::make_shared<rclcpp::Node>(
    "test_remap_topic_with_sub_node_1", node_options_1_);
  auto pub = node_1->create_publisher<test_msgs::msg::BasicTypes>(topic_name, 1);

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridgeOptions domain_bridge_options;
  domain_bridge_options.name(domain_bridge_name);
  domain_bridge::DomainBridge bridge(domain_bridge_options);
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.remap_name(remap_name);
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", domain_1_, domain_2_, topic_bridge_options);

  // Wait for bridge publisher to appear on domain 2
  auto node_2 = std::make_shared<rclcpp::Node>(
    "test_remap_topic_with_sub_node_2", node_options_2_);
  // Expect the remapped topic to appear
  ASSERT_TRUE(wait_for_publisher(node_2, expected_name));
  // Do not expect the original topic name
  ASSERT_FALSE(wait_for_publisher(node_2, topic_name, std::chrono::seconds(1)));
}

TEST_F(TestDomainBridgeRemapName, remap_topic_name_invalid)
{
  const std::string topic_name("test_remap_topic_invalid_before");
  const std::string remap_name("test_remap_topic_1nv@lid_after");

  domain_bridge::DomainBridge bridge;
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.remap_name(remap_name);
  EXPECT_THROW(
    bridge.bridge_topic(
      topic_name, "test_msgs/msg/BasicTypes", domain_1_, domain_2_, topic_bridge_options),
    rclcpp::exceptions::InvalidTopicNameError);
}
