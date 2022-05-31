// Copyright 2018, Bosch Software Innovations GmbH.
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
//
// NOTE: This file was directly copied from rosbag2_transport
// TODO(jacobperron): Replace with upstream implementation when available
//                     https://github.com/ros2/rclcpp/pull/1452

#include "generic_subscription.hpp"

#include <memory>
#include <string>

#include "rclcpp/any_subscription_callback.hpp"
#include "rclcpp/subscription.hpp"

namespace
{
rcl_subscription_options_t rosbag2_get_subscription_options(const rclcpp::QoS & qos)
{
  auto options = rcl_subscription_get_default_options();
  options.qos = qos.get_rmw_qos_profile();
  return options;
}
}  // unnamed namespace

namespace domain_bridge
{

GenericSubscription::GenericSubscription(
  rclcpp::node_interfaces::NodeBaseInterface * node_base,
  const rosidl_message_type_support_t & ts,
  const std::string & topic_name,
  const rclcpp::QoS & qos,
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback)
: SubscriptionBase(
    node_base,
    ts,
    topic_name,
    rosbag2_get_subscription_options(qos),
    true),
  default_allocator_(rcutils_get_default_allocator()),
  callback_(callback),
  qos_(qos)
{}

std::shared_ptr<void> GenericSubscription::create_message()
{
  return create_serialized_message();
}

std::shared_ptr<rclcpp::SerializedMessage> GenericSubscription::create_serialized_message()
{
  return borrow_serialized_message(0);
}

void GenericSubscription::handle_message(
  std::shared_ptr<void> & message, const rclcpp::MessageInfo & message_info)
{
  (void) message_info;
  auto typed_message = std::static_pointer_cast<rclcpp::SerializedMessage>(message);
  callback_(typed_message);
}

void GenericSubscription::handle_loaned_message(
  void * message, const rclcpp::MessageInfo & message_info)
{
  (void) message;
  (void) message_info;
  throw std::runtime_error{"unexpected callback being called"};
}

void GenericSubscription::handle_serialized_message(
  const std::shared_ptr<rclcpp::SerializedMessage> & serialized_message,
  const rclcpp::MessageInfo & message_info)
{
  (void) serialized_message;
  (void) message_info;
  throw std::runtime_error{"unexpected callback being called"};
}

void GenericSubscription::return_message(std::shared_ptr<void> & message)
{
  auto typed_message = std::static_pointer_cast<rclcpp::SerializedMessage>(message);
  return_serialized_message(typed_message);
}

void GenericSubscription::return_serialized_message(
  std::shared_ptr<rclcpp::SerializedMessage> & message)
{
  message.reset();
}

const rclcpp::QoS & GenericSubscription::qos_profile() const
{
  return qos_;
}

std::shared_ptr<rclcpp::SerializedMessage>
GenericSubscription::borrow_serialized_message(size_t capacity)
{
  return std::make_shared<rclcpp::SerializedMessage>(capacity);
}

}  // namespace domain_bridge
