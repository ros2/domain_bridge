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

#ifndef DOMAIN_BRIDGE__UTILS_HPP_
#define DOMAIN_BRIDGE__UTILS_HPP_

#include "rcl/time.h"
#include "rclcpp/duration.hpp"
#include "rmw/types.h"

namespace domain_bridge
{
  namespace utils
    {
      rclcpp::Duration from_rmw_time(rmw_time_t duration)
      {
        constexpr rcl_duration_value_t limit_ns = std::numeric_limits<rcl_duration_value_t>::max();
        constexpr rcl_duration_value_t limit_sec = RCL_NS_TO_S(limit_ns);
        if (duration.sec > limit_sec || duration.nsec > limit_ns) {
          return rclcpp::Duration{limit_ns};
        }
        uint64_t total_ns = RCL_S_TO_NS(duration.sec) + duration.nsec;
        if (total_ns > limit_ns) {
          return rclcpp::Duration{limit_ns};
        }
        return rclcpp::Duration{static_cast<rcl_duration_value_t>(total_ns)};
      }
    }  // namespace utils
}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__UTILS_HPP_
