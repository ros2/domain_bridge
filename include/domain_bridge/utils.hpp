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

#ifndef DOMAIN_BRIDGE__UTILS_HPP_
#define DOMAIN_BRIDGE__UTILS_HPP_

#include <string>
#include <memory>

#include "rclcpp/duration.hpp"
#include "rclcpp/node.hpp"
#include "rmw/types.h"

#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

namespace utils
{

DOMAIN_BRIDGE_PUBLIC
rclcpp::Duration from_rmw_time(rmw_time_t duration);

DOMAIN_BRIDGE_PUBLIC
rclcpp::Node::SharedPtr
create_node(
  const std::string & name,
  std::size_t domain_id,
  std::shared_ptr<rclcpp::Context> context = nullptr);

}  // namespace utils

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__UTILS_HPP_
