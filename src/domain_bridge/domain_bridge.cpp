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

#include <map>
#include <memory>
#include <string>

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/exceptions.hpp"

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rcutils/logging_macros.h>
#include <rosbag2_cpp/typesupport_helpers.hpp>

#include "generic_publisher.hpp"
#include "generic_subscription.hpp"

namespace domain_bridge
{

/// Implementation of \ref DomainBridge.
class DomainBridgeImpl
{
public:
  using SubscriptionMap = std::map<std::string, std::shared_ptr<GenericSubscription>>;
  using PublisherMap = std::map<std::string, std::shared_ptr<GenericPublisher>>;

  DomainBridgeImpl(size_t domain_id_a, size_t domain_id_b)
  {
    this->context_a_ = create_context_with_domain_id(domain_id_a);
    this->context_b_ = create_context_with_domain_id(domain_id_b);

    auto options_a = create_node_options(context_a_);
    auto options_b = create_node_options(context_b_);

    this->node_a_ = std::make_shared<rclcpp::Node>("domain_bridge_a", options_a);
    this->node_b_ = std::make_shared<rclcpp::Node>("domain_bridge_b", options_b);

    rclcpp::ExecutorOptions executor_options;
    executor_options.context = context_a_;
    this->executor_ = std::make_unique<rclcpp::executors::MultiThreadedExecutor>(executor_options);

    // Test bridge
    this->bridge_from_a("chitter", "std_msgs/msg/String");
    this->bridge_from_b("chatter", "std_msgs/msg/String");
  }

  ~DomainBridgeImpl()
  {
    const std::string reason("Domain bridge shutdown");
    this->context_a_->shutdown(reason);
    this->context_b_->shutdown(reason);
  }

  rclcpp::Context::SharedPtr create_context_with_domain_id(size_t domain_id)
  {
    auto context = std::make_shared<rclcpp::Context>();
    rclcpp::InitOptions options;
    options.auto_initialize_logging(false)
           .set_domain_id(domain_id);
    context->init(0, nullptr, options);
    return context;
  }

  rclcpp::NodeOptions create_node_options(rclcpp::Context::SharedPtr context)
  {
    rclcpp::NodeOptions options;
    return options.context(context)
                  .use_global_arguments(false)
                  .start_parameter_services(false)
                  .start_parameter_event_publisher(false);
  }

  std::shared_ptr<GenericPublisher> create_publisher(
    rclcpp::Node::SharedPtr node,
    PublisherMap & map,
    const std::string & topic_name,
    const rosidl_message_type_support_t & typesupport_handle)
  {
    if (map.find(topic_name) != map.end()) {
      throw BridgeExistsError(
        node->get_node_options().context()->get_domain_id(),
        topic_name
      );
    }
    map[topic_name] = std::make_shared<GenericPublisher>(
      node->get_node_base_interface().get(),
      typesupport_handle,
      topic_name,
      rclcpp::QoS(10));
    return map[topic_name];
  }

  void create_subscription(
    rclcpp::Node::SharedPtr node,
    SubscriptionMap & map,
    std::shared_ptr<GenericPublisher> publisher,
    const std::string & topic_name,
    const rosidl_message_type_support_t & typesupport_handle)
  {
    if (map.find(topic_name) != map.end()) {
      throw BridgeExistsError(
        node->get_node_options().context()->get_domain_id(),
        topic_name
      );
    }

    // Create subscription
    auto subscription = std::make_shared<GenericSubscription>(
      node->get_node_base_interface().get(),
      typesupport_handle,
      topic_name,
      rclcpp::QoS(10),
      [publisher](std::shared_ptr<rclcpp::SerializedMessage> msg) {
        // Publish message into the other domain
        auto serialized_data_ptr = std::make_shared<rcl_serialized_message_t>(
          msg->get_rcl_serialized_message());
        publisher->publish(serialized_data_ptr);
      });
    node->get_node_topics_interface()->add_subscription(subscription, nullptr);
    map[topic_name] = subscription;
  }

  void bridge_from_a(const std::string & topic_name, const std::string & type_name) {
    // Get typesupport
    auto typesupport_library = rosbag2_cpp::get_typesupport_library(
      type_name, "rosidl_typesupport_cpp");
    auto typesupport_handle = rosbag2_cpp::get_typesupport_handle(
      type_name, "rosidl_typesupport_cpp", typesupport_library);

    // The publisher should be created first so it is available to the subscription callback
    auto publisher = this->create_publisher(
      this->node_b_, this->publisher_map_b_, topic_name, *typesupport_handle);
    this->create_subscription(
      this->node_a_, this->subscription_map_a_, publisher, topic_name, *typesupport_handle);
  }

  void bridge_from_b(const std::string & topic_name, const std::string & type_name) {
    // Get typesupport
    auto typesupport_library = rosbag2_cpp::get_typesupport_library(
      type_name, "rosidl_typesupport_cpp");
    auto typesupport_handle = rosbag2_cpp::get_typesupport_handle(
      type_name, "rosidl_typesupport_cpp", typesupport_library);

    // The publisher should be created first so it is available to the subscription callback
    auto publisher = this->create_publisher(
      this->node_a_, this->publisher_map_a_, topic_name, *typesupport_handle);
    this->create_subscription(
      this->node_b_, this->subscription_map_b_, publisher, topic_name, *typesupport_handle);
  }

  void spin() {
    this->executor_->add_node(this->node_a_);
    this->executor_->add_node(this->node_b_);
    this->executor_->spin();
  }

  /// Context for domain A
  rclcpp::Context::SharedPtr context_a_;

  /// Context for domain B
  rclcpp::Context::SharedPtr context_b_;

  /// Node for domain A
  rclcpp::Node::SharedPtr node_a_;

  /// Node for domain B
  rclcpp::Node::SharedPtr node_b_;

  /// Map of topic names to publishers for domain A
  PublisherMap publisher_map_a_;

  /// Map of topic names to publishers for domain B
  PublisherMap publisher_map_b_;

  /// Map of topic names to subscriptions for domain A
  SubscriptionMap subscription_map_a_;

  /// Map of topic names to subscriptions for domain B
  SubscriptionMap subscription_map_b_;

  /// Common executor for both nodes
  std::unique_ptr<rclcpp::Executor> executor_;
};  // class DomainBridgeImpl

DomainBridge::DomainBridge(size_t domain_id_a, size_t domain_id_b)
  : impl_(std::make_unique<DomainBridgeImpl>(domain_id_a, domain_id_b))
{}

DomainBridge::~DomainBridge()
{}

void DomainBridge::spin() {
  impl_->spin();
}

}  // namespace domain_bridge
