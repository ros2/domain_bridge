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

#include "domain_bridge/domain_bridge_config.hpp"
#include "domain_bridge/parse_domain_bridge_yaml_config.hpp"

namespace domain_bridge
{

DomainBridgeConfig parse_domain_bridge_yaml_config(std::filesystem::path file_path)
{
  // Check if file exists
  if (!std::filesystem::is_regular_file(file_path)) {
    throw YamlParsingError(file_path, "file does not exist");
  }

  YAML::Node config = YAML::LoadFile(file_path);

  DomainBridgeConfig domain_bridge_config;
  if (config["name"]) {
    domain_bridge_config.options.name(config["name"].as<std::string>());
  }

  // Check for any default domain IDs
  bool is_default_from_domain = false;
  bool is_default_to_domain = false;
  std::size_t default_from_domain = 0u;
  std::size_t default_to_domain = 0u;
  if (config["from_domain"]) {
    default_from_domain = config["from_domain"].as<std::size_t>();
    is_default_from_domain = true;
  }
  if (config["to_domain"]) {
    default_to_domain = config["to_domain"].as<std::size_t>();
    is_default_to_domain = true;
  }

  if (config["topics"]) {
    if (config["topics"].Type() != YAML::NodeType::Map) {
      throw YamlParsingError(file_path, "expected map value for 'topics'");
    }
    for (const auto & topic_node : config["topics"]) {
      // Parse keys for a topic bridge
      const std::string topic = topic_node.first.as<std::string>();

      auto topic_info = topic_node.second;
      if (topic_info.Type() != YAML::NodeType::Map) {
        throw YamlParsingError(file_path, "expected map value for each topic");
      }

      if (!topic_info["type"]) {
        throw YamlParsingError(file_path, "missing 'type' for topic '" + topic + "'");
      }
      const std::string type = topic_info["type"].as<std::string>();

      std::size_t from_domain_id = default_from_domain;
      if (topic_info["from_domain"]) {
        from_domain_id = topic_info["from_domain"].as<std::size_t>();
      } else {
        if (!is_default_from_domain) {
          throw YamlParsingError(file_path, "missing 'from_domain' for topic '" + topic + "'");
        }
      }

      std::size_t to_domain_id = default_to_domain;
      if (topic_info["to_domain"]) {
        to_domain_id = topic_info["to_domain"].as<std::size_t>();
      } else {
        if (!is_default_to_domain) {
          throw YamlParsingError(file_path, "missing 'to_domain' for topic '" + topic + "'");
        }
      }

      // Add topic bridge to config
      domain_bridge_config.topics.push_back({{topic, type, from_domain_id, to_domain_id}, {}});
    }
  }

  return domain_bridge_config;
}

}  // namespace domain_bridge
