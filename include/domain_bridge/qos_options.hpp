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

#ifndef DOMAIN_BRIDGE__QOS_OPTIONS_HPP_
#define DOMAIN_BRIDGE__QOS_OPTIONS_HPP_

#include <optional>

#include "rmw/types.h"

#include "domain_bridge/visibility_control.hpp"

namespace domain_bridge
{

/// Options for overriding QoS settings.
class QosOptions
{
public:
  /// Constructor.
  /**
   *  Default values:
   *
   *    - reliability = nullopt_t (detect automatically)
   *    - durability = nullopt_t (detect automatically)
   *    - history = RMW_QOS_POLICY_HISTORY_KEEP_LAST
   *    - depth = 10
   *    - deadline = 0 (RMW default)
   *    - lifespan = 0 (RMW default)
   */
  DOMAIN_BRIDGE_PUBLIC
  QosOptions() = default;

  /// Get reliability.
  DOMAIN_BRIDGE_PUBLIC
  std::optional<rmw_qos_reliability_policy_t>
  reliability() const;

  /// Set reliability.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  reliability(rmw_qos_reliability_policy_t reliability);

  /// Get durability.
  DOMAIN_BRIDGE_PUBLIC
  std::optional<rmw_qos_durability_policy_t>
  durability() const;

  /// Set durability.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  durability(rmw_qos_durability_policy_t durability);

  /// Get history.
  DOMAIN_BRIDGE_PUBLIC
  rmw_qos_history_policy_t
  history() const;

  /// Set history.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  history(rmw_qos_history_policy_t history);

  /// Get history depth.
  DOMAIN_BRIDGE_PUBLIC
  std::size_t
  depth() const;

  /// Set history depth.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  depth(const std::size_t & depth);

  /// Get deadline in nanoseconds.
  /**
   * If nullopt_t is returned, then use automatic matching.
   */
  DOMAIN_BRIDGE_PUBLIC
  std::optional<std::int64_t>
  deadline() const;

  /// Set deadline.
  /**
   * \param deadline: number of nanoseconds.
   *   If zero, then use the RMW default policy.
   *   If negative, then it is treated an "infinite" (max number of nanoseconds).
   */
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  deadline(const std::int64_t & deadline);

  /// Set deadline policy to automatically match available publishers.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  deadline_auto();

  /// Get lifespan in nanoseconds.
  /**
   * If nullopt_t is returned, then use automatic matching.
   */
  DOMAIN_BRIDGE_PUBLIC
  std::optional<std::int64_t>
  lifespan() const;

  /// Set lifespan in nanoseconds.
  /**
   * \param lifespan: number of nanoseconds.
   *   If zero, then use the RMW default policy.
   *   If negative, then it is treated an "infinite" (max number of nanoseconds).
   */
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  lifespan(const std::int64_t & lifespan);

  /// Set lifespan policy to automatically match available publihsers.
  DOMAIN_BRIDGE_PUBLIC
  QosOptions &
  lifespan_auto();

private:
  std::optional<rmw_qos_reliability_policy_t> reliability_;
  std::optional<rmw_qos_durability_policy_t> durability_;
  rmw_qos_history_policy_t history_{RMW_QOS_POLICY_HISTORY_KEEP_LAST};
  std::size_t depth_{10};
  std::optional<std::int64_t> deadline_{0};
  std::optional<std::int64_t> lifespan_{0};
};  // class QosOptions

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__QOS_OPTIONS_HPP_
