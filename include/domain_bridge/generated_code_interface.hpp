#pragma once

#include <string>
#include "domain_bridge/domain_bridge.hpp"

namespace domain_bridge {

bool add_service_bridge_by_name(
    DomainBridge & domain_bridge,
    const std::string& service_name,
    const std::string& service_type,
    size_t from_domain_id,
    size_t to_domain_id,
    const ServiceBridgeOptions & options = ServiceBridgeOptions());

bool add_action_bridge_by_name(
    DomainBridge & domain_bridge,
    const std::string& service_name,
    const std::string& service_type,
    size_t from_domain_id,
    size_t to_domain_id,
    const ActionBridgeOptions & options = ActionBridgeOptions());

}
