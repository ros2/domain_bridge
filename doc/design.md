# domain_bridge Design

This article documents the design of the domain_bridge.

## Background

A ROS 2 network can be partitioned by assigning *nodes* (more specifically, *contexts* associated with nodes) different *domain IDs*.
A domain ID is simply an integer.
Nodes with different domain IDs cannot directly communicate with each other.
This is useful for avoiding interference and reducing network traffic, for example.
For more information, check out [this article about DDS domains from RTI](https://community.rti.com/static/documentation/connext-dds/5.2.3/doc/manuals/connext_dds/html_files/RTI_ConnextDDS_CoreLibraries_UsersManual/Content/UsersManual/Fundamentals_of_DDS_Domains_and_DomainPa.htm).

However, there are applications that would like to be able to share *some* data between different ROS 2 domains.
Therefore, it would be nice to have the ability to "bridge" a smaller set of ROS network traffic across domains, hence the *domain_bridge*.

## Requirements

The following are a set of requirements we would like our domain bridge to meet:

1. Bridge ROS topics from one domain to another.

There should be a way for nodes in different domains to communicate with each other using ROS topics.
This should be configurable on a per-topic basis.
I.e. if there is a topic being published on domain *A*, there should be a mechanism to allow for subscriptions to the topic in another domain *B*, where *A* != *B*.

2. Bridge ROS services from one domain to another.

The same as (1), but for ROS services.

3. Bridge ROS actions from one domain to another.

The same as (1), but for ROS actions.

4. No explicit dependencies on ROS interface types

The bridge should work with any supported ROS type (e.g. .msg, .srv, .action, or .idl).
Furthermore, the bridge should not have to declare any explicit dependencies on interfaces types (e.g. in code or package.xml).
Ideally, the bridge can just deal with serialized data, though it may be necessary to dynamically load type support at runtime.

5. Accurately bridge Quality of Service settings

All ROS communication entities (e.g. publishers and service clients) have [Quality of Service (QoS)](http://design.ros2.org/articles/qos.html) settings associated with them.
The bridge should faithfully map the QoS settings of data coming from one domain into another domain.
For example, a publisher with reliability policy set to "best effort" should continue to publish as "best effort" in the other domains when bridged.

If there are multiple publishers on the same topic, but with different QoS settings, the bridge should have two streams of data, each with their own QoS settings.
For example, if there is one publisher using "best effort" and another publisher using "reliable" on the same topic (with the same domain ID) and a bridge is made,
then the bridge should forward data as "best effort" for the first publisher and as "reliable" for the second publisher into the output domain.

Since remotely querying the *history* and *depth* QoS policies is not possible in many implementations (e.g. it is not required by the DDS spec),
the bridge should offer a way to configure these values explicitly.
This is important so that users can configure the bridge to avoid exceeding the history queue depth, resulting in lost messages.

6. Remapping

[Remapping](https://design.ros2.org/articles/static_remapping.html) is a generally useful concept in ROS.
The bridge should support remapping topic, service, and action names when mapping the name from one domain to another.
It is possible to bridge different topic names with the same name from different domains, so we should keep this in mind when it comes to remapping names.

For example, users may want to remap topic `/foo` in domain ID `1` to a different name `/bar` in domain ID `2`.

7. Easily configurable

Since the set of bridged data depends on the application, we need an easy way to configure what data is bridged.
Users should be able to specify the following information at an API level:

- The domain IDs that should be bridged
- The topic names and types that should be bridged (same for services and actions)
- History and depth QoS policies for each topic bridged
- Remappings of topic, service, and action names
- Optionally, it could be useful to override QoS settings for each entity being bridged.

Not only should the bridge be configurable at an API level, but there should also be a way to configure it without having to change code (e.g. a passed in YAML or XML file).

8. Security

The bridge should not subvert [Secure ROS 2](https://github.com/ros2/sros2) (SROS 2) mechanisms.
Similarly, SROS 2 should not prevent the domain bridge from working correclty (i.e. the bridge should not break with security enabled).

Consider that it is possible for a user to only secure half of a domain bridge (i.e. only one domain has security enabled).
This means that a bridged topic, though secure in one domain, is left completely open in another domain.
Presuming that there is no straight-forward solution to enforcing that both sides of a bridge are secured, the least we can do is make sure users are well-informed of this potential security vulnerability.
Additionally, tools may be provided to help streamline securing a ROS system with a domain bridge.

## Approach

The proposed solution involves several nodes in a single bridge process.
For each unique domain ID involved in the bridge process, a ROS context is created for that domain ID and a node is associated with the context.
Therefore, we end up with one node per context per domain ID, all inside a single process.
Bridged data will be shared between the nodes in the process so that it can be rebroadcast to a different domain.
Nodes will be created as needed, based on the configuration of the bridge.

For example, the following diagram illustrates a configuration where the "/chatter" topic from domain IDs `1` and `3` are both being bridged to domain ID `2`:

![](topic_example.png)

### API

A C++ library with a public API is provided for users to call in their own process and extend as they like.
The public API is expected to evolve over time, but at the very least users should be able to bridge ROS networks primitives.


For convenience, a standalone binary executable is also provided for easy integration into ROS systems.

### Supporting generic types

When bridging data, serialized data will be read from one domain and forwarded to another domain.
As long as the bridge does not need access to the typed data, there is no need for deserialization and, therefore, C++ type support.
In this way, we can handle arbitrary data coming from the middleware (as long as we load type support for the middleware).
This is exactly how [rosbag2](https://github.com/ros2/rosbag2/tree/e4ce24cdfa7e24c6d2c025ecc38ab1157a0eecc8/rosbag2_transport) works; defining generic publishers and subscriptions.
In fact, the generic publisher and subscription implementation is being moved to a common location that the domain bridge can leverage once available (see https://github.com/ros2/rclcpp/pull/1452).

### QoS mapping

TODO

### Remapping

TODO

### Configuration

The [YAML](https://yaml.org/) language was chosen as an easy way to externally configure the domain bridge.
YAML is relatively simple to read for humans and computers alike and is commonly used in other places of the ROS ecosystem (e.g. ROS parameters).

The bridge will look for three keys, `topics`, `services`, and `actions`, each of which contains a list of objects.
The key for each object is the input name of the ROS entity (topic, service, or action name).
For each ROS entity, you must specifiy the `type` of the interface, the domain ID where input data will come from (`from_domain`), and the domain ID for where to bridge (`to_domain`).

Here is an example of a configuration file for bridging multiple topics, a service, and an action:

```yaml
topics:
  # Bridge "/foo/chatter" topic from doman ID 2 to domain ID 3
  - foo/chatter:
    type: example_interfaces/msg/String
    from_domain: 2
    to_domain: 3
  # Bridge "/clock" topic from doman ID 2 to domain ID 3, with depth 1
  - clock:
    type: rosgraph_msgs/msg/Clock
    from_domain: 2
    to_domain: 3
    depth: 1
  # Bridge "/clock" topic from doman ID 2 to domain ID 6, with "keep all" history policy
  - clock:
    type: rosgraph_msgs/msg/Clock
    from_domain: 2
    to_domain: 6
    history: keep_all

services:
  # Bridge "add_two_ints" service from domain ID 4 to domain ID 6
  - add_two_ints:
    type: example_interfaces/srv/AddTwoInts
    from_domain: 4
    to_domain: 6

actions:
  # Bridge "fibonacci" action from domain ID 2 to domain ID 3
  - fibonacci:
    type: example_interfaces/action/Fibonacci
    from_domain: 2
    to_domain: 3
```

### Security

TODO

## Alternatives

### SOSS

[System Of Systems Synthesizer](https://soss.docs.eprosima.com/en/latest/index.html) allows communication among an arbitrary number of protocols that speak different languages.
It can be [used as a ROS 2 domain bridge](https://github.com/osrf/soss/blob/f5471497751d5f8e8eb63d728614074e295b2666/examples/sample_ros2_domain_change.yaml).

Pros:
- A good choice if you also want to bridge different middlewares (not just ROS 2).

Cons:
- Requires generating/building SOSS type support for every interface being bridged.
- Slightly less performant due to extra data marshaling.
