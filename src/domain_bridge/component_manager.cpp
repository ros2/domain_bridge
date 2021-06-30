// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#include "domain_bridge/component_manager.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace std::placeholders;

namespace domain_bridge
{

rclcpp::NodeOptions
ComponentManager::SetNodeOptions(
  std::vector<rclcpp::Parameter> parameters,
  std::vector<std::string> remap_rules,
  const std::shared_ptr<LoadNode::Request> request)
{
  auto options = rclcpp::NodeOptions()
    .use_global_arguments(false)
    .parameter_overrides(parameters)
    .arguments(remap_rules);

  for (const auto & a : request->extra_arguments) {
    const rclcpp::Parameter extra_argument = rclcpp::Parameter::from_parameter_msg(a);
    if (extra_argument.get_name() == "use_intra_process_comms") {
      if (extra_argument.get_type() != rclcpp::ParameterType::PARAMETER_BOOL) {
        throw rclcpp_components::ComponentManagerException(
                "Extra component argument 'use_intra_process_comms' must be a boolean");
      }
      options.use_intra_process_comms(extra_argument.get_value<bool>());
    } else if (extra_argument.get_name() == "domain_id") {
      if (extra_argument.get_type() != rclcpp::ParameterType::PARAMETER_INTEGER) {
        throw rclcpp_components::ComponentManagerException(
                "Extra component argument 'domain_id' must be an integer");
      }
      std::size_t domain_id = extra_argument.get_value<std::size_t>();
      auto context = std::make_shared<rclcpp::Context>();
      rclcpp::InitOptions init_options;
      init_options.auto_initialize_logging(false).set_domain_id(domain_id);
      context->init(0, nullptr, init_options);
      options.context(context);
    }
  }

  return options;
}

}  // namespace domain_bridge
