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

#include <sstream>
#include <string>

#include "rclcpp/executors/single_threaded_executor.hpp"
#include "rcpputils/scope_exit.hpp"
#include "rcutils/cmdline_parser.h"
#include "rcutils/env.h"

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/parse_domain_bridge_yaml_config.hpp"

#include "ndds/ndds_c.h"

void help()
{
  std::cerr << "Usage: domain_bridge "
    "[--from FROM_DOMAIN_ID] [--to TO_DOMAIN_ID] [-h] YAML_CONFIG" << std::endl <<
    std::endl <<
    "Arguments:" << std::endl <<
    "    YAML_CONFIG    path to a YAML configuration file." << std::endl <<
    std::endl <<
    "Options:" << std::endl <<
    "    --from FROM_DOMAIN_ID    All data will be bridged from this domain ID.  " << std::endl <<
    "                             This overrides any domain IDs set in the YAML file." <<
    std::endl <<
    "    --to TO_DOMAIN_ID        All data will be bridged to this domain ID.  " << std::endl <<
    "                             This overrides any domain IDs set in the YAML file." <<
    std::endl <<
    "    --help, -h               Print this help message." << std::endl;
}

bool
set_default_qos_library(DDS_DomainParticipantFactory * dpf);

const char *
get_domain_profile_prefix();

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  if (argc < 2) {
    std::cerr << "error: Expected YAML config file" << std::endl;
    help();
    return 1;
  }

  if (rcutils_cli_option_exist(argv, argv + argc, "-h") ||
    rcutils_cli_option_exist(argv, argv + argc, "--help"))
  {
    help();
    return 0;
  }

  // Get options
  const char * from_domain_opt = rcutils_cli_get_option(argv, argv + argc, "--from");
  const char * to_domain_opt = rcutils_cli_get_option(argv, argv + argc, "--to");
  std::size_t from_domain = 0u;
  if (from_domain_opt) {
    std::istringstream iss(from_domain_opt);
    iss >> from_domain;
    if (iss.fail() || !iss.eof()) {
      std::cerr << "error: Failed to parse FROM_DOMAIN_ID '" <<
        from_domain_opt << "'" << std::endl;
      return 1;
    }
  }
  std::size_t to_domain = 0u;
  if (to_domain_opt) {
    std::istringstream iss(to_domain_opt);
    iss >> to_domain;
    if (iss.fail() || !iss.eof()) {
      std::cerr << "error: Failed to parse TO_DOMAIN_ID '" <<
        to_domain_opt << "'" << std::endl;
      return 1;
    }
  }

  std::string yaml_config = argv[argc - 1];
  domain_bridge::DomainBridgeConfig domain_bridge_config =
    domain_bridge::parse_domain_bridge_yaml_config(yaml_config);

  // Override 'from_domain' and 'to_domain' in config
  for (auto & topic_option_pair : domain_bridge_config.topics) {
    if (from_domain_opt) {
      topic_option_pair.first.from_domain_id = from_domain;
    }
    if (to_domain_opt) {
      topic_option_pair.first.to_domain_id = to_domain;
    }
  }
  DDS_DomainParticipantFactory * dpf = DDS_DomainParticipantFactory_get_instance();
  const char * prefix = get_domain_profile_prefix();
  static DDS_DomainParticipantQos default_dpqos = DDS_DomainParticipantQos_INITIALIZER;
  bool reset_participant_profile = false;
  if (set_default_qos_library(dpf) && prefix) {
    if (
      DDS_DomainParticipantFactory_get_default_participant_qos(
        dpf, &default_dpqos) != DDS_RETCODE_OK)
    {
      throw std::runtime_error("failed to get default participang qos");
    }
    reset_participant_profile = true;
    domain_bridge_config.options.on_new_domain_callback(
      [dpf, prefix](size_t domain_id)
      {
        auto profile_name = std::string(prefix) + "_" + std::to_string(domain_id);
        if (
          DDS_RETCODE_OK == DDS_DomainParticipantFactory_set_default_participant_qos_with_profile(
            dpf, nullptr, profile_name.c_str()))
        {
          return;
        }
        RCLCPP_INFO(
          rclcpp::get_logger("domain_bridge"),
          "failed to set rti connext profile '%s' for domain '%zu' "
          ", the default profile will be used",
          profile_name.c_str(), domain_id);
        if (
          DDS_RETCODE_OK != DDS_DomainParticipantFactory_set_default_participant_qos(
            dpf, &default_dpqos))
        {
          RCLCPP_ERROR(
            rclcpp::get_logger("domain_bridge"),
            "failed to set reset the rti connext profile participant profile to the default");
        }
      });
  }
  domain_bridge::DomainBridge domain_bridge(domain_bridge_config);
  if (
    reset_participant_profile &&
    DDS_RETCODE_OK != DDS_DomainParticipantFactory_set_default_participant_qos(
      dpf, &default_dpqos))
  {
    RCLCPP_ERROR(
      rclcpp::get_logger("domain_bridge"),
      "failed to set reset the rti connext profile participant profile to the default");
  }

  rclcpp::executors::SingleThreadedExecutor executor;
  domain_bridge.add_to_executor(executor);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}

const char *
get_domain_profile_prefix()
{
  const char * qos_profile_library_name = nullptr;
  const char * error = rcutils_get_env(
    "DOMAIN_BRIDGE_CONNEXT_DDS_QOS_PROFILE_PREFIX", &qos_profile_library_name);
  if (error) {
    throw std::runtime_error(std::string("rcutils_get_env() failed: ") + error);
  }
  if (qos_profile_library_name && 0 == strcmp("", qos_profile_library_name)) {
    return nullptr;
  }
  return qos_profile_library_name;
}

bool
set_default_qos_library(DDS_DomainParticipantFactory * dpf)
{
  const char * qos_profile_library_name = nullptr;
  const char * error = rcutils_get_env(
    "DOMAIN_BRIDGE_CONNEXT_DDS_QOS_LIBRARY_NAME", &qos_profile_library_name);
  if (error) {
    throw std::runtime_error(std::string("rcutils_get_env() failed: ") + error);
  }
  if (qos_profile_library_name && 0 == strcmp("", qos_profile_library_name)) {
    qos_profile_library_name = nullptr;
  }

  DDS_StringSeq qos_libraries;
  rcpputils::scope_exit(
    [qos_libraries = &qos_libraries]() {
      DDS_StringSeq_finalize(qos_libraries);
    });
  if (!qos_profile_library_name) {
    // environment variable is empty
    if (
      DDS_RETCODE_OK != DDS_DomainParticipantFactory_get_qos_profile_libraries(
        dpf, &qos_libraries))
    {
      throw std::runtime_error("failed to get qos profile libraries");
    }
    // If only one non-builtin qos profile library was loaded, use that one.
    if (DDS_StringSeq_get_length(&qos_libraries) > 3) {
      return false;
    }
    for (int i = 0; i < DDS_StringSeq_get_length(&qos_libraries); i++) {
      const char * item = DDS_StringSeq_get(&qos_libraries, i);
      if (
        strcmp("BuiltinQosLib", item) != 0 &&
        strcmp("BuiltinQosLibExp", item) != 0)
      {
        if (!qos_profile_library_name) {
          qos_profile_library_name = item;
        } else {
          RCLCPP_INFO(
            rclcpp::get_logger("domain_bridge"),
            "found more than one RTI Connext qos library available, specify the desired library "
            "name in the 'DOMAIN_BRIDGE_CONNEXT_DDS_QOS_LIBRARY_NAME' environment variable");
          qos_profile_library_name = nullptr;
          break;
        }
      }
    }
  }
  if (qos_profile_library_name) {
    if (
      DDS_RETCODE_OK != DDS_DomainParticipantFactory_set_default_library(
        dpf, qos_profile_library_name))
    {
      throw std::runtime_error(std::string("failed to set default library: ") + error);
    }
    return true;
  }
  return false;
}
