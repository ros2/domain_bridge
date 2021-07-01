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

#ifndef DOMAIN_BRIDGE__COMPONENT_MANAGER_HPP__
#define DOMAIN_BRIDGE__COMPONENT_MANAGER_HPP__

#include "rclcpp_components/component_manager.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{
/// ComponentManager handles the services to load, unload, and get the list of loaded components.
class ComponentManager : public rclcpp_components::ComponentManager
{
  using rclcpp_components::ComponentManager::ComponentManager;

protected:
  DOMAIN_BRIDGE_PUBLIC
  virtual rclcpp::NodeOptions
  SetNodeOptions(
    std::vector<rclcpp::Parameter> parameters,
    std::vector<std::string> remap_rules,
    const std::shared_ptr<LoadNode::Request> request);

private:
  std::map<std::size_t, std::shared_ptr<rclcpp::Context>> contexts_;
};

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__COMPONENT_MANAGER_HPP__
