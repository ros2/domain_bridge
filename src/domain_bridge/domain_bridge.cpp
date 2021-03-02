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

#include "domain_bridge/domain_bridge.hpp"

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "domain_bridge/domain_bridge_options.hpp"
#include "domain_bridge/exceptions.hpp"
#include "domain_bridge/topic_bridge_options.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/executor.hpp"
#include "rcutils/logging_macros.h"
#include "rosbag2_cpp/typesupport_helpers.hpp"

#include "generic_publisher.hpp"
#include "generic_subscription.hpp"

namespace domain_bridge
{

/// Implementation of \ref DomainBridge.
class DomainBridgeImpl
{
public:
  using SubscriptionMap =
    std::map<std::pair<size_t, std::string>, std::shared_ptr<GenericSubscription>>;
  using PublisherMap = std::map<std::pair<size_t, std::string>, std::shared_ptr<GenericPublisher>>;
  using NodeMap = std::map<size_t, std::shared_ptr<rclcpp::Node>>;

  explicit DomainBridgeImpl(const DomainBridgeOptions & options)
  : options_(options)
  {}

  ~DomainBridgeImpl() = default;

  rclcpp::Context::SharedPtr create_context_with_domain_id(size_t domain_id)
  {
    auto context = std::make_shared<rclcpp::Context>();
    rclcpp::InitOptions options;
    options.auto_initialize_logging(false).set_domain_id(domain_id);
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

  rclcpp::Node::SharedPtr get_node_for_domain(size_t domain_id)
  {
    auto domain_id_node_pair = node_map_.find(domain_id);

    // If we don't already have a node for the domain, create one
    if (node_map_.end() == domain_id_node_pair) {
      auto context = create_context_with_domain_id(domain_id);
      auto node_options = create_node_options(context);
      std::ostringstream oss;
      oss << "domain_bridge_" << std::to_string(options_.id()) << "_" << std::to_string(domain_id);
      auto node = std::make_shared<rclcpp::Node>(oss.str(), node_options);
      node_map_[domain_id] = node;
      return node;
    }

    return domain_id_node_pair->second;
  }

  std::shared_ptr<GenericPublisher> create_publisher(
    rclcpp::Node::SharedPtr node,
    PublisherMap & map,
    const std::string & topic_name,
    const rosidl_message_type_support_t & typesupport_handle)
  {
    size_t domain_id = node->get_node_options().context()->get_domain_id();
    auto domain_id_topic_pair = std::make_pair(domain_id, topic_name);
    if (map.find(domain_id_topic_pair) != map.end()) {
      throw BridgeExistsError(
              node->get_node_options().context()->get_domain_id(),
              topic_name
      );
    }
    map[domain_id_topic_pair] = std::make_shared<GenericPublisher>(
      node->get_node_base_interface().get(),
      typesupport_handle,
      topic_name,
      rclcpp::QoS(10));
    return map[domain_id_topic_pair];
  }

  void create_subscription(
    rclcpp::Node::SharedPtr node,
    SubscriptionMap & map,
    std::shared_ptr<GenericPublisher> publisher,
    const std::string & topic_name,
    const rosidl_message_type_support_t & typesupport_handle)
  {
    size_t domain_id = node->get_node_options().context()->get_domain_id();
    auto domain_id_topic_pair = std::make_pair(domain_id, topic_name);
    if (map.find(domain_id_topic_pair) != map.end()) {
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
    map[domain_id_topic_pair] = subscription;
  }

  void bridge_topic(
    const std::string & topic,
    const std::string & type,
    size_t from_domain_id,
    size_t to_domain_id,
    const TopicBridgeOptions & options)
  {
    // TODO(jacobperron): Do something with options.callback_group()
    (void)options;

    rclcpp::Node::SharedPtr from_domain_node = get_node_for_domain(from_domain_id);
    rclcpp::Node::SharedPtr to_domain_node = get_node_for_domain(to_domain_id);

    // Get typesupport
    auto typesupport_library = rosbag2_cpp::get_typesupport_library(
      type, "rosidl_typesupport_cpp");
    auto typesupport_handle = rosbag2_cpp::get_typesupport_handle(
      type, "rosidl_typesupport_cpp", typesupport_library);

    // Create publisher for the 'to_domain' and subscription for the 'from_domain'
    // The publisher should be created first so it is available to the subscription callback
    auto publisher = this->create_publisher(
      to_domain_node, this->publisher_map_, topic, *typesupport_handle);
    this->create_subscription(
      from_domain_node, this->subscription_map_, publisher, topic, *typesupport_handle);
  }

  void add_to_executor(rclcpp::Executor & executor)
  {
    for (const auto & domain_id_node_pair : this->node_map_) {
      executor.add_node(domain_id_node_pair.second);
    }
  }

  DomainBridgeOptions options_;

  /// Map of domain IDs to ROS nodes
  NodeMap node_map_;

  /// Map of (domain id, topic name) to publishers
  PublisherMap publisher_map_;

  /// Map of (domain_id, topic name) to subscriptions
  SubscriptionMap subscription_map_;
};  // class DomainBridgeImpl

DomainBridge::DomainBridge(const DomainBridgeOptions & options)
: impl_(std::make_unique<DomainBridgeImpl>(options))
{}

DomainBridge::~DomainBridge() = default;

DomainBridgeOptions
DomainBridge::get_domain_bridge_options() const
{
  return impl_->options_;
}

void DomainBridge::add_to_executor(rclcpp::Executor & executor)
{
  impl_->add_to_executor(executor);
}

void DomainBridge::bridge_topic(
  const std::string & topic,
  const std::string & type,
  size_t from_domain_id,
  size_t to_domain_id,
  const TopicBridgeOptions & options)
{
  impl_->bridge_topic(topic, type, from_domain_id, to_domain_id, options);
}

}  // namespace domain_bridge
