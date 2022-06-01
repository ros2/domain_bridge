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

// cpplint thinks this is a C system header
#include <filesystem>

#include <cstdint>
#include <string>

#include "domain_bridge/domain_bridge_config.hpp"
#include "domain_bridge/topic_bridge_options.hpp"
#include "domain_bridge/qos_options.hpp"

#include "rmw/types.h"

#include "domain_bridge/parse_domain_bridge_yaml_config.hpp"

namespace domain_bridge
{

static QosOptions parse_qos_options(YAML::Node yaml_node, const std::string & file_path)
{
  QosOptions options;

  if (!yaml_node["qos"]) {
    return options;
  }

  if (yaml_node["qos"].Type() != YAML::NodeType::Map) {
    throw YamlParsingError(file_path, "expected map value for 'qos'");
  }

  auto qos_node = yaml_node["qos"];

  if (qos_node["reliability"]) {
    try {
      auto reliability_str = qos_node["reliability"].as<std::string>();
      if ("reliable" == reliability_str) {
        options.reliability(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
      } else if ("best_effort" == reliability_str) {
        options.reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
      } else {
        throw YamlParsingError(
                file_path, "unsupported reliability policy value '" + reliability_str + "'");
      }
    } catch (const YAML::BadConversion &) {
      throw YamlParsingError(file_path, "reliability policy must be a string");
    }
  }

  if (qos_node["durability"]) {
    try {
      auto durability_str = qos_node["durability"].as<std::string>();
      if ("volatile" == durability_str) {
        options.durability(RMW_QOS_POLICY_DURABILITY_VOLATILE);
      } else if ("transient_local" == durability_str) {
        options.durability(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
      } else {
        throw YamlParsingError(
                file_path, "unsupported durability policy value '" + durability_str + "'");
      }
    } catch (const YAML::BadConversion &) {
      throw YamlParsingError(file_path, "durability policy must be a string");
    }
  }

  if (qos_node["history"]) {
    try {
      auto history_str = qos_node["history"].as<std::string>();
      if ("keep_last" == history_str) {
        options.history(RMW_QOS_POLICY_HISTORY_KEEP_LAST);
      } else if ("keep_all" == history_str) {
        options.history(RMW_QOS_POLICY_HISTORY_KEEP_ALL);
      } else {
        throw YamlParsingError(file_path, "unsupported history policy value '" + history_str + "'");
      }
    } catch (const YAML::BadConversion &) {
      throw YamlParsingError(file_path, "history policy must be a string");
    }
  }

  if (qos_node["depth"]) {
    try {
      auto depth = qos_node["depth"].as<std::size_t>();
      options.depth(depth);
    } catch (const YAML::BadConversion &) {
      throw YamlParsingError(file_path, "depth policy must be an integer");
    }
  }

  if (qos_node["deadline"]) {
    // First, try to get deadline as an integer, then check if it is the string 'auto'
    try {
      auto deadline_ns = qos_node["deadline"].as<std::int64_t>();
      options.deadline(deadline_ns);
    } catch (const YAML::BadConversion &) {
      try {
        auto deadline_str = qos_node["deadline"].as<std::string>();
        if ("auto" == deadline_str) {
          options.deadline_auto();
        } else {
          throw YamlParsingError(
                  file_path, "unsupported deadline policy value '" + deadline_str + "'");
        }
      } catch (const YAML::BadConversion &) {
        throw YamlParsingError(file_path, "deadline policy must be an integer or a string");
      }
    }
  }

  if (qos_node["lifespan"]) {
    // First, try to get lifespan as an integer, then check if it is the string 'auto'
    try {
      auto lifespan_ns = qos_node["lifespan"].as<std::int64_t>();
      options.lifespan(lifespan_ns);
    } catch (const YAML::BadConversion &) {
      try {
        auto lifespan_str = qos_node["lifespan"].as<std::string>();
        if ("auto" == lifespan_str) {
          options.lifespan_auto();
        } else {
          throw YamlParsingError(
                  file_path, "unsupported lifespan policy value '" + lifespan_str + "'");
        }
      } catch (const YAML::BadConversion &) {
        throw YamlParsingError(file_path, "lifespan policy must be an integer or a string");
      }
    }
  }

  return options;
}

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
  if (config["mode"]) {
    try {
      auto mode_str = config["mode"].as<std::string>();
      if ("compress" == mode_str) {
        domain_bridge_config.options.mode(DomainBridgeOptions::Mode::Compress);
      } else if ("decompress" == mode_str) {
        domain_bridge_config.options.mode(DomainBridgeOptions::Mode::Decompress);
      } else if ("normal" != mode_str) {
        throw YamlParsingError(file_path, "unsupported mode value '" + mode_str + "'");
      }
    } catch (const YAML::BadConversion &) {
      throw YamlParsingError(file_path, "mode must be an string");
    }
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

      // Parse topic bridge options
      TopicBridgeOptions options;
      if (topic_info["remap"]) {
        options.remap_name(topic_info["remap"].as<std::string>());
      }
      options.qos_options(parse_qos_options(topic_info, file_path));

      // Add topic bridge to config
      domain_bridge_config.topics.push_back({{topic, type, from_domain_id, to_domain_id}, options});
    }
  }

  return domain_bridge_config;
}

}  // namespace domain_bridge
