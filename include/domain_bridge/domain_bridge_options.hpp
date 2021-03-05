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

#ifndef DOMAIN_BRIDGE__DOMAIN_BRIDGE_OPTIONS_HPP_
#define DOMAIN_BRIDGE__DOMAIN_BRIDGE_OPTIONS_HPP_

#include <string>

#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

/// Options for domain bridge.
class DomainBridgeOptions
{
public:
  /// Constructor.
  /**
   *  Default values:
   *
   *    - name = "domain_bridge"
   */
  DOMAIN_BRIDGE_PUBLIC
  DomainBridgeOptions() = default;

  /// Destructor.
  DOMAIN_BRIDGE_PUBLIC
  virtual
  ~DomainBridgeOptions() = default;

  /// Copy constructor.
  DOMAIN_BRIDGE_PUBLIC
  DomainBridgeOptions(const DomainBridgeOptions & other) = default;

  /// Assignment operator.
  DOMAIN_BRIDGE_PUBLIC
  DomainBridgeOptions &
  operator=(const DomainBridgeOptions & other) = default;

  /// Get the name of the domain bridge.
  DOMAIN_BRIDGE_PUBLIC
  std::string
  name() const;

  /// Set the name of the domain bridge.
  /**
   * The name is used to prefix names of nodes created by the domain bridge.
   */
  DOMAIN_BRIDGE_PUBLIC
  DomainBridgeOptions &
  name(std::string name);

private:
  std::string name_{"domain_bridge"};
};  // class DomainBridgeOptions

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__DOMAIN_BRIDGE_OPTIONS_HPP_
