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

#include "rclcpp/executors/single_threaded_executor.hpp"

#include "domain_bridge/domain_bridge.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // TODO(jacobperron): Configure via YAML file
  domain_bridge::DomainBridge domain_bridge;
  domain_bridge.bridge_topic("image", "sensor_msgs/msg/Image", 5, 10);

  auto executor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  domain_bridge.add_to_executor(executor);
  executor->spin();

  rclcpp::shutdown();

  return 0;
}
