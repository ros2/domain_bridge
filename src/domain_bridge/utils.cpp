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

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "rcl/node.h"
#include "rcl/node_options.h"
#include "rcl/time.h"
#include "rclcpp/context.hpp"
#include "rclcpp/init_options.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/node_interfaces/node_base.hpp"

#include "domain_bridge/utils.hpp"

namespace domain_bridge
{

namespace utils
{

rclcpp::Duration from_rmw_time(rmw_time_t duration)
{
  constexpr rcl_duration_value_t limit_ns = std::numeric_limits<rcl_duration_value_t>::max();
  constexpr rcl_duration_value_t limit_sec = RCL_NS_TO_S(limit_ns);
  if (duration.sec > limit_sec || duration.nsec > limit_ns) {
    return rclcpp::Duration{limit_ns};
  }
  uint64_t total_ns = RCL_S_TO_NS(duration.sec) + duration.nsec;
  if (total_ns > limit_ns) {
    return rclcpp::Duration{limit_ns};
  }
  return rclcpp::Duration{static_cast<rcl_duration_value_t>(total_ns)};
}

rclcpp::Node::SharedPtr
create_node(
  const std::string & name,
  std::size_t domain_id,
  std::shared_ptr<rclcpp::Context> context)
{
  if (context == nullptr) {
    context = std::make_shared<rclcpp::Context>();
  }
  rclcpp::InitOptions init_options;
  init_options.auto_initialize_logging(false);
  context->init(0, nullptr, init_options);

  rclcpp::NodeOptions node_options;
  node_options.context(context)
  .use_global_arguments(false)
  .start_parameter_services(false)
  .start_parameter_event_publisher(false);

  auto node = std::make_shared<rclcpp::Node>(name, node_options);
  auto node_base_interface = node->get_node_base_interface();
  rcl_node_t * rcl_node_handle = node_base_interface->get_rcl_node_handle();
  // Hacky work-around because setting domain ID is not a feature in the rclcpp layer
  const_cast<rcl_node_options_t *>(
    rcl_node_get_options(rcl_node_handle))->domain_id = domain_id;

  return node;
}

std::size_t
get_domain_id_from_node(
  rclcpp::Node & node)
{
  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_base_interface =
    node.get_node_base_interface();
  rcl_node_t * rcl_node_handle = node_base_interface->get_rcl_node_handle();

  std::size_t domain_id;
  rcl_ret_t ret = rcl_node_get_domain_id(rcl_node_handle, &domain_id);
  if (ret == RCL_RET_OK) {
    return domain_id;
  } else if (ret == RCL_RET_NODE_INVALID) {
    throw std::runtime_error("[get_domain_id_from_node] Node invalid.");
  } else if (ret == RCL_RET_INVALID_ARGUMENT) {
    throw std::runtime_error("[get_domain_id_from_node] Invalid argument.");
  } else {
    throw std::runtime_error("[get_domain_id_from_node] Unspecified error.");
  }
  // return (node.get_node_options().get_rcl_node_options())->domain_id;
}

}  // namespace utils

}  // namespace domain_bridge
