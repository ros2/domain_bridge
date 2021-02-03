#include "domain_bridge/domain_bridge.hpp"

int main(int argc, char ** argv) {
  (void)argc;
  (void)argv;

  domain_bridge::DomainBridge domain_bridge(1, 2);
  domain_bridge.spin();
}
