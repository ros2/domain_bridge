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

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <string>

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/domain_bridge_from_yaml.hpp"
#include "domain_bridge/domain_bridge_options.hpp"

namespace domain_bridge
{

DomainBridge domain_bridge_from_yaml(std::string file_path)
{
  // Check if file exists
  if (!std::filesystem::is_regular_file(file_path)) {
    throw YamlParsingError(file_path, "file does not exist");
  }

  YAML::Node config = YAML::LoadFile(file_path);

  DomainBridgeOptions domain_bridge_options;
  if (config["name"]) {
    domain_bridge_options.name(config["name"].as<std::string>());
  }
  DomainBridge domain_bridge(domain_bridge_options);

  if (config["topics"]) {
    if (config["topics"].Type() != YAML::NodeType::Map) {
      throw YamlParsingError(file_path, "expected map value for 'topics'");
    }
    for (const auto & topic_node : config["topics"]) {
      // Parse required keys for a topic bridge
      const std::string topic = topic_node.first.as<std::string>();
      auto topic_info = topic_node.second;
      if (topic_info.Type() != YAML::NodeType::Map) {
        throw YamlParsingError(file_path, "expected map value for each topic");
      }
      if (!topic_info["type"]) {
        throw YamlParsingError(file_path, "missing 'type' for topic '" + topic + "'");
      }
      const std::string type = topic_info["type"].as<std::string>();
      if (!topic_info["from_domain"]) {
        throw YamlParsingError(file_path, "missing 'from_domain' for topic '" + topic + "'");
      }
      const std::size_t from_domain_id = topic_info["from_domain"].as<std::size_t>();
      if (!topic_info["to_domain"]) {
        throw YamlParsingError(file_path, "missing 'to_domain' for topic '" + topic + "'");
      }
      const std::size_t to_domain_id = topic_info["to_domain"].as<std::size_t>();

      // Create topic bridge
      domain_bridge.bridge_topic(topic, type, from_domain_id, to_domain_id);
    }
  }

  return domain_bridge;
}

}  // namespace domain_bridge
