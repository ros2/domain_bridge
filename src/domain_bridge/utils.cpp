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

#include "rcl/allocator.h"
#include "rcl/init_options.h"
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

rclcpp::Duration from_rmw_time(const rmw_time_t duration)
{
  // From https://github.com/ros2/rclcpp/pull/1467/commits/b6bfc5868035b3cc8774275e08c6194b0f484e37
  constexpr rcl_duration_value_t limit_ns = std::numeric_limits<rcl_duration_value_t>::max();
  constexpr rcl_duration_value_t limit_sec = RCL_NS_TO_S(limit_ns);
  if (duration.sec > limit_sec || duration.nsec > limit_ns) {
    return rclcpp::Duration{limit_ns};
  }
  const uint64_t total_ns = RCL_S_TO_NS(duration.sec) + duration.nsec;
  if (total_ns > limit_ns) {
    return rclcpp::Duration{limit_ns};
  }
  return rclcpp::Duration{static_cast<rcl_duration_value_t>(total_ns)};
}

rclcpp::Context::SharedPtr
create_context_with_domain_id(const std::size_t domain_id)
{
  rcl_init_options_t rcl_init_options = rcl_get_zero_initialized_init_options();
  rcl_ret_t ret = rcl_init_options_init(
    &rcl_init_options, rcl_get_default_allocator());
  if (RCL_RET_OK != ret) {
    std::runtime_error("Failed to initialize rcl_init_options");
  }

  ret = rcl_init_options_set_domain_id(&rcl_init_options, domain_id);
  if (RCL_RET_OK != ret) {
    std::runtime_error("Failed to set domain ID to rcl_init_options");
  }

  rclcpp::InitOptions init_options(rcl_init_options);
  init_options.auto_initialize_logging(false);

  auto context = std::make_shared<rclcpp::Context>();
  context->init(0, nullptr, init_options);
  return context;
}

rclcpp::Node::SharedPtr
create_node(
  const std::string & name,
  const std::size_t domain_id,
  rclcpp::Context::SharedPtr context)
{
  if (context == nullptr) {
    context = create_context_with_domain_id(domain_id);
  }

  rclcpp::NodeOptions node_options;
  node_options.context(context)
  .use_global_arguments(false)
  .start_parameter_services(false)
  .start_parameter_event_publisher(false);

  return std::make_shared<rclcpp::Node>(name, node_options);
}

std::size_t
get_node_domain_id(
  rclcpp::Node & node)
{
  // Need separate instance for InitOptions so that it stays in scope till domain ID is obtained
  const rclcpp::InitOptions init_options =
    node.get_node_base_interface()->get_context()->get_init_options();
  const rcl_init_options_t * rcl_init_options = init_options.get_rcl_init_options();

  std::size_t domain_id;
  // const_cast is safe because `rcl_init_options_get_domain_id` only reads the input structure
  const rcl_ret_t ret =
    rcl_init_options_get_domain_id(
    const_cast<rcl_init_options_t *>(rcl_init_options),
    &domain_id);
  if (RCL_RET_OK != ret) {
    throw std::runtime_error("Failed to get domain ID from rcl_init_options");
  }

  return domain_id;
}

}  // namespace utils

}  // namespace domain_bridge
