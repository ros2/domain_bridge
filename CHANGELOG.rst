^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package domain_bridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.4.0 (2021-10-22)
------------------
* Make some end to end tests more reliable (`#51 <https://github.com/ros2/domain_bridge/issues/51>`_)
* Install CLI parsing header (`#45 <https://github.com/ros2/domain_bridge/issues/45>`_)
* Run communication-related tests with all available RMWs (`#43 <https://github.com/ros2/domain_bridge/issues/43>`_)
* Fix bug when waiting for service to be available (`#42 <https://github.com/ros2/domain_bridge/issues/42>`_) (`#55 <https://github.com/ros2/domain_bridge/issues/55>`_)
* Update CI to target Galactic (`#37 <https://github.com/ros2/domain_bridge/issues/37>`_)
* Add `--mode` argument to `domain_bridge` (`#35 <https://github.com/ros2/domain_bridge/issues/35>`_)
* Add template method to help bridging services (`#26 <https://github.com/ros2/domain_bridge/issues/26>`_)
* Fix library target install (`#36 <https://github.com/ros2/domain_bridge/issues/36>`_)
* Update CI workflow (`#34 <https://github.com/ros2/domain_bridge/issues/34>`_)
* Prevent bridging from/to same domain ID (`#33 <https://github.com/ros2/domain_bridge/issues/33>`_)
* RTI QoS profiles patch (`#25 <https://github.com/ros2/domain_bridge/issues/25>`_)
  * Add option to run callback before creating new Domain
  * Add a domain_bridge executable that allows to load an RTI qos profile before creating a new domain  participant
* Add compressing and decompressing modes (`#24 <https://github.com/ros2/domain_bridge/issues/24>`_)
* Refactor to use generic pub/sub from rclcpp (`#30 <https://github.com/ros2/domain_bridge/issues/30>`_)
* Fix bug in generic subscription (`#27 <https://github.com/ros2/domain_bridge/issues/27>`_)
* Contributors: Ivan Santiago Paunovic, Jacob Perron, Rebecca Butler

0.3.0 (2021-05-20)
------------------
* Override handle_serialized_message (`#21 <https://github.com/ros2/domain_bridge/issues/21>`_)
* Do not crash if there's an error querying endpoint info (`#20 <https://github.com/ros2/domain_bridge/issues/20>`_)
* Add topic remapping (`#19 <https://github.com/ros2/domain_bridge/issues/19>`_)
* Fix doc link in readme (`#18 <https://github.com/ros2/domain_bridge/issues/18>`_)
* Contributors: Chris Lalancette, Jacob Perron, Tully Foote

0.2.0 (2021-04-08)
------------------
* Stop installing test resources (`#17 <https://github.com/ros2/domain_bridge/issues/17>`_)
* Add explicit link against stdc++fs (`#16 <https://github.com/ros2/domain_bridge/issues/16>`_)
* Contributors: Scott K Logan

0.1.0 (2021-04-05)
------------------
* Change default value of deadline and lifespan (`#15 <https://github.com/ros2/domain_bridge/issues/15>`_)
* Include rclcpp from-source in CI
* Add missing test dependency
* Add QoS overriding
* Add launch file (`#9 <https://github.com/ros2/domain_bridge/issues/9>`_)
* Ignore generated Python files (`#12 <https://github.com/ros2/domain_bridge/issues/12>`_)
* Add '--from' and '--to' options to executable + add tests (`#7 <https://github.com/ros2/domain_bridge/issues/7>`_)
* Automatically match QoS settings across the bridge (`#5 <https://github.com/ros2/domain_bridge/issues/5>`_)
* Refactor YAML parsing and allow default domain IDs (`#6 <https://github.com/ros2/domain_bridge/issues/6>`_)
* Support for configuring domain bridge with YAML (`#4 <https://github.com/ros2/domain_bridge/issues/4>`_)
* Fix topic bridge less operator (`#3 <https://github.com/ros2/domain_bridge/issues/3>`_)
* Add GitHub workflow for CI
* Add unit tests
* Add domain bridge library (`#1 <https://github.com/ros2/domain_bridge/issues/1>`_)
* Contributors: Jacob Perron
