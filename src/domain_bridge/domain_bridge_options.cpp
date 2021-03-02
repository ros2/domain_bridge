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

#include <cstddef>

#include "domain_bridge/domain_bridge_options.hpp"

namespace domain_bridge
{

std::size_t
DomainBridgeOptions::id() const
{
  return this->id_;
}

DomainBridgeOptions &
DomainBridgeOptions::id(std::size_t id)
{
  this->id_ = id;
  return *this;
}

}  // namespace domain_bridge
