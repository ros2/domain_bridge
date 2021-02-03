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

#ifndef DOMAIN_BRIDGE__EXCEPTIONS_HPP_
#define DOMAIN_BRIDGE__EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

class BridgeExistsError : public std::runtime_error
{
public:
  DOMAIN_BRIDGE_PUBLIC
  BridgeExistsError(size_t domain_id, const std::string topic)
  : std::runtime_error(
      "the topic '" + topic + "' is already bridged from domain " + std::to_string(domain_id))
  {}
};

}  // namespace domain_bridge
#endif  // DOMAIN_BRIDGE__EXCEPTIONS_HPP_
