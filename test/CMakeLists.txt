find_package(ament_cmake_gmock REQUIRED)
find_package(ament_cmake_gtest REQUIRED)
find_package(launch_testing_ament_cmake REQUIRED)
find_package(rmw_implementation_cmake REQUIRED)
find_package(test_msgs REQUIRED)

ament_add_gmock(test_domain_bridge
  domain_bridge/test_domain_bridge.cpp
)
if(TARGET test_domain_bridge)
  ament_target_dependencies(test_domain_bridge
    "rclcpp"
  )
  target_link_libraries(test_domain_bridge ${PROJECT_NAME}_lib)
endif()

ament_add_gmock(test_parse_domain_bridge_yaml_config
  domain_bridge/test_parse_domain_bridge_yaml_config.cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/domain_bridge"
)
if(TARGET test_parse_domain_bridge_yaml_config)
  target_link_libraries(test_parse_domain_bridge_yaml_config ${PROJECT_NAME}_lib)
endif()

add_library(${PROJECT_NAME}_test_component_lib SHARED
  ${PROJECT_NAME}/test_component.cpp
)
ament_target_dependencies(${PROJECT_NAME}_test_component_lib
  rclcpp
  test_msgs
)
rclcpp_components_register_node(${PROJECT_NAME}_test_component_lib
  PLUGIN "domain_bridge::TestComponent"
  EXECUTABLE test_component)

# The following tests are run for each available RMW implementation
function(add_communication_tests)
  set(rmw_implementation_env RMW_IMPLEMENTATION=${rmw_implementation})
  ament_add_gmock(test_qos_matching${target_suffix}
    domain_bridge/test_qos_matching.cpp
    ENV ${rmw_implementation_env}
  )
  if(TARGET test_qos_matching${target_suffix})
    ament_target_dependencies(test_qos_matching${target_suffix}
      "rclcpp"
      "test_msgs"
    )
    target_link_libraries(test_qos_matching${target_suffix} ${PROJECT_NAME}_lib)
  endif()

  ament_add_gmock(test_domain_bridge_end_to_end${target_suffix}
    domain_bridge/test_domain_bridge_end_to_end.cpp
    ENV ${rmw_implementation_env}
  )
  if(TARGET test_domain_bridge_end_to_end${target_suffix})
    ament_target_dependencies(test_domain_bridge_end_to_end${target_suffix}
      "rclcpp"
      "test_msgs"
    )
    target_link_libraries(test_domain_bridge_end_to_end${target_suffix} ${PROJECT_NAME}_lib)
  endif()

  ament_add_gmock(test_domain_bridge_services${target_suffix}
    domain_bridge/test_domain_bridge_services.cpp
    ENV ${rmw_implementation_env}
  )
  if(TARGET test_domain_bridge_services${target_suffix})
    ament_target_dependencies(test_domain_bridge_services${target_suffix}
      "rclcpp"
      "test_msgs"
    )
    target_link_libraries(test_domain_bridge_services${target_suffix} ${PROJECT_NAME}_lib)
  endif()
endfunction()
call_for_each_rmw_implementation(add_communication_tests)

# Test executable
add_launch_test(
  test_domain_bridge.launch.py
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}_exec>"
)

install(TARGETS ${PROJECT_NAME}_test_component_lib
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)