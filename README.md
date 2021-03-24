# domain_bridge

A ROS 2 domain bridge.
Bridges ROS communication between different ROS domain IDs.

See the [design document](docs/design.md) for more details about how the bridge works.

## Prerequisites

- [ROS 2](https://index.ros.org/doc/ros2/Installation) (Galactic or newer)

## Installation

### Ubuntu 20.04

Replace `$ROS_DISTRO` with the ROS codename (e.g. `galactic`):

```
sudo apt install ros-$ROS_DISTRO-domain-bridge
```

### From source

**Prerequisites:**

- Git
- [colcon](https://colcon.readthedocs.io/) (colcon_cmake)

```
mkdir -p domain_bridge_ws/src
cd domain_bridge_ws
git clone https://github.com/ros2/domain_bridge.git src/
colcon build
```

## Usage

### CLI

There is a standalone executable that can be configured via a YAML file.

You must provide the path to a YAML configuration file as the last argument to the executable.
For example,

```sh
ros2 run domain_bridge domain_bridge examples/example_bridge_config.yaml
```

There are also options to override the domain IDs set for all entities in the YAML config,
for example,

```sh
ros2 run domain_bridge domain_bridge --from 7 --to 8 examples/example_bridge_config.yaml
```

Use the `--help` option for more usage information:

```sh
ros2 run domain_bridge domain_bridge --help
```

### Launch

You can also use the [example launch file](TODO):

```xml
ros2 launch domain_bridge domain_bridge.launch.xml config:=path/to/config.yaml
```

Or, include it in your own launch file:

```xml
<launch>
  <include file="$(find-pkg-share domain_birdge)/launch/domain_bridge.launch.xml">
    <arg name="config" value="path/to/config.yaml" />
  </include>
</launch>
```

### C++ library

There is a C++ API that can be integrated into your own process, you can find the [API docs here](TODO).
