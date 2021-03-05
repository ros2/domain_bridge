# domain_bridge

A ROS 2 domain bridge.
Bridges ROS communication between different ROS domain IDs.

See the [design document](docs/design.md) for more details about how the bridge works.

**NOTE: the following sections are aspirational and are a work-in-progress.**

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

There is a standalone executable that can be configured via a YAML file:

```sh
ros2 run domain_bridge domain_bridge path/to/config.yaml
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
