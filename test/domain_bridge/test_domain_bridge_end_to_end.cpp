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
#include <utility>

#include "rclcpp/context.hpp"
#include "rclcpp/executors.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/serialization.hpp"
#include "test_msgs/msg/basic_types.hpp"

#include "domain_bridge/compress_messages.hpp"
#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/msg/compressed_msg.hpp"
#include "domain_bridge/utils.hpp"


static constexpr std::size_t kDomain1{1u};
static constexpr std::size_t kDomain2{2u};

using namespace std::chrono_literals;

// TODO(ivanpauno): We could use parameterized testing here ...
class TestDomainBridgeEndToEnd : public ::testing::Test
{
protected:
  void SetUp() override
  {
    context_1_ = domain_bridge::utils::create_context_with_domain_id(kDomain1);
    node_1_ = domain_bridge::utils::create_node(
      "node_1", kDomain1, context_1_);
    context_2_ = domain_bridge::utils::create_context_with_domain_id(kDomain2);
    node_2_ = domain_bridge::utils::create_node(
      "node_2", kDomain2, context_2_);
  }
  std::shared_ptr<rclcpp::Context> context_1_;
  std::shared_ptr<rclcpp::Context> context_2_;
  std::shared_ptr<rclcpp::Node> node_1_;
  std::shared_ptr<rclcpp::Node> node_2_;
  // use a qos profile with transient local volatility to make the tests simple.
  // it's hard to guarantee that all pub/sub have matched and so the first few messages
  // can be lost if using a profile with volatile durability.
  rclcpp::QoS pub_sub_qos_ = rclcpp::QoS(1).transient_local();
};

static
bool
poll_condition(std::function<bool()> condition, std::chrono::seconds timeout)
{
  auto start = std::chrono::steady_clock::now();
  while (
    !condition() &&
    (start + timeout > std::chrono::steady_clock::now()))
  {
    std::this_thread::sleep_for(50ms);
  }
  return condition();
}

/*
 * Can't call spin_until_future_complete() inside a callback executed by an executor.
 *
 * https://github.com/ros2/rclcpp/blob/d7804e1b3fd9676d302ec72f02c49ba04cbed5e6/rclcpp/include/rclcpp/executor.hpp#L198-L199
 */
class ScopedAsyncSpinner
{
public:
  explicit ScopedAsyncSpinner(std::shared_ptr<rclcpp::Context> context)
  : executor_{get_executor_options_with_context(std::move(context))},
    thread_{[this, stop_token = std::shared_future<void>{promise_.get_future()}] {
        executor_.spin_until_future_complete(stop_token);
      }}
  {}

  ~ScopedAsyncSpinner()
  {
    promise_.set_value();
    // TODO(ivanpauno): Report bug in rclcpp.
    // This shouldn't be needed if spin_until_future_complete() worked
    // correctly.
    executor_.cancel();
    thread_.join();
  }

  rclcpp::Executor &
  get_executor()
  {
    return executor_;
  }

private:
  static
  rclcpp::ExecutorOptions
  get_executor_options_with_context(std::shared_ptr<rclcpp::Context> context)
  {
    rclcpp::ExecutorOptions ret;
    ret.context = std::move(context);
    return ret;
  }

  rclcpp::executors::SingleThreadedExecutor executor_;
  std::promise<void> promise_;
  std::thread thread_;
};

TEST_F(TestDomainBridgeEndToEnd, remap_topic_name)
{
  const std::string topic_name("test_remap_topic_before");
  const std::string remap_name("test_remap_topic_after");

  std::atomic<bool> got_message = false;

  auto pub = node_1_->create_publisher<test_msgs::msg::BasicTypes>(topic_name, pub_sub_qos_);
  auto sub = node_2_->create_subscription<test_msgs::msg::BasicTypes>(
    remap_name,
    pub_sub_qos_,
    [&got_message](test_msgs::msg::BasicTypes::SharedPtr) {got_message = true;});

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridge bridge;
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.remap_name(remap_name);
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", kDomain1, kDomain2, topic_bridge_options);

  pub->publish(test_msgs::msg::BasicTypes{});
  ScopedAsyncSpinner spinner{context_1_};
  spinner.get_executor().add_node(node_1_);
  spinner.get_executor().add_node(node_2_);
  bridge.add_to_executor(spinner.get_executor());
  EXPECT_TRUE(poll_condition([&got_message]() {return got_message.load();}, 10s));
}

TEST_F(TestDomainBridgeEndToEnd, remap_topic_name_with_substitution)
{
  const std::string topic_name("test_remap_topic_with_sub");
  const std::string remap_name("~/test_remap_topic_with_sub");
  const std::string domain_bridge_name("my_test_name");
  const std::string expected_name("/my_test_name/test_remap_topic_with_sub");

  std::atomic<bool> got_message = false;

  // Create a publisher on domain 1
  auto pub = node_1_->create_publisher<test_msgs::msg::BasicTypes>(topic_name, pub_sub_qos_);
  auto sub = node_2_->create_subscription<test_msgs::msg::BasicTypes>(
    expected_name,
    pub_sub_qos_,
    [&got_message](test_msgs::msg::BasicTypes::SharedPtr) {got_message = true;});

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridgeOptions domain_bridge_options;
  domain_bridge_options.name(domain_bridge_name);
  domain_bridge::DomainBridge bridge{domain_bridge_options};
  domain_bridge::TopicBridgeOptions topic_bridge_options;
  topic_bridge_options.remap_name(remap_name);
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", kDomain1, kDomain2, topic_bridge_options);

  pub->publish(test_msgs::msg::BasicTypes{});
  ScopedAsyncSpinner spinner{context_1_};
  spinner.get_executor().add_node(node_1_);
  spinner.get_executor().add_node(node_2_);
  bridge.add_to_executor(spinner.get_executor());
  EXPECT_TRUE(poll_condition([&got_message]() {return got_message.load();}, 10s));
}

TEST_F(TestDomainBridgeEndToEnd, compress_mode)
{
  const std::string topic_name("test_compress");
  const std::string domain_bridge_name("my_test_name");

  std::atomic<bool> got_message = false;

  // Create a publisher on domain 1
  auto pub = node_1_->create_publisher<test_msgs::msg::BasicTypes>(topic_name, pub_sub_qos_);
  auto sub = node_2_->create_subscription<domain_bridge::msg::CompressedMsg>(
    topic_name,
    pub_sub_qos_,
    [&got_message](domain_bridge::msg::CompressedMsg::SharedPtr) {got_message = true;});

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridgeOptions domain_bridge_options;
  domain_bridge_options.name(domain_bridge_name);
  domain_bridge_options.mode(domain_bridge::DomainBridgeOptions::Mode::Compress);
  domain_bridge::DomainBridge bridge{domain_bridge_options};
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", kDomain1, kDomain2);

  pub->publish(test_msgs::msg::BasicTypes{});
  ScopedAsyncSpinner spinner{context_1_};
  spinner.get_executor().add_node(node_1_);
  spinner.get_executor().add_node(node_2_);
  bridge.add_to_executor(spinner.get_executor());
  EXPECT_TRUE(poll_condition([&got_message]() {return got_message.load();}, 10s));
}

TEST_F(TestDomainBridgeEndToEnd, decompress_mode)
{
  const std::string topic_name("test_decompress");
  const std::string domain_bridge_name("my_test_name");

  std::atomic<bool> got_message = false;

  // Create a publisher on domain 1
  auto pub = node_1_->create_publisher<domain_bridge::msg::CompressedMsg>(topic_name, pub_sub_qos_);
  auto sub = node_2_->create_subscription<test_msgs::msg::BasicTypes>(
    topic_name,
    pub_sub_qos_,
    [&got_message](test_msgs::msg::BasicTypes::SharedPtr) {got_message = true;});

  // Bridge the publisher topic to domain 2 with a remap option
  domain_bridge::DomainBridgeOptions domain_bridge_options;
  domain_bridge_options.name(domain_bridge_name);
  domain_bridge_options.mode(domain_bridge::DomainBridgeOptions::Mode::Decompress);
  domain_bridge::DomainBridge bridge{domain_bridge_options};
  bridge.bridge_topic(
    topic_name, "test_msgs/msg/BasicTypes", kDomain1, kDomain2);

  rclcpp::Serialization<test_msgs::msg::BasicTypes> serializer;
  test_msgs::msg::BasicTypes msg;
  rclcpp::SerializedMessage serialized_msg;
  serializer.serialize_message(&msg, &serialized_msg);
  domain_bridge::msg::CompressedMsg compressed_msg;
  std::unique_ptr<ZSTD_CCtx, size_t (*)(ZSTD_CCtx *)> cctx{ZSTD_createCCtx(), &ZSTD_freeCCtx};
  compressed_msg.data = domain_bridge::compress_message(cctx.get(), std::move(serialized_msg));
  pub->publish(compressed_msg);
  ScopedAsyncSpinner spinner{context_1_};
  spinner.get_executor().add_node(node_1_);
  spinner.get_executor().add_node(node_2_);
  bridge.add_to_executor(spinner.get_executor());
  EXPECT_TRUE(poll_condition([&got_message]() {return got_message.load();}, 10s));
}
