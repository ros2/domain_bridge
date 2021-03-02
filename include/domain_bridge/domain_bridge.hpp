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

#ifndef DOMAIN_BRIDGE__DOMAIN_BRIDGE_HPP_
#define DOMAIN_BRIDGE__DOMAIN_BRIDGE_HPP_

#include <memory>
#include <string>

#include "rclcpp/executor.hpp"

#include "domain_bridge/topic_bridge_options.hpp"
#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

class DomainBridgeImpl;

/// Bridge ROS entities across different domains.
class DomainBridge
{
public:
  /// Constructor.
  DOMAIN_BRIDGE_PUBLIC
  DomainBridge();

  /// Destructor.
  DOMAIN_BRIDGE_PUBLIC
  ~DomainBridge();

  /// Add the bridge to an executor.
  /**
   * When the executor is running ROS traffic will be bridged between domains.
   * Only topics registered with \ref bridge_topic() will have their messages forwarded.
   *
   * This method will add all nodes associated with this bridge to the provided executor.
   *
   * \param executor: The executor to add this domain bridge to.
   */
  DOMAIN_BRIDGE_PUBLIC
  void add_to_executor(rclcpp::Executor & executor);

  /// Bridge a topic from one domain to another.
  /**
   * \param topic: Name of the topic to bridge.
   * \param type: Name of the topic type (e.g. "example_interfaces/msg/String")
   * \param from_domain_id: Domain ID the bridge will use to subscribe to the topic.
   * \param to_domain_id: Domain ID the bridge will use to publish to the topic.
   * \param options: Options for bridging the topic.
   */
  DOMAIN_BRIDGE_PUBLIC
  void bridge_topic(
    const std::string & topic,
    const std::string & type,
    size_t from_domain_id,
    size_t to_domain_id,
    const TopicBridgeOptions & options = TopicBridgeOptions());

private:
  std::unique_ptr<DomainBridgeImpl> impl_;
};  // class DomainBridge

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__DOMAIN_BRIDGE_HPP_
