#ifndef WATCHER_H
#define WATCHER_H

#include "Address.h"
#include "DNSProxy.h"

#include <ctime>
#include <map>
#include <ostream>

typedef struct pcap pcap_t;
struct pcap_pkthdr;

struct EthernetHeader;
struct IPv4Header;
struct IPv6Header;

struct Info {
  struct {
    IP  ip;
    MAC mac;
    // uint32_t ip4;
    // uint8_t  mac[8];
  } src;

  struct {
    IP  ip;
    MAC mac;
    // uint32_t ip4;
    // uint8_t  mac[8];
  } dst;

  Info(const EthernetHeader* eth, const IPv4Header* ip4);

  bool operator < (const Info& other) const;
};

struct Data {
  uint64_t bytes;
  uint64_t packets;
  uint64_t timestamp;

  Data() {
    bytes     = 0;
    packets   = 0;
    timestamp = 0;
  }
};

class Watcher {
  pcap_t* mPCAP;
  char*   mPCAPErrors;

  // Cache<uint32_t, std::string> mDNSCache;
  std::map<Info, Data> mCountCache;
  const DNSProxy*      mDNSProxy;

private:
  static void callback(unsigned char* user, const pcap_pkthdr* header, const unsigned char* data);

public:
  Watcher(const char* interface, const DNSProxy* dns);
  ~Watcher();

  void render(std::ostream& stream) const;

  // void handle_dns();
  void handle_ip4(const struct pcap_pkthdr* header, const unsigned char* data);
  void handle_ip6(const struct pcap_pkthdr* header, const unsigned char* data);

  void start();
  void stop();
};

#endif
