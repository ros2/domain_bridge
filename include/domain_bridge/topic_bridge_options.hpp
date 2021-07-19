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

#ifndef DOMAIN_BRIDGE__TOPIC_BRIDGE_OPTIONS_HPP_
#define DOMAIN_BRIDGE__TOPIC_BRIDGE_OPTIONS_HPP_

#include <memory>
#include <string>

#include "rclcpp/callback_group.hpp"

#include "domain_bridge/qos_options.hpp"
#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

/// Options for bridged topics.
class TopicBridgeOptions
{
public:
  /// Constructor.
  /**
   *  Default values:
   *
   *    - callback_group = nullptr (node's default)
   *    - qos_options = default (see QosOptions for more information)
   *    - remap_name = "" (no remap)
   *    - bidirectional = false (one-way)
   *    - reversed = false
   */
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions() = default;

  /// Get callback group associated with the topics pub/sub pair.
  DOMAIN_BRIDGE_PUBLIC
  std::shared_ptr<rclcpp::CallbackGroup>
  callback_group() const;

  /// Set the callback group associated with the topics pub/sub pair.
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions &
  callback_group(std::shared_ptr<rclcpp::CallbackGroup> group);

  /// Get QoS options.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions
  qos_options() const;

  /// Set QoS options.
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions &
  qos_options(const QosOptions & qos_options);

  /// Get remap name.
  DOMAIN_BRIDGE_PUBLIC
  const std::string &
  remap_name() const;

  /// Set remap name.
  /**
   * An empty string implies no remap.
   */
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions &
  remap_name(const std::string & remap_name);

  /// Get bidirectional option.
  DOMAIN_BRIDGE_PUBLIC
  const bool &
  bidirectional() const;

  /// Set bidirectional option. If true, this will bridge the topic in both directions.
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions &
  bidirectional(const bool & bidirectional);

  /// Get reversed option.
  DOMAIN_BRIDGE_PUBLIC
  const bool &
  reversed() const;

  /// Set reversed option. If true, this will swap the 'to' and 'from' domain IDs.
  DOMAIN_BRIDGE_PUBLIC
  TopicBridgeOptions &
  reversed(const bool & reversed);

private:
  std::shared_ptr<rclcpp::CallbackGroup> callback_group_{nullptr};

  QosOptions qos_options_;

  std::string remap_name_;

  bool bidirectional_{false};

  bool reversed_{false};
};  // class TopicBridgeOptions

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__TOPIC_BRIDGE_OPTIONS_HPP_
