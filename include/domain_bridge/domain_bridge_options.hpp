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

#include <cstddef>

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
   *    - domain_bridge_id = 0u
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

  /// Get the identifier for the domain bridge.
  DOMAIN_BRIDGE_PUBLIC
  std::size_t
  id() const;

  /// Set the identifier for the domain bridge.
  /**
   * The identifier is added to names of nodes created by the domain bridge.
   */
  DOMAIN_BRIDGE_PUBLIC
  DomainBridgeOptions &
  id(std::size_t id);

private:
  std::size_t id_{0u};
};  // class DomainBridgeOptions

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__DOMAIN_BRIDGE_OPTIONS_HPP_
