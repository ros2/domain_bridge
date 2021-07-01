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

#include <cstring>
#include <sstream>
#include <string>

#include "rclcpp/executors/single_threaded_executor.hpp"
#include "rcutils/cmdline_parser.h"

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/parse_domain_bridge_yaml_config.hpp"

constexpr char kCompressModeStr[] = "compress";
constexpr char kDecompressModeStr[] = "decompress";
constexpr char kNormalModeStr[] = "normal";

void help()
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

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  if (argc < 2) {
    std::cerr << "error: Expected YAML config file" << std::endl;
    help();
    return 1;
  }

  if (rcutils_cli_option_exist(argv, argv + argc, "-h") ||
    rcutils_cli_option_exist(argv, argv + argc, "--help"))
  {
    help();
    return 0;
  }

  // Get options
  const char * from_domain_opt = rcutils_cli_get_option(argv, argv + argc, "--from");
  const char * to_domain_opt = rcutils_cli_get_option(argv, argv + argc, "--to");
  const char * mode_opt = rcutils_cli_get_option(argv, argv + argc, "--mode");
  std::size_t from_domain = 0u;
  if (from_domain_opt) {
    std::istringstream iss(from_domain_opt);
    iss >> from_domain;
    if (iss.fail() || !iss.eof()) {
      std::cerr << "error: Failed to parse FROM_DOMAIN_ID '" <<
        from_domain_opt << "'" << std::endl;
      return 1;
    }
  }
  std::size_t to_domain = 0u;
  if (to_domain_opt) {
    std::istringstream iss(to_domain_opt);
    iss >> to_domain;
    if (iss.fail() || !iss.eof()) {
      std::cerr << "error: Failed to parse TO_DOMAIN_ID '" <<
        to_domain_opt << "'" << std::endl;
      return 1;
    }
  }
  domain_bridge::DomainBridgeOptions::Mode mode = domain_bridge::DomainBridgeOptions::Mode::Normal;
  if (mode_opt) {
    if (0 == std::strncmp(mode_opt, kCompressModeStr, sizeof(kCompressModeStr))) {
      mode = domain_bridge::DomainBridgeOptions::Mode::Compress;
    } else if (0 == std::strncmp(mode_opt, kDecompressModeStr, sizeof(kDecompressModeStr))) {
      mode = domain_bridge::DomainBridgeOptions::Mode::Decompress;
    } else if (0 == std::strncmp(mode_opt, kNormalModeStr, sizeof(kNormalModeStr))) {
      mode = domain_bridge::DomainBridgeOptions::Mode::Normal;
    } else {
      std::cerr << "error: Invalid '--mode' option '" <<
        mode_opt << "'" << std::endl;
      return 1;
    }
  }

  std::string yaml_config = argv[argc - 1];
  domain_bridge::DomainBridgeConfig domain_bridge_config =
    domain_bridge::parse_domain_bridge_yaml_config(yaml_config);

  // Override 'from_domain' and 'to_domain' in config
  for (auto & topic_option_pair : domain_bridge_config.topics) {
    if (from_domain_opt) {
      topic_option_pair.first.from_domain_id = from_domain;
    }
    if (to_domain_opt) {
      topic_option_pair.first.to_domain_id = to_domain;
    }
  }
  if (mode_opt) {
    domain_bridge_config.options.mode(mode);
  }
  domain_bridge::DomainBridge domain_bridge(domain_bridge_config);

  rclcpp::executors::SingleThreadedExecutor executor;
  domain_bridge.add_to_executor(executor);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}
