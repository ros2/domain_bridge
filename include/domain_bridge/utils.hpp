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

/// Convert a duration represented as rmw_time_t to rclcpp::Duration.
/**
 * \param duration: The duration to be converted.
 * \return The provided duration as an rclcpp::Duration instance.
 */
DOMAIN_BRIDGE_PUBLIC
rclcpp::Duration from_rmw_time(const rmw_time_t duration);

/// Create a ROS context with a given domain ID.
/**
 * \param domain_id: The integral domain ID for the context.
 * \return Shared pointer to an instance of a context with the given domain ID.
 */
DOMAIN_BRIDGE_PUBLIC
rclcpp::Context::SharedPtr
create_context_with_domain_id(const std::size_t domain_id);

/// Create a ROS node with a given name, domain ID, and context instance.
/**
 * \param name: The name of the generated node.
 * \param context: The context instance (if not null) to which this node should belong.
 * \param domain_id: The integral domain ID for the node.
 * \return Shared pointer to an instance of a node with the given constraints.
 */
DOMAIN_BRIDGE_PUBLIC
rclcpp::Node::SharedPtr
create_node(
  const std::string & name,
  const std::size_t domain_id,
  rclcpp::Context::SharedPtr context = nullptr);

/// Get the domain ID of a ROS node.
/**
 * \param node: The node whose domain ID will be obtained.
 * \return The integral domain ID for the argument node.
 */
DOMAIN_BRIDGE_PUBLIC
std::size_t
get_node_domain_id(
  rclcpp::Node & node);

}  // namespace utils

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__UTILS_HPP_
