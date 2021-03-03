find_package(ament_cmake_gmock REQUIRED)
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
