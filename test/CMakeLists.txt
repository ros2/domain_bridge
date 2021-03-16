find_package(ament_cmake_gmock REQUIRED)
find_package(ament_cmake_gtest REQUIRED)
find_package(launch_testing_ament_cmake REQUIRED)
find_package(test_msgs REQUIRED)

ament_add_gmock(test_domain_bridge
  domain_bridge/test_domain_bridge.cpp
)
if(TARGET test_domain_bridge)
  ament_target_dependencies(test_domain_bridge
    "rclcpp"
  )
  target_link_libraries(test_domain_bridge ${PROJECT_NAME})
endif()

ament_add_gmock(test_parse_domain_bridge_yaml_config
  domain_bridge/test_parse_domain_bridge_yaml_config.cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/domain_bridge"
)
if(TARGET test_parse_domain_bridge_yaml_config)
  target_link_libraries(test_parse_domain_bridge_yaml_config ${PROJECT_NAME})
endif()

ament_add_gmock(test_qos_matching
  domain_bridge/test_qos_matching.cpp
)
if(TARGET test_qos_matching)
  ament_target_dependencies(test_qos_matching
    "rclcpp"
    "test_msgs"
  )
  target_link_libraries(test_qos_matching ${PROJECT_NAME})
endif()

# Test executable
add_launch_test(
  test_domain_bridge.launch.py
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}_exec>"
)

# Install test files to build directory
install(
  DIRECTORY "domain_bridge/config"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)
