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
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
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
  /// A matching QoS
  rclcpp::QoS qos{10};  // the depth value should be overridden later

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
    {
      std::lock_guard<std::mutex> lock(waiting_map_mutex_);
      shutting_down_ = true;
    }
    for (const auto & t : waiting_threads_) {
      // Notify and join
      t.second.second->notify_all();
      t.second.first->join();
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
    auto opt_qos = get_topic_qos(topic, node);
    if (opt_qos) {
      const QosMatchInfo & qos = opt_qos.value();
      callback(qos);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(waiting_map_mutex_);
      waiting_map_[node].push_back({topic, callback});

      // If we already have a thread for this node, then notify that there is a new callback
      auto existing_thread = waiting_threads_.find(node);
      if (existing_thread != waiting_threads_.end()) {
        existing_thread->second.second->notify_all();
        return;
      }
    }

    // If we made it this far, there doesn't exist a thread for waiting so we'll create one
    // First, create a condition variable to notify the waiting thread when to wake up
    auto cv = std::make_shared<std::condition_variable>();

    // Create a thread that waits for a publisher to become available
    auto invoke_callback_when_qos_ready = [this, node, cv]()
      {
        auto event = node->get_graph_event();
        while (true) {
          // Wait for graph event
          // Note, in case new publishers don't trigger a graph event we add a
          // timeout so that we can still poll periodically for new publishers
          node->wait_for_graph_change(event, std::chrono::seconds(1));
          event->check_and_clear();

          {
            std::unique_lock<std::mutex> lock(waiting_map_mutex_);
            // If we're shuttdown down, exit the thread
            if (this->shutting_down_) {
              return;
            }

            // Check if QoS is ready for any of the topics
            auto & topic_callback_vec = waiting_map_.at(node);
            for (auto it = topic_callback_vec.begin(); it != topic_callback_vec.end(); ++it) {
              const std::string & topic = it->first;
              const auto callback = it->second;
              std::optional<QosMatchInfo> opt_qos;
              try {
                opt_qos = this->get_topic_qos(topic, node);
              } catch (const rclcpp::exceptions::RCLError & ex) {
                // If the context was shutdown, then exit cleanly
                // This can happen if we get a SIGINT
                const auto context = node->get_node_options().context();
                if (!context->is_valid()) {
                  return;
                }
                throw ex;
              }

              if (opt_qos) {
                const QosMatchInfo & qos = opt_qos.value();
                callback(qos);
                topic_callback_vec.erase(it--);
              }
            }

            // It's only worth continuing if we have callbacks to handle
            // or we're shutting down
            cv->wait(
              lock,
              [this, &topic_callback_vec]
              {return (topic_callback_vec.size() > 0u) || this->shutting_down_;});
          }
        }
      };
    auto waiting_thread = std::make_shared<std::thread>(invoke_callback_when_qos_ready);
    waiting_threads_[node] = {waiting_thread, cv};
  }

private:
  using ThreadMap = std::unordered_map<
    std::shared_ptr<rclcpp::Node>,
    std::pair<std::shared_ptr<std::thread>, std::shared_ptr<std::condition_variable>>>;
  using WaitingMap = std::unordered_map<
    std::shared_ptr<rclcpp::Node>,
    std::vector<std::pair<std::string, std::function<void (const QosMatchInfo)>>>>;

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
  std::optional<QosMatchInfo> get_topic_qos(std::string topic, rclcpp::Node::SharedPtr node) const
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

    // Initialize QoS
    QosMatchInfo result_qos;
    // Default reliability and durability to value of first endpoint
    result_qos.qos.reliability(endpoint_info_vec[0].qos_profile().reliability());
    result_qos.qos.durability(endpoint_info_vec[0].qos_profile().durability());
    // Always use automatic liveliness
    result_qos.qos.liveliness(rclcpp::LivelinessPolicy::Automatic);

    // Reliability and durability policies can cause trouble with enpoint matching
    // Count number of "reliable" publishers and number of "transient local" publishers
    std::size_t reliable_count = 0u;
    std::size_t transient_local_count = 0u;
    // For duration-based policies, note the largest value to ensure matching all publishers
    rclcpp::Duration max_deadline(0, 0u);
    rclcpp::Duration max_lifespan(0, 0u);
    for (const auto & info : endpoint_info_vec) {
      const auto & profile = info.qos_profile();
      if (profile.reliability() == rclcpp::ReliabilityPolicy::Reliable) {
        reliable_count++;
      }
      if (profile.durability() == rclcpp::DurabilityPolicy::TransientLocal) {
        transient_local_count++;
      }
      if (profile.deadline() > max_deadline) {
        max_deadline = profile.deadline();
      }
      if (profile.lifespan() > max_lifespan) {
        max_lifespan = profile.lifespan();
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

    result_qos.qos.deadline(max_deadline);
    result_qos.qos.lifespan(max_lifespan);

    return result_qos;
  }

  /// Flag to tell waiting threads we're shutting down
  bool shutting_down_{false};

  /// Threads used for waiting on publishers to become available
  ThreadMap waiting_threads_;

  /// Waiting data shared by threads
  /**
   * Mapping nodes to (topic, callback) pairs.
   */
  WaitingMap waiting_map_;

  /// Mutex for waiting_map_
  std::mutex waiting_map_mutex_;
};  // class WaitForQosHandler

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__WAIT_FOR_QOS_HANDLER_HPP_
