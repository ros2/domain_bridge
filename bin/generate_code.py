#!/usr/bin/env python3

import argparse
import os
from ament_index_python.packages import get_packages_with_prefixes
from rosidl_cmake import expand_template
import re


def camel_case_to_lower_case_underscore(value):
    # insert an underscore before any upper case letter
    # which is not followed by another upper case letter
    value = re.sub('(.)([A-Z][a-z]+)', '\\1_\\2', value)
    # insert an underscore before any upper case letter
    # which is preseded by a lower case letter or number
    value = re.sub('([a-z0-9])([A-Z])', '\\1_\\2', value)
    return value.lower()

def type_name_to_include(type_name):
    package, interface_type, type = type_name.split("/")
    type = camel_case_to_lower_case_underscore(type)
    return "#include <" + package + "/" + interface_type + "/" + type + ".hpp>"

def type_name_to_cpp_type(type_name):
    package, interface_type, type = type_name.split("/")
    return package + "::" + interface_type + "::" + type

def get_types():
    ros2_packages = get_packages_with_prefixes()

    message_types = []
    service_types = []
    action_types = []

    for package_name, prefix_path in ros2_packages.items():
        share_path = os.path.join(prefix_path, 'share', package_name)
        # Collect message types
        msg_path = os.path.join(share_path, 'msg')
        if os.path.exists(msg_path):
            for msg_file in os.listdir(msg_path):
                if msg_file.endswith('.msg'):
                    msg_type = f"{package_name}/msg/{msg_file[:-4]}"
                    message_types.append(msg_type)
        # Collect service types
        srv_path = os.path.join(share_path, 'srv')
        if os.path.exists(srv_path):
            for srv_file in os.listdir(srv_path):
                if srv_file.endswith('.srv'):
                    srv_type = f"{package_name}/srv/{srv_file[:-4]}"
                    service_types.append(srv_type)
        # Collect action types
        action_path = os.path.join(share_path, 'action')
        if os.path.exists(action_path):
            for action_file in os.listdir(action_path):
                if action_file.endswith('.action'):
                    action_type = f"{package_name}/action/{action_file[:-7]}"
                    action_types.append(action_type)

    return message_types, service_types, action_types

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    args = parser.parse_args()

    message_types, service_types, action_types = get_types()

    template_file = os.path.join(os.path.dirname(__file__), '../resources/code_template.cpp.em')
    expand_template(template_file, {
        'message_types': message_types,
        'service_types': service_types,
        'action_types': action_types,
        'camel_case_to_lower_case_underscore': camel_case_to_lower_case_underscore,
        'type_name_to_include': type_name_to_include,
        'type_name_to_cpp_type': type_name_to_cpp_type
    }, args.output)

if __name__ == '__main__':
    main()
