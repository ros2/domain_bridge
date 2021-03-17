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

#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "domain_bridge/domain_bridge_options.hpp"
#include "domain_bridge/topic_bridge.hpp"
#include "domain_bridge/topic_bridge_options.hpp"

#include "rclcpp/executor.hpp"
#include "rclcpp/expand_topic_or_service_name.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rosbag2_cpp/typesupport_helpers.hpp"
#include "rmw/types.h"

#include "generic_publisher.hpp"
#include "generic_subscription.hpp"
#include "wait_for_qos_handler.hpp"

namespace domain_bridge
{

/// Implementation of \ref DomainBridge.
class DomainBridgeImpl
{
public:
  using NodeMap = std::unordered_map<std::size_t, std::shared_ptr<rclcpp::Node>>;
  using TopicBridgeMap = std::map<
    TopicBridge,
    std::pair<std::shared_ptr<GenericPublisher>, std::shared_ptr<GenericSubscription>>>;
  using TypesupportMap = std::unordered_map<
    std::string, std::shared_ptr<rcpputils::SharedLibrary>>;

  explicit DomainBridgeImpl(const DomainBridgeOptions & options)
  : options_(options)
  {}

  ~DomainBridgeImpl() = default;

  rclcpp::Context::SharedPtr create_context_with_domain_id(std::size_t domain_id)
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

  rclcpp::Node::SharedPtr get_node_for_domain(std::size_t domain_id)
  {
    auto domain_id_node_pair = node_map_.find(domain_id);

    // If we don't already have a node for the domain, create one
    if (node_map_.end() == domain_id_node_pair) {
      auto context = create_context_with_domain_id(domain_id);
      auto node_options = create_node_options(context);
      std::ostringstream oss;
      oss << options_.name() << "_" << std::to_string(domain_id);
      auto node = std::make_shared<rclcpp::Node>(oss.str(), node_options);
      node_map_[domain_id] = node;
      return node;
    }

    return domain_id_node_pair->second;
  }

  /// Load typesupport library into a cache.
  void load_typesupport_library(std::string type)
  {
    if (loaded_typesupports_.find(type) != loaded_typesupports_.end()) {
      // Typesupport library already loaded
      return;
    }
    loaded_typesupports_[type] = rosbag2_cpp::get_typesupport_library(
      type, "rosidl_typesupport_cpp");
  }

  std::shared_ptr<GenericPublisher> create_publisher(
    rclcpp::Node::SharedPtr node,
    const std::string & topic_name,
    const rclcpp::QoS & qos,
    const rosidl_message_type_support_t & typesupport_handle,
    rclcpp::CallbackGroup::SharedPtr group)
  {
    auto publisher = std::make_shared<GenericPublisher>(
      node->get_node_base_interface().get(),
      typesupport_handle,
      topic_name,
      qos);
    node->get_node_topics_interface()->add_publisher(publisher, std::move(group));
    return publisher;
  }

  std::shared_ptr<GenericSubscription> create_subscription(
    rclcpp::Node::SharedPtr node,
    std::shared_ptr<GenericPublisher> publisher,
    const std::string & topic_name,
    const rclcpp::QoS & qos,
    const rosidl_message_type_support_t & typesupport_handle,
    rclcpp::CallbackGroup::SharedPtr group)
  {
    // Create subscription
    auto subscription = std::make_shared<GenericSubscription>(
      node->get_node_base_interface().get(),
      typesupport_handle,
      topic_name,
      qos,
      [publisher](std::shared_ptr<rclcpp::SerializedMessage> msg) {
        // Publish message into the other domain
        auto serialized_data_ptr = std::make_shared<rcl_serialized_message_t>(
          msg->get_rcl_serialized_message());
        publisher->publish(serialized_data_ptr);
      });
    node->get_node_topics_interface()->add_subscription(subscription, std::move(group));
    return subscription;
  }

  void bridge_topic(
    const TopicBridge & topic_bridge,
    const TopicBridgeOptions & topic_options)
  {
    // Validate topic name
    const std::string & topic = rclcpp::expand_topic_or_service_name(
      topic_bridge.topic_name, options_.name(), "/");

    const std::string & type = topic_bridge.type_name;
    const std::size_t & from_domain_id = topic_bridge.from_domain_id;
    const std::size_t & to_domain_id = topic_bridge.to_domain_id;

    // Validate type name by loading library support (if not already loaded)
    // Front-loading let's us fail early on invalid type names
    load_typesupport_library(type);

    // Check if already bridged
    if (bridged_topics_.find(topic_bridge) != bridged_topics_.end()) {
      std::cerr << "Topic '" << topic << "' with type '" << type << "'" <<
        " already bridged from domain " << std::to_string(from_domain_id) <<
        " to domain " << std::to_string(to_domain_id) << ", ignoring" << std::endl;
      return;
    }

    // Create a null entry to avoid duplicate bridges
    bridged_topics_[topic_bridge] = {nullptr, nullptr};

    rclcpp::Node::SharedPtr from_domain_node = get_node_for_domain(from_domain_id);
    rclcpp::Node::SharedPtr to_domain_node = get_node_for_domain(to_domain_id);

    // Register a callback to be triggered when QoS settings are available for one or more
    // publishers on the 'from' side of the bridge
    // The callback may be triggered immediately if a publisher is available
    auto create_bridge =
      [this, topic, topic_bridge, topic_options, from_domain_node, to_domain_node]
        (const QosMatchInfo & qos_match)
      {
        const std::string & type = topic_bridge.type_name;

        // Apply QoS overrides
        const auto & qos_options = topic_options.qos_options();
        rclcpp::QoS qos(qos_options.depth());
        qos.history(qos_options.history());
        if (qos_options.reliability()) {
          qos.reliability(qos_options.reliability().value());
        } else {
          qos.reliability(qos_match.qos.reliability());
        }
        if (qos_options.durability()) {
          qos.durability(qos_options.durability().value());
        } else {
          qos.durability(qos_match.qos.durability());
        }
        if (qos_options.deadline()) {
          const auto deadline_ns = qos_options.deadline().value();
          if (deadline_ns < 0) {
            qos.deadline(
              rclcpp::Duration::from_nanoseconds(std::numeric_limits<std::int64_t>::max()));
          } else {
            qos.deadline(rclcpp::Duration::from_nanoseconds(deadline_ns));
          }
        } else {
          qos.deadline(qos_match.qos.deadline());
        }
        if (qos_options.lifespan()) {
          const auto lifespan_ns = qos_options.lifespan().value();
          if (lifespan_ns < 0) {
            qos.lifespan(
              rclcpp::Duration::from_nanoseconds(std::numeric_limits<std::int64_t>::max()));
          } else {
            qos.lifespan(rclcpp::Duration::from_nanoseconds(lifespan_ns));
          }
        } else {
          qos.lifespan(qos_match.qos.lifespan());
        }

        qos.liveliness(qos_match.qos.liveliness());
        qos.liveliness_lease_duration(qos_match.qos.liveliness_lease_duration());

        // Print any match warnings
        for (const auto & warning : qos_match.warnings) {
          std::cerr << warning << std::endl;
        }

        // Get typesupport handle
        auto typesupport_handle = rosbag2_cpp::get_typesupport_handle(
          type, "rosidl_typesupport_cpp", loaded_typesupports_.at(type));

        // Create publisher for the 'to_domain'
        // The publisher should be created first so it is available to the subscription callback
        auto publisher = this->create_publisher(
          to_domain_node,
          topic,
          qos,
          *typesupport_handle,
          topic_options.callback_group());

        // Create subscription for the 'from_domain'
        auto subscription = this->create_subscription(
          from_domain_node,
          publisher,
          topic,
          qos,
          *typesupport_handle,
          topic_options.callback_group());

        this->bridged_topics_[topic_bridge] = {publisher, subscription};
      };
    wait_for_qos_handler_.register_on_publisher_qos_ready_callback(
      topic, from_domain_node, create_bridge);
  }

  void add_to_executor(rclcpp::Executor & executor)
  {
    for (const auto & domain_id_node_pair : node_map_) {
      executor.add_node(domain_id_node_pair.second);
    }
  }

  std::vector<TopicBridge> get_bridged_topics() const
  {
    std::vector<TopicBridge> result;
    for (const auto & bridge : bridged_topics_) {
      result.push_back(bridge.first);
    }
    return result;
  }

  DomainBridgeOptions options_;

  /// Map of domain IDs to ROS nodes
  NodeMap node_map_;

  /// Set of bridged topics
  TopicBridgeMap bridged_topics_;

  /// Cache of typesupport libraries
  TypesupportMap loaded_typesupports_;

  /// QoS event handler
  WaitForQosHandler wait_for_qos_handler_;
};  // class DomainBridgeImpl

DomainBridge::DomainBridge(const DomainBridgeOptions & options)
: impl_(std::make_unique<DomainBridgeImpl>(options))
{}

DomainBridge::DomainBridge(const DomainBridgeConfig & config)
: impl_(std::make_unique<DomainBridgeImpl>(config.options))
{
  for (const auto & topic_bridge_pair : config.topics) {
    bridge_topic(topic_bridge_pair.first, topic_bridge_pair.second);
  }
}

DomainBridge::DomainBridge(DomainBridge && other) = default;

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
  std::size_t from_domain_id,
  std::size_t to_domain_id,
  const TopicBridgeOptions & options)
{
  impl_->bridge_topic({topic, type, from_domain_id, to_domain_id}, options);
}

void DomainBridge::bridge_topic(
  const TopicBridge & topic_bridge,
  const TopicBridgeOptions & options)
{
  impl_->bridge_topic(topic_bridge, options);
}

std::vector<TopicBridge> DomainBridge::get_bridged_topics() const
{
  return impl_->get_bridged_topics();
}

}  // namespace domain_bridge
