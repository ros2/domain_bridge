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

class TestBidirectionalDomainBridge : public ::testing::Test
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

const std::size_t TestBidirectionalDomainBridge::domain_1_;
const std::size_t TestBidirectionalDomainBridge::domain_2_;
std::shared_ptr<rclcpp::Context> TestBidirectionalDomainBridge::context_1_;
std::shared_ptr<rclcpp::Context> TestBidirectionalDomainBridge::context_2_;
rclcpp::NodeOptions TestBidirectionalDomainBridge::node_options_1_;
rclcpp::NodeOptions TestBidirectionalDomainBridge::node_options_2_;

TEST_F(TestBidirectionalDomainBridge, create_bidirectional_bridge)
{
  const std::string topic_name("foo");

  // Create a publisher on domain 1
  auto pub_node = std::make_shared<rclcpp::Node>("test_bidirectional_pub_node", node_options_1_);
  auto pub = pub_node->create_publisher<test_msgs::msg::BasicTypes>(topic_name, 1);

  // Bridge the publisher topic bidirectionally
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.bidirectional(true);
  domain_bridge::DomainBridgeConfig config;
  config.topics.push_back(
  {
    {topic_name, "test_msgs/msg/BasicTypes", domain_2_, domain_1_},
    topic_bridge_options
  });
  domain_bridge::DomainBridge bridge(config);

  // 'to' domain is 1, but since we are bridging both ways, publisher should also appear on domain 2
  auto sub_node = std::make_shared<rclcpp::Node>("test_bidirectional_sub_node", node_options_2_);
  ASSERT_TRUE(wait_for_publisher(sub_node, topic_name));
}
