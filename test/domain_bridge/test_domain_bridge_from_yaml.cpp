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

#include <gtest/gtest.h>

#include <filesystem>

#include <algorithm>
#include <string>
#include <vector>

#include "domain_bridge/domain_bridge.hpp"
#include "domain_bridge/domain_bridge_from_yaml.hpp"
#include "domain_bridge/topic_bridge.hpp"

class TestDomainBridgeFromYaml : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestCase()
  {
    rclcpp::shutdown();
  }

  void SetUp()
  {
    test_yaml_dir_ = std::filesystem::current_path() / std::filesystem::path{"config"};
  }

  std::filesystem::path test_yaml_dir_;
};

TEST_F(TestDomainBridgeFromYaml, empty)
{
  // Empty YAML file
  const std::string yaml_path = (test_yaml_dir_ / std::filesystem::path{"empty.yaml"}).string();
  auto bridge = domain_bridge::domain_bridge_from_yaml(yaml_path);
  EXPECT_EQ(bridge->get_bridged_topics().size(), 0u);
}

TEST_F(TestDomainBridgeFromYaml, name)
{
  const std::string yaml_path = (test_yaml_dir_ / std::filesystem::path{"name.yaml"}).string();
  auto bridge = domain_bridge::domain_bridge_from_yaml(yaml_path);
  EXPECT_EQ(bridge->get_bridged_topics().size(), 0u);
  EXPECT_EQ(bridge->get_domain_bridge_options().name(), "domain_bridge_test_name");
}

TEST_F(TestDomainBridgeFromYaml, topics)
{
  std::vector<domain_bridge::TopicBridge> expected = {
    {"baz", "test_msgs/msg/Empty", 10, 7},
    {"/foo/bar", "test_msgs/msg/BasicTypes", 6, 10},
    {"foo", "test_msgs/msg/Strings", 3, 4}
  };
  std::sort(expected.begin(), expected.end());
  const std::string yaml_path = (test_yaml_dir_ / std::filesystem::path{"topics.yaml"}).string();
  auto bridge = domain_bridge::domain_bridge_from_yaml(yaml_path);
  std::vector<domain_bridge::TopicBridge> bridged_topics = bridge->get_bridged_topics();
  ASSERT_EQ(bridged_topics.size(), expected.size());
  for (std::size_t i = 0u; i < bridged_topics.size(); ++i) {
    EXPECT_EQ(bridged_topics[i].topic_name, expected[i].topic_name);
    EXPECT_EQ(bridged_topics[i].type_name, expected[i].type_name);
    EXPECT_EQ(bridged_topics[i].from_domain_id, expected[i].from_domain_id);
    EXPECT_EQ(bridged_topics[i].to_domain_id, expected[i].to_domain_id);
  }
}

TEST_F(TestDomainBridgeFromYaml, invalid)
{
  // Non-existent file
  {
    const std::string yaml_path =
      (test_yaml_dir_ / std::filesystem::path{"not_a_file.yaml"}).string();
    EXPECT_THROW(
      domain_bridge::domain_bridge_from_yaml(yaml_path), domain_bridge::YamlParsingError);
  }
  // Missing 'type' key
  {
    const std::string yaml_path =
      (test_yaml_dir_ / std::filesystem::path{"missing_type.yaml"}).string();
    EXPECT_THROW(
      domain_bridge::domain_bridge_from_yaml(yaml_path), domain_bridge::YamlParsingError);
  }
  // Missing 'from_domain' key
  {
    const std::string yaml_path =
      (test_yaml_dir_ / std::filesystem::path{"missing_from_domain.yaml"}).string();
    EXPECT_THROW(
      domain_bridge::domain_bridge_from_yaml(yaml_path), domain_bridge::YamlParsingError);
  }
  // Missing 'to_domain' key
  {
    const std::string yaml_path =
      (test_yaml_dir_ / std::filesystem::path{"missing_to_domain.yaml"}).string();
    EXPECT_THROW(
      domain_bridge::domain_bridge_from_yaml(yaml_path), domain_bridge::YamlParsingError);
  }
  // 'topics' has wrong type
  {
    const std::string yaml_path =
      (test_yaml_dir_ / std::filesystem::path{"topics_as_list.yaml"}).string();
    EXPECT_THROW(
      domain_bridge::domain_bridge_from_yaml(yaml_path), domain_bridge::YamlParsingError);
  }
}
