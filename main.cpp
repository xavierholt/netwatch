#include <iostream>
#include <signal.h>
#include <thread>

#include "Server.h"
#include "Watcher.h"

DNSProxy* dnsproxy;
Watcher*  watcher;
Server*   server;

void sigint(int) {
  watcher->stop();
  server->stop();
}

int main() {
  signal(SIGINT, sigint);

  Address upstream("192.168.0.1", 53);
  Address binding = Address::IPv4(5300);

  dnsproxy = new DNSProxy(upstream, binding);
  watcher  = new Watcher("en0", dnsproxy);
  server   = new Server(watcher);

  std::thread proxy([](){
    try {
      dnsproxy->start();
    }
    catch(const std::runtime_error& e) {
      std::cerr << "DNS proxy crashed: " << e.what() << std::endl;
    }
  });

  std::thread servy([](){
    try {
      server->start();
    }
    catch(const std::runtime_error& e) {
      std::cerr << "Metric server crashed: " << e.what() << std::endl;
    }
  });

  watcher->start();

  // std::cout << '\n';
  // watcher->render(std::cout);
  return 0;
}
