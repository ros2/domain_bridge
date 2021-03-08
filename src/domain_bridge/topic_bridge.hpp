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

#ifndef DOMAIN_BRIDGE__TOPIC_BRIDGE_HPP_
#define DOMAIN_BRIDGE__TOPIC_BRIDGE_HPP_

#include <cstddef>
#include <memory>
#include <string>

#include "generic_publisher.hpp"
#include "generic_subscription.hpp"

namespace domain_bridge
{

/// Info and ROS entities related to a topic bridge
struct TopicBridge
{
  /// Domain ID that the subscription uses
  std::size_t from_domain_id;

  /// Domain ID that the publisher uses
  std::size_t to_domain_id;

  /// Name of the bridged topic
  std::string topic_name;

  /// Name of the message type sent on the topic
  std::string type_name;

  /// The publisher half of the bridge
  std::shared_ptr<GenericPublisher> publisher;

  /// The subscription half of the bridge
  std::shared_ptr<GenericSubscription> subscription;
};

/// Compare two TopicBridge objects
struct TopicBridgeCompare
{
  bool operator()(const TopicBridge & lhs, const TopicBridge & rhs) const
  {
    // Sort by 'from_domain_id',
    //   then by 'to_domain_id',
    //   then by 'topic_name',
    //   then by 'type_name'
    if (lhs.from_domain_id < rhs.from_domain_id) {
      return true;
    }
    if (lhs.from_domain_id != rhs.from_domain_id) {
      return false;
    }
    if (lhs.to_domain_id < rhs.to_domain_id) {
      return true;
    }
    if (lhs.to_domain_id != rhs.to_domain_id) {
      return false;
    }
    int name_compare = lhs.topic_name.compare(rhs.topic_name);
    if (name_compare < 0) {
      return true;
    }
    if (name_compare != 0) {
      return false;
    }
    int type_compare = lhs.type_name.compare(rhs.type_name);
    if (type_compare < 0) {
      return true;
    }
    return false;
  }
};

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__TOPIC_BRIDGE_HPP_
