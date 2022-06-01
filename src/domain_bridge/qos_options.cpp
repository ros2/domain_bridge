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

#include <optional>

#include <cstdint>

#include "domain_bridge/qos_options.hpp"

namespace domain_bridge
{

std::optional<rmw_qos_reliability_policy_t>
QosOptions::reliability() const
{
  return reliability_;
}

QosOptions &
QosOptions::reliability(rmw_qos_reliability_policy_t reliability)
{
  reliability_.emplace(reliability);
  return *this;
}

std::optional<rmw_qos_durability_policy_t>
QosOptions::durability() const
{
  return durability_;
}

QosOptions &
QosOptions::durability(rmw_qos_durability_policy_t durability)
{
  durability_.emplace(durability);
  return *this;
}

rmw_qos_history_policy_t
QosOptions::history() const
{
  return history_;
}

QosOptions &
QosOptions::history(rmw_qos_history_policy_t history)
{
  history_ = history;
  return *this;
}

std::size_t
QosOptions::depth() const
{
  return depth_;
}

QosOptions &
QosOptions::depth(const std::size_t & depth)
{
  depth_ = depth;
  return *this;
}

std::optional<std::int64_t>
QosOptions::deadline() const
{
  return deadline_;
}

QosOptions &
QosOptions::deadline(const std::int64_t & deadline)
{
  deadline_.emplace(deadline);
  return *this;
}

QosOptions &
QosOptions::deadline_auto()
{
  deadline_.reset();
  return *this;
}

std::optional<std::int64_t>
QosOptions::lifespan() const
{
  return lifespan_;
}

QosOptions &
QosOptions::lifespan(const std::int64_t & lifespan)
{
  lifespan_.emplace(lifespan);
  return *this;
}

QosOptions &
QosOptions::lifespan_auto()
{
  lifespan_.reset();
  return *this;
}

}  // namespace domain_bridge
