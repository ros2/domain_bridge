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

#ifndef DOMAIN_BRIDGE__PROCESS_CMD_LINE_ARGUMENTS_HPP_
#define DOMAIN_BRIDGE__PROCESS_CMD_LINE_ARGUMENTS_HPP_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "domain_bridge/domain_bridge_config.hpp"

namespace domain_bridge
{
namespace detail
{
static constexpr char kCompressModeStr[] = "compress";
static constexpr char kDecompressModeStr[] = "decompress";
static constexpr char kNormalModeStr[] = "normal";

inline
void
print_help()
{
  std::cerr << "Usage: domain_bridge "
    "[--from FROM_DOMAIN_ID] [--to TO_DOMAIN_ID] [-h] YAML_CONFIG" << std::endl <<
    std::endl <<
    "Arguments:" << std::endl <<
    "    YAML_CONFIG    path to a YAML configuration file." << std::endl <<
    std::endl <<
    "Options:" << std::endl <<
    "    --from FROM_DOMAIN_ID    All data will be bridged from this domain ID.  " << std::endl <<
    "                             This overrides any domain IDs set in the YAML file." <<
    std::endl <<
    "    --to TO_DOMAIN_ID        All data will be bridged to this domain ID.  " << std::endl <<
    "                             This overrides any domain IDs set in the YAML file." <<
    std::endl <<
    "    --mode MODE_STR          Specify the bridge mode, valid values: '" << kCompressModeStr <<
    "', '" << kDecompressModeStr << "' or '" << kNormalModeStr << "'.  " << std::endl <<
    "                             This overrides any mode set in the YAML file." << std::endl <<
    "                             Defaults to '" << kNormalModeStr << "'." <<
    std::endl <<
    "    --help, -h               Print this help message." << std::endl;
}

/// Parse argument string and return as a size_t
/**
 * \param arg argument string
 * \param error_str argument name to output if parsing fails
 * \return size_t value
*/
inline
std::optional<size_t>
parse_size_t_arg(const std::string & arg, const char * error_str)
{
  size_t value;
  std::istringstream iss(arg);
  iss >> value;
  if (iss.fail() || !iss.eof()) {
    std::cerr << "error: Failed to parse " << error_str << " '" <<
      arg << "'" << std::endl;
    return std::nullopt;
  }
  return value;
}
}  // namespace detail

/// Parse command line arguments
/**
 * \param args vector of argument strings
 * \return pair containing domain bridge config and status
*/
inline
std::pair<std::optional<DomainBridgeConfig>, int>
process_cmd_line_arguments(const std::vector<std::string> & args)
{
  if (args.size() < 2) {
    std::cerr << "error: Expected YAML config file" << std::endl;
    detail::print_help();
    return std::make_pair(std::nullopt, 1);
  }
  std::optional<size_t> from_domain_id;
  std::optional<size_t> to_domain_id;
  std::optional<domain_bridge::DomainBridgeOptions::Mode> mode;
  std::optional<std::string> yaml_config;

  for (auto it = ++args.cbegin() /*skip executable name*/; it != args.cend(); ++it) {
    const auto & arg = *it;
    if (arg == "-h" || arg == "--help") {
      detail::print_help();
      return std::make_pair(std::nullopt, 0);
    }
    if (arg == "--from") {
      if (from_domain_id) {
        std::cerr << "error: --from option passed more than once" << std::endl;
        detail::print_help();
        return std::make_pair(std::nullopt, 1);
      }
      ++it;
      from_domain_id = detail::parse_size_t_arg(*it, "FROM_DOMAIN_ID");
      if (!from_domain_id) {
        return std::make_pair(std::nullopt, 1);
      }
      continue;
    }
    if (arg == "--to") {
      if (to_domain_id) {
        std::cerr << "error: --to option passed more than once" << std::endl;
        detail::print_help();
        return std::make_pair(std::nullopt, 1);
      }
      ++it;
      to_domain_id = detail::parse_size_t_arg(*it, "TO_DOMAIN_ID");
      if (!to_domain_id) {
        return std::make_pair(std::nullopt, 1);
      }
      continue;
    }
    if (arg == "--mode") {
      if (mode) {
        std::cerr << "error: --mode option passed more than once" << std::endl;
        detail::print_help();
        return std::make_pair(std::nullopt, 1);
      }
      ++it;
      const auto & mode_str = *it;
      if (mode_str == detail::kCompressModeStr) {
        mode = domain_bridge::DomainBridgeOptions::Mode::Compress;
      } else if (mode_str == detail::kDecompressModeStr) {
        mode = domain_bridge::DomainBridgeOptions::Mode::Decompress;
      } else if (mode_str == detail::kNormalModeStr) {
        mode = domain_bridge::DomainBridgeOptions::Mode::Normal;
      } else {
        std::cerr << "error: Invalid '--mode' option '" <<
          mode_str << "'" << std::endl;
        return std::make_pair(std::nullopt, 1);
      }
      continue;
    }
    if (yaml_config) {
      std::cerr << "error: Can only specify one yaml configuration file '" <<
        *it << "'" << std::endl;
      return std::make_pair(std::nullopt, 1);
    }
    yaml_config = *it;
  }
  if (!yaml_config) {
    std::cerr << "error: Must specify one yaml configuration file" << std::endl;
    return std::make_pair(std::nullopt, 1);
  }
  domain_bridge::DomainBridgeConfig domain_bridge_config =
    domain_bridge::parse_domain_bridge_yaml_config(*yaml_config);

  // Override 'from_domain' and 'to_domain' in config
  if (from_domain_id || to_domain_id) {
    for (auto & topic_option_pair : domain_bridge_config.topics) {
      if (from_domain_id) {
        topic_option_pair.first.from_domain_id = *from_domain_id;
      }
      if (to_domain_id) {
        topic_option_pair.first.to_domain_id = *to_domain_id;
      }
    }
  }
  if (mode) {
    domain_bridge_config.options.mode(*mode);
  }
  return std::make_pair(domain_bridge_config, 0);
}
}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__PROCESS_CMD_LINE_ARGUMENTS_HPP_
