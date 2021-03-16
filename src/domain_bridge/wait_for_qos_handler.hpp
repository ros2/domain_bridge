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

#ifndef DOMAIN_BRIDGE__WAIT_FOR_QOS_HANDLER_HPP_
#define DOMAIN_BRIDGE__WAIT_FOR_QOS_HANDLER_HPP_

// Silly cpplint thinks this is a C system header
#include <optional>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/node.hpp"
#include "rclcpp/qos.hpp"

namespace domain_bridge
{

/// QoS match information.
/**
 * Used as a return value for QoS matching functions.
 */
struct QosMatchInfo
{
  explicit QosMatchInfo(const rclcpp::QoS & init_qos)
  : qos(init_qos)
  {}

  /// A matching QoS
  rclcpp::QoS qos;

  /// Any warning messages about the matching QoS
  std::vector<std::string> warnings;
};

/// Listens for publishers on the ROS graph and provides best matching QoS for them.
/**
 * To start listening on a particular topic, register a callback with
 * `register_on_publisher_qos_ready_callback()`.
 * The registered callback will be called when one or more publishers are available.
 * The callback is automatically unregistered after it is called once, or when this class
 * object is destroyed.
 *
 * This is a RAII class.
 */
class WaitForQosHandler
{
public:
  WaitForQosHandler() = default;

  ~WaitForQosHandler()
  {
    shutting_down_.store(true);
    for (const auto & t : waiting_threads_) {
      t->join();
    }
  }

  /// Deleted copy constructor.
  explicit WaitForQosHandler(const DomainBridgeOptions & other) = delete;

  /// Deleted assignment operator.
  WaitForQosHandler &
  operator=(const WaitForQosHandler & other) = delete;

  /// Register a callback that is called when QoS is ready for one or more publishers.
  /**
   * \param topic: The name of the topic to monitor.
   * \param node: The node to use to monitor the topic.
   * \param callback: User callback that is triggered when QoS settings can be determined
   *   for the provided topic name.
   *   If QoS settings are already available, then the callback is called immediately.
   */
  void register_on_publisher_qos_ready_callback(
    std::string topic,
    rclcpp::Node::SharedPtr node,
    std::function<void(const QosMatchInfo)> callback)
  {
    // If the QoS is already avaiable, trigger the callback immediately
    auto opt_qos = this->get_topic_qos(topic, node);
    if (opt_qos) {
      const QosMatchInfo & qos = opt_qos.value();
      callback(qos);
      return;
    }

    // Create a thread that waits for a publisher to become available
    auto invoke_callback_when_qos_ready = [this, topic, node, callback]()
      {
        auto event = node->get_graph_event();
        while (!this->shutting_down_.load()) {
          // Wait for graph event
          // Note, in case new publishers don't trigger a graph event we add a
          // timeout so that we can still poll periodically for new publishers
          node->wait_for_graph_change(event, std::chrono::seconds(1));
          event->check_and_clear();
          // Check if QoS is ready
          auto opt_qos = this->get_topic_qos(topic, node);
          if (opt_qos) {
            const QosMatchInfo & qos = opt_qos.value();
            callback(qos);
            return;
          }
        }
      };
    auto waiting_thread = std::make_shared<std::thread>(invoke_callback_when_qos_ready);
    waiting_threads_.push_back(waiting_thread);
  }

private:
  /// Get QoS settings that best match all available publishers.
  /**
   * Queries QoS of existing publishers and returns QoS that is compatibile
   * with the majority of publishers.
   * If possible, the returned QoS will exactly match those of the publishers.
   *
   * If the existing publishers do not have matching QoS settings, then a warning is set in the
   * return struct.
   *
   * If there are no publishers, then no QoS is returned (i.e. the optional object is not set).
   */
  std::optional<QosMatchInfo> get_topic_qos(std::string topic, rclcpp::Node::SharedPtr node)
  {
    // TODO(jacobperron): replace this with common implementation when it is available.
    //       See: https://github.com/ros2/rosbag2/issues/601

    // This implementation is inspired by rosbag2_transport:
    // https://github.com/ros2/rosbag2/blob/master/rosbag2_transport/src/rosbag2_transport/qos.cpp

    // Query QoS info for publishers
    std::vector<rclcpp::TopicEndpointInfo> endpoint_info_vec =
      node->get_publishers_info_by_topic(topic);
    std::size_t num_endpoints = endpoint_info_vec.size();

    // If there are no publishers, return an empty optional
    if (num_endpoints < 1u) {
      return {};
    }

    // Initialize QoS arbitrarily
    QosMatchInfo result_qos(endpoint_info_vec[0].qos_profile());

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
      result_qos.qos.best_effort();
      std::string warning = "Some, but not all, publishers on topic '" + topic +
        "' on domain ID " + std::to_string(node->get_node_options().context()->get_domain_id()) +
        " offer 'reliable' reliability. Falling back to 'best effort' reliability in order "
        "to connect to all publishers.";
      result_qos.warnings.push_back(warning);
    }

    // If not all publishers have a "transient local" policy, then use a "volatile" policy
    // and print a warning
    if (transient_local_count > 0u && transient_local_count != num_endpoints) {
      result_qos.qos.durability_volatile();
      std::string warning = "Some, but not all, publishers on topic '" + topic +
        "' on domain ID " + std::to_string(node->get_node_options().context()->get_domain_id()) +
        " offer 'transient local' durability. Falling back to 'volatile' durability in order "
        "to connect to all publishers.";
      result_qos.warnings.push_back(warning);
    }

    return result_qos;
  }

  /// Flag to tell waiting threads we're shutting down
  std::atomic_bool shutting_down_{false};

  /// Threads used for waiting on publishers to become available
  std::vector<std::shared_ptr<std::thread>> waiting_threads_;
};  // class WaitForQosHandler

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__WAIT_FOR_QOS_HANDLER_HPP_
