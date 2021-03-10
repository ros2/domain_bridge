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

#ifndef DOMAIN_BRIDGE__DOMAIN_BRIDGE_FROM_YAML_HPP_
#define DOMAIN_BRIDGE__DOMAIN_BRIDGE_FROM_YAML_HPP_

#include <filesystem>
#include <string>

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

class YamlParsingError : public std::runtime_error
{
public:
  DOMAIN_BRIDGE_PUBLIC
  YamlParsingError(const std::filesystem::path file_path, const std::string message)
  : std::runtime_error("error parsing the file '" + file_path.string() + "': " + message)
  {}
};

/// Create a DomainBridge from a YAML file.
/**
 * The YAML file may contain the following optional keys:
 *
 * - name: Name of the domain bridge
 * - topics: A map of topic names to a map of topic bridge information
 *
 * For each topic bridge, the follow information is required, in the form of YAML keys:
 *
 * - type: Message type to bridge
 * - from_domain: Subscribe to the topic on this domain ID
 * - to_domain: Publish to the topic on this domain ID
 *
 * For example,
 *
 * ```yaml
 * name: my_bridge
 * topics:
 *   chatter:
 *     type: example_interfaces/msg/String
 *     from_domain: 2
 *     to_domain: 3
 *   clock:
 *     type: rosgraph_msgs/msg/Clock
 *     from_domain: 2
 *     to_domain: 3
 * ```
 *
 * \param file_path: Path to the YAML file.
 * \return A pointer to an initialized DomainBridge that was configured based on input YAML file.
 * \throws domain_bridge::YamlParsingError if there was an error parsing the YAML file,
 *   for example, if a required key is missing.
 */
DOMAIN_BRIDGE_PUBLIC
DomainBridge domain_bridge_from_yaml(std::filesystem::path file_path);

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__DOMAIN_BRIDGE_FROM_YAML_HPP_
