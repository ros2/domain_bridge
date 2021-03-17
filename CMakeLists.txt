cmake_minimum_required(VERSION 3.5)
project(domain_bridge)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcutils REQUIRED)
# Leverage rosbag2's generic type support utilities
find_package(rosbag2_cpp REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/${PROJECT_NAME}/domain_bridge.cpp
  src/${PROJECT_NAME}/domain_bridge_options.cpp
  src/${PROJECT_NAME}/generic_publisher.cpp
  src/${PROJECT_NAME}/generic_subscription.cpp
  src/${PROJECT_NAME}/parse_domain_bridge_yaml_config.cpp
  src/${PROJECT_NAME}/qos_options.cpp
  src/${PROJECT_NAME}/topic_bridge_options.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  rcutils
  rosbag2_cpp
  yaml_cpp_vendor
)

add_executable(${PROJECT_NAME}_exec
  src/domain_bridge.cpp
)

set_target_properties(${PROJECT_NAME}_exec PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PREFIX "")

target_link_libraries(${PROJECT_NAME}_exec
  ${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_exec
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY examples launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(
  rclcpp
  rosbag2_cpp
  rcutils
  yaml_cpp_vendor
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  add_subdirectory(test)
endif()

ament_package()
