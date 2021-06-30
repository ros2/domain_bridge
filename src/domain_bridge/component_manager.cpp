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

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ament_index_cpp/get_resource.hpp"
#include "class_loader/class_loader.hpp"
#include "rcpputils/filesystem_helper.hpp"
#include "rcpputils/split.hpp"

using namespace std::placeholders;

namespace domain_bridge
{

void
ComponentManager::OnLoadNode(
  const std::shared_ptr<rmw_request_id_t> request_header,
  const std::shared_ptr<LoadNode::Request> request,
  std::shared_ptr<LoadNode::Response> response)
{
  (void) request_header;

  try {
    auto resources = get_component_resources(request->package_name);

    for (const auto & resource : resources) {
      if (resource.first != request->plugin_name) {
        continue;
      }
      auto factory = create_component_factory(resource);

      if (factory == nullptr) {
        continue;
      }

      std::vector<rclcpp::Parameter> parameters;
      for (const auto & p : request->parameters) {
        parameters.push_back(rclcpp::Parameter::from_parameter_msg(p));
      }

      std::vector<std::string> remap_rules;
      remap_rules.reserve(request->remap_rules.size() * 2 + 1);
      remap_rules.push_back("--ros-args");
      for (const std::string & rule : request->remap_rules) {
        remap_rules.push_back("-r");
        remap_rules.push_back(rule);
      }

      if (!request->node_name.empty()) {
        remap_rules.push_back("-r");
        remap_rules.push_back("__node:=" + request->node_name);
      }

      if (!request->node_namespace.empty()) {
        remap_rules.push_back("-r");
        remap_rules.push_back("__ns:=" + request->node_namespace);
      }

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

      auto node_id = GetUniqueId();
      SetUniqueId(node_id + 1);

      if (0 == node_id) {
        // This puts a technical limit on the number of times you can add a component.
        // But even if you could add (and remove) them at 1 kHz (very optimistic rate)
        // it would still be a very long time before you could exhaust the pool of id's:
        //   2^64 / 1000 times per sec / 60 sec / 60 min / 24 hours / 365 days = 584,942,417 years
        // So around 585 million years. Even at 1 GHz, it would take 585 years.
        // I think it's safe to avoid trying to handle overflow.
        // If we roll over then it's most likely a bug.
        throw std::overflow_error("exhausted the unique ids for components in this process");
      }

      try {
        SetNodeWrapper(node_id, factory->create_node_instance(options));
      } catch (const std::exception & ex) {
        // In the case that the component constructor throws an exception,
        // rethrow into the following catch block.
        throw rclcpp_components::ComponentManagerException(
                "Component constructor threw an exception: " + std::string(ex.what()));
      } catch (...) {
        // In the case that the component constructor throws an exception,
        // rethrow into the following catch block.
        throw rclcpp_components::ComponentManagerException(
                "Component constructor threw an exception");
      }

      auto node = GetNodeWrapper(node_id).get_node_base_interface();
      if (auto exec = GetExecutor().lock()) {
        exec->add_node(node, true);
      }
      response->full_node_name = node->get_fully_qualified_name();
      response->unique_id = node_id;
      response->success = true;
      return;
    }
    RCLCPP_ERROR(
      get_logger(), "Failed to find class with the requested plugin name '%s' in "
      "the loaded library",
      request->plugin_name.c_str());
    response->error_message = "Failed to find class with the requested plugin name.";
    response->success = false;
  } catch (const rclcpp_components::ComponentManagerException & ex) {
    RCLCPP_ERROR(get_logger(), "%s", ex.what());
    response->error_message = ex.what();
    response->success = false;
  }
}

}  // namespace domain_bridge
