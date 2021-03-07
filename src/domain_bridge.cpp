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

#include <memory>
#include <string>

#include "rclcpp/executors/single_threaded_executor.hpp"

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/domain_bridge_from_yaml.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  if (argc != 2) {
    std::cerr << "Expected one argument" << std::endl <<
      "Usage: domain_bridge [YAML_CONFIG]" << std::endl;
    return 1;
  }
  std::string yaml_config = argv[1];
  std::unique_ptr<domain_bridge::DomainBridge> domain_bridge =
    domain_bridge::domain_bridge_from_yaml(yaml_config);

  rclcpp::executors::SingleThreadedExecutor executor;
  domain_bridge->add_to_executor(executor);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}
