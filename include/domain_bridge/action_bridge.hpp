#ifndef DOMAIN_BRIDGE__ACTION_BRIDGE_HPP_
#define DOMAIN_BRIDGE__ACTION_BRIDGE_HPP_

#include <cstddef>
#include <string>

namespace domain_bridge
{

/// Info and ROS entities related to a action bridge
struct ActionBridge
{
  /// Name of the bridged action
  std::string action_name;

  /// Name of the message type sent on the action
  std::string type_name;

  /// Domain ID that the subscription uses
  std::size_t from_domain_id;

  /// Domain ID that the publisher uses
  std::size_t to_domain_id;

  /// Less-than operator.
  /**
   * Sort by 'from_domain_id',
   *   then by 'to_domain_id',
   *   then by 'action_name',
   *   then by 'type_name'
   */
  bool operator<(const ActionBridge & other) const
  {
    if (from_domain_id < other.from_domain_id) {
      return true;
    }
    if (from_domain_id > other.from_domain_id) {
      return false;
    }
    if (to_domain_id < other.to_domain_id) {
      return true;
    }
    if (to_domain_id > other.to_domain_id) {
      return false;
    }
    int name_compare = action_name.compare(other.action_name);
    if (name_compare < 0) {
      return true;
    }
    if (name_compare > 0) {
      return false;
    }
    int type_compare = type_name.compare(other.type_name);
    if (type_compare < 0) {
      return true;
    }
    return false;
  }
};

}  // namespace domain_bridge

#endif  // DOMAIN_BRIDGE__ACTION_BRIDGE_HPP_
