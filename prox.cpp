#include "Address.h"
#include "DNSProxy.h"

#include <iostream>

int main() {
  Address upstream("192.168.0.1", 53);
  Address binding = Address::IPv4(5300);

  std::cout << "Upstream: " << upstream << '\n';
  std::cout << "Binding:  " << binding  << '\n';

  DNSProxy proxy(upstream, binding);
  proxy.start();

  return 0;
}
