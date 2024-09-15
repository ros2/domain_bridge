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

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "rclcpp/node.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

// This is a non-header file meant to be included in `domain_bridge.hpp`.
// It contains the implementation of the DomainBridge::bridge_action() template method.

// Conditionally include domain_bridge.hpp to help intellisense.
#ifndef DOMAIN_BRIDGE__DOMAIN_BRIDGE_HPP_
# include "domain_bridge/domain_bridge.hpp"
#endif

using namespace std::chrono_literals;

namespace domain_bridge
{

namespace detail
{
// action_name, from_domain_id, to_domain_id
using ActionBridge = std::tuple<std::string, size_t, size_t>;

bool
is_bridging_action(
  const DomainBridgeImpl & impl, const ActionBridge & action_bridge);

void
add_action_bridge(
  DomainBridgeImpl & impl,
  const rclcpp::Node::SharedPtr & node,
  ActionBridge action_bridge,
  std::function<std::shared_ptr<rclcpp_action::ServerBase>()> create_action,
  std::shared_ptr<rclcpp_action::ClientBase> client);

rclcpp::Node::SharedPtr
get_node_for_domain(DomainBridgeImpl & impl, std::size_t domain_id);

const std::string &
get_node_name(const DomainBridgeImpl & impl);
}  // namespace detail

/// Bridge a action from one domain to another.
/**
 * \param action_bridge: Struct containing info about the action to bridge.
 * \param options: Options for bridging the topic.
 */
template<typename ActionT>
void
DomainBridge::bridge_action(
  const std::string & action_name,
  size_t from_domain_id,
  size_t to_domain_id,
  const ActionBridgeOptions & options)
{
  using ResultT = typename ActionT::Result;
  using FeedbackT = typename ActionT::Feedback;
  using GoalT = typename ActionT::Goal;

  const auto & node_name = detail::get_node_name(*impl_);
  // Validate action name
  const std::string & resolved_action_name = rclcpp::expand_topic_or_service_name(
    action_name, node_name, "/", true);

  // If a remap name is provided, then validate it
  // otherwise "remap" to the same name
  std::string action_remapped = resolved_action_name;
  if (!options.remap_name().empty()) {
    action_remapped = rclcpp::expand_topic_or_service_name(
      options.remap_name(), node_name, "/", true);
  }

  std::tuple<std::string, size_t, size_t> action_bridge = {
    resolved_action_name,
    from_domain_id,
    to_domain_id
  };

  // Check if already bridged
  if (detail::is_bridging_action(*impl_, action_bridge))
  {
    std::cerr << "Action '" << resolved_action_name << "'" << " already bridged from domain " <<
      std::to_string(from_domain_id) << " to domain " << std::to_string(to_domain_id) <<
      ", ignoring" << std::endl;
    return;
  }

  rclcpp::Node::SharedPtr from_domain_node = detail::get_node_for_domain(*impl_, from_domain_id);
  rclcpp::Node::SharedPtr to_domain_node = detail::get_node_for_domain(*impl_, to_domain_id);

  // Create a client for the 'from_domain'
  auto client = rclcpp_action::create_client<ActionT>(
      from_domain_node,
      resolved_action_name,
      options.callback_group());

  auto handle_goal =
    [client, to_domain_node](
      const rclcpp_action::GoalUUID & /*uuid*/,
      std::shared_ptr<const GoalT> /*goal*/) -> rclcpp_action::GoalResponse
    {
      RCLCPP_INFO(to_domain_node->get_logger(), "Received goal request");
      // Accept all goals
      return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    };
  auto handle_cancel =
    [client, to_domain_node](const std::shared_ptr<rclcpp_action::ServerGoalHandle<ActionT>> /*goal_handle*/) {
      RCLCPP_INFO(to_domain_node->get_logger(), "Received request to cancel goal");
      // Accept all cancel requests
      return rclcpp_action::CancelResponse::ACCEPT;
    };
  auto execute =
    [client, to_domain_node](const std::shared_ptr<rclcpp_action::ServerGoalHandle<ActionT>> goal_handle)
    {
      RCLCPP_INFO(to_domain_node->get_logger(), "Executing goal forwarding");

    // Wait for the backend action server to be available
    if (!client->wait_for_action_server(5s)) {
      RCLCPP_ERROR(to_domain_node->get_logger(), "Backend action server not available");
      goal_handle->abort(std::make_shared<ResultT>());
      return;
    }

    // Create a goal message for the backend action server
    auto goal_msg = GoalT();
    goal_msg = *goal_handle->get_goal();

    // Set up goal options, including callbacks
    using SendGoalOptions = typename rclcpp_action::Client<ActionT>::SendGoalOptions;
    auto send_goal_options = SendGoalOptions();

    // Goal response callback
    send_goal_options.goal_response_callback =
      [goal_handle, to_domain_node](typename rclcpp_action::ClientGoalHandle<ActionT>::SharedPtr backend_goal_handle) {
        if (!backend_goal_handle) {
          RCLCPP_ERROR(to_domain_node->get_logger(), "Backend server rejected the goal");
          goal_handle->abort(std::make_shared<ResultT>());
        } else {
          RCLCPP_INFO(to_domain_node->get_logger(), "Goal accepted by backend server");
        }
      };

    // Feedback callback
    send_goal_options.feedback_callback =
      [to_domain_node, goal_handle](typename rclcpp_action::ClientGoalHandle<ActionT>::SharedPtr,
                          const std::shared_ptr<const FeedbackT> feedback) {
        RCLCPP_INFO(to_domain_node->get_logger(), "Forwarding feedback");
        auto feedback_copy = std::make_shared<FeedbackT>(*feedback);
        goal_handle->publish_feedback(feedback_copy);
      };

    // Result callback
    send_goal_options.result_callback =
      [to_domain_node, goal_handle](const typename rclcpp_action::ClientGoalHandle<ActionT>::WrappedResult & result) {
        RCLCPP_INFO(to_domain_node->get_logger(), "Received result from backend server");
        switch (result.code) {
          case rclcpp_action::ResultCode::SUCCEEDED:
            goal_handle->succeed(result.result);
            break;
          case rclcpp_action::ResultCode::ABORTED:
            RCLCPP_ERROR(to_domain_node->get_logger(), "Goal was aborted by backend server");
            goal_handle->abort(result.result);
            break;
          case rclcpp_action::ResultCode::CANCELED:
            RCLCPP_INFO(to_domain_node->get_logger(), "Goal was canceled by backend server");
            goal_handle->canceled(result.result);
            break;
          default:
            RCLCPP_ERROR(to_domain_node->get_logger(), "Unknown result code from backend server");
            goal_handle->abort(result.result);
            break;
        }
      };

    // Send the goal to the backend action server
    auto future_goal_handle = client->async_send_goal(goal_msg, send_goal_options);

    // Handle cancellation in a loop
    while (rclcpp::ok()) {
      // Check if the goal has been canceled
      if (goal_handle->is_canceling()) {
        RCLCPP_INFO(to_domain_node->get_logger(), "Forwarding cancel request to backend server");
        auto cancel_future = client->async_cancel_all_goals();

        // We don't use spin_until_future_complete, so we just log and return
        RCLCPP_INFO(to_domain_node->get_logger(), "Cancel request sent to backend server");
        goal_handle->canceled(std::make_shared<ResultT>());
        return;
      }

      // Sleep briefly to prevent busy waiting
      std::this_thread::sleep_for(100ms);

      // Check if the goal is done (successfully or not)
      if (goal_handle->is_executing() || goal_handle->is_active()) {
        continue;  // Goal is still executing
      } else {
        break;  // Goal is done
      }
    }
    };
  auto handle_accept =
    [client, execute = std::move(execute)](const std::shared_ptr<rclcpp_action::ServerGoalHandle<ActionT>> goal_handle) {
      std::thread{std::bind(execute, goal_handle)}.detach();
    };

  // A action for the 'to_domain' will be created when we detect there is a action running in
  // the 'from_domain'.
  auto create_action_cb = [
    to_domain_node = std::move(to_domain_node),
    action_remapped = std::move(action_remapped),
    handle_goal = std::move(handle_goal),
    handle_cancel = std::move(handle_cancel),
    handle_accept = std::move(handle_accept),
    options = std::move(options)]()
  {
    return rclcpp_action::create_server<ActionT>(
      to_domain_node,
      action_remapped,
      handle_goal,
      handle_cancel,
      handle_accept);
  };

  detail::add_action_bridge(
    *impl_, from_domain_node, action_bridge, create_action_cb, client);
}

}  // namespace domain_bridge
