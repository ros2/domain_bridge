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

#include <memory>
#include <string>

#include "rclcpp/callback_group.hpp"

#include "domain_bridge/topic_bridge_options.hpp"

namespace domain_bridge
{

std::shared_ptr<rclcpp::CallbackGroup>
TopicBridgeOptions::callback_group() const
{
  return callback_group_;
}

TopicBridgeOptions &
TopicBridgeOptions::callback_group(std::shared_ptr<rclcpp::CallbackGroup> group)
{
  callback_group_ = group;
  return *this;
}

QosOptions
TopicBridgeOptions::qos_options() const
{
  return qos_options_;
}

TopicBridgeOptions &
TopicBridgeOptions::qos_options(const QosOptions & qos_options)
{
  qos_options_ = qos_options;
  return *this;
}

const std::string &
TopicBridgeOptions::remap_name() const
{
  return remap_name_;
}

TopicBridgeOptions &
TopicBridgeOptions::remap_name(const std::string & remap_name)
{
  remap_name_ = remap_name;
  return *this;
}

}  // namespace domain_bridge
