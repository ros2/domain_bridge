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

// Silly cpplint thinks this is a C system header
#include <optional>

#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
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
#include "rcutils/logging_macros.h"
#include "rosbag2_cpp/typesupport_helpers.hpp"
#include "rmw/types.h"

#include "generic_publisher.hpp"
#include "generic_subscription.hpp"

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

  ~DomainBridgeImpl()
  {
    shutting_down_.store(true);
    for (const auto & t : waiting_threads_) {
      t->join();
    }
  }

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

  /// Get the QoS to use for a topic bridge.
  /**
   * Queries QoS of existing publishers and returns QoS that is compatibile
   * with the majority of publishers.
   * If possible, the returned QoS will exactly match those of the publishers.
   *
   * If the existing publishers do not have matching QoS settings, then a warning is emitted.
   *
   * If there are no publishers, then no QoS is returned (i.e. the optional object is not set).
   */
  std::optional<rclcpp::QoS> get_topic_qos(std::string topic, rclcpp::Node::SharedPtr node)
  {
    // TODO(jacobperron): replace this with common implementation when it is available.
    //       See: https://github.com/ros2/rosbag2/issues/601

    // This implementation is inspired by rosbag2_transport:
    // https://github.com/ros2/rosbag2/blob/master/rosbag2_transport/src/rosbag2_transport/qos.cpp

    // Query QoS info for publishers
    std::vector<rclcpp::TopicEndpointInfo> endpoint_info_vec =
      node->get_publishers_info_by_topic(topic);
    std::size_t num_endpoints = endpoint_info_vec.size();

    // If there are no publishers, return default QoS
    if (num_endpoints < 1u) {
      return {};
    }

    // Initialize QoS arbitrarily
    rclcpp::QoS result_qos = endpoint_info_vec[0].qos_profile();

    // Reliability and durability policies can cause trouble with enpoint matching
    // Count number of "reliable" publishers and number of "transient local" publishers
    std::size_t reliable_count = 0u;
    std::size_t transient_local_count = 0u;
    for (const auto & info : endpoint_info_vec) {
      const auto & profile = info.qos_profile().get_rmw_qos_profile();
      if (profile.reliability == RMW_QOS_POLICY_RELIABILITY_RELIABLE) {
        reliable_count++;
      }
      if (profile.durability == RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL) {
        transient_local_count++;
      }
    }

    // If not all publishers have a "reliable" policy, then use a "best effort" policy
    // and print a warning
    if (reliable_count > 0u && reliable_count != num_endpoints) {
      result_qos.best_effort();
      std::cerr << "Some, but not all, publishers on topic '" << topic << "' on domain ID " <<
        std::to_string(node->get_node_options().context()->get_domain_id()) <<
        " offer 'reliable' reliability. Falling back to 'best effort' reliability in order "
        "to connect to all publishers." << std::endl;
    }

    // If not all publishers have a "transient local" policy, then use a "volatile" policy
    // and print a warning
    if (transient_local_count > 0u && transient_local_count != num_endpoints) {
      result_qos.durability_volatile();
      std::cerr << "Some, but not all, publishers on topic '" << topic << "' on domain ID " <<
        std::to_string(node->get_node_options().context()->get_domain_id()) <<
        " offer 'transient local' durability. Falling back to 'volatile' durability in order "
        "to connect to all publishers." << std::endl;
    }

    return result_qos;
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

  void register_on_publisher_qos_ready_callback(
    std::string topic,
    rclcpp::Node::SharedPtr node,
    std::function<void(const rclcpp::QoS)> callback)
  {
    // If the QoS is already avaiable, trigger the callback immediately
    auto opt_qos = this->get_topic_qos(topic, node);
    if (opt_qos) {
      const rclcpp::QoS & qos = opt_qos.value();
      callback(qos);
      return;
    }

    // Create a thread that waits for a publisher to become availble
    auto invoke_callback_when_qos_ready = [this, topic, node, callback]()
      {
        while (!this->shutting_down_.load()) {
          // TODO(jacobperron): Use graph events
          auto opt_qos = this->get_topic_qos(topic, node);
          if (opt_qos) {
            const rclcpp::QoS & qos = opt_qos.value();
            callback(qos);
            return;
          }
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      };
    auto waiting_thread = std::make_shared<std::thread>(invoke_callback_when_qos_ready);
    waiting_threads_.push_back(waiting_thread);
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
    auto create_bridge = [ = ](const rclcpp::QoS & qos)
      {
        // Get typesupport handle
        auto typesupport_handle = rosbag2_cpp::get_typesupport_handle(
          type, "rosidl_typesupport_cpp", loaded_typesupports_[type]);

        // Create publisher for the 'to_domain'
        // The publisher should be created first so it is available to the subscription callback
        auto publisher = this->create_publisher(
          to_domain_node, topic, qos, *typesupport_handle, topic_options.callback_group());

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
    register_on_publisher_qos_ready_callback(topic, from_domain_node, create_bridge);
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

  // Cache of typesupport libraries
  TypesupportMap loaded_typesupports_;

  /// Threads used for waiting on publishers to become available
  std::vector<std::shared_ptr<std::thread>> waiting_threads_;

  /// Flag to tell waiting threads we're shutting down
  std::atomic_bool shutting_down_{false};
};  // class DomainBridgeImpl

DomainBridge::DomainBridge(const DomainBridgeOptions & options)
: impl_(std::make_unique<DomainBridgeImpl>(options))
{}

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
