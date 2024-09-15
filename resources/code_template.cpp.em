#include <string>
#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/generated_code_interface.hpp"

@[for srv_type in service_types]@
@(type_name_to_include(srv_type))
@[end for]@

@[for action_type in action_types]@
@(type_name_to_include(action_type))
@[end for]@

bool domain_bridge::add_service_bridge_by_name(
    domain_bridge::DomainBridge & domain_bridge,
    const std::string& service_name,
    const std::string& service_type,
    size_t from_domain_id,
    size_t to_domain_id,
    const domain_bridge::ServiceBridgeOptions & options) {

    if (false) {
    }
@[for srv_type in service_types]@
    else if (service_type == "@(srv_type)") {
        // Handle @(srv_type)
      std::cout << "Adding service bridge for @(srv_type) " << service_name << " from domain " << from_domain_id << " to domain " << to_domain_id << std::endl;

        domain_bridge.bridge_service<@(type_name_to_cpp_type(srv_type))>(service_name, from_domain_id, to_domain_id, options);
        return true;
    }
@[end for]@
    return false;
}

bool domain_bridge::add_action_bridge_by_name(
    domain_bridge::DomainBridge & domain_bridge,
    const std::string& action_name,
    const std::string& action_type,
    size_t from_domain_id,
    size_t to_domain_id,
    const domain_bridge::ActionBridgeOptions & options) {

    if (false) {
    }
@[for action_type in action_types]@
    else if (action_type == "@(action_type)") {
        // Handle @(action_type)
      std::cout << "Adding action bridge for @(action_type) " << action_name << " from domain " << from_domain_id << " to domain " << to_domain_id << std::endl;

        domain_bridge.bridge_action<@(type_name_to_cpp_type(action_type))>(action_name, from_domain_id, to_domain_id, options);
        return true;
    }
@[end for]@
    return false;
}
