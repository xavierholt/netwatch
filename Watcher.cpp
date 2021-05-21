#include "Watcher.h"

#include "Headers.h"

#include <iostream> // TEMP!

#include <string>

// #include <arpa/inet.h>
#include <pcap.h>

Info::Info(const EthernetHeader* eth, const IPv4Header* ip4) {
  memcpy(src.mac.bytes, eth->src, 6);
  memcpy(dst.mac.bytes, eth->dst, 6);

  // std::cout << ip4->src << " -> " << ip4->dst << '\n';

  src.ip = &ip4->src;
  dst.ip = &ip4->dst;
}

bool Info::operator < (const Info& other) const {
  return memcmp(this, &other, sizeof(Info)) < 0;
}

Watcher::Watcher(const char* interface, const DNSProxy* dns): mPCAPErrors(0) {
  mPCAPErrors = new char[PCAP_ERRBUF_SIZE];
  mDNSProxy   = dns;

  uint32_t addr;
  uint32_t mask;
  if(pcap_lookupnet(interface, &addr, &mask, mPCAPErrors) != 0) {
    std::string header = "Failed to load interface information:\n";
    throw std::runtime_error(header + mPCAPErrors);
  }

  mPCAPErrors[0] = '\0';
  mPCAP = pcap_create(interface, mPCAPErrors);
  if(mPCAP == NULL || mPCAPErrors[0] != '\0') {
    std::string header = "Failed to open a PCAP session:\n";
    throw std::runtime_error(header + mPCAPErrors);
  }

  if(pcap_set_timeout(mPCAP, 1000) != 0) {
    std::string header = "Failed to set PCAP timeout:\n";
    throw std::runtime_error(header + pcap_geterr(mPCAP));
  }

  if(pcap_set_immediate_mode(mPCAP, 0) != 0) {
    std::string header = "Failed to set PCAP to buffered mode:\n";
    throw std::runtime_error(header + pcap_geterr(mPCAP));
  }

  if(pcap_set_promisc(mPCAP, 1) != 0) {
    std::string header = "Failed to set PCAP to promiscuous mode:\n";
    throw std::runtime_error(header + pcap_geterr(mPCAP));
  }

  if(pcap_set_snaplen(mPCAP, 54) != 0) {
    std::string header = "Failed to set PCAP snapshot length:\n";
    throw std::runtime_error(header + pcap_geterr(mPCAP));
  }

  if(pcap_activate(mPCAP) != 0) {
    std::string header = "Failed to activate PCAP:\n";
    throw std::runtime_error(header + pcap_geterr(mPCAP));
  }
}

Watcher::~Watcher() {
  pcap_close(mPCAP);
  delete [] mPCAPErrors;
}

void Watcher::callback(unsigned char* user, const pcap_pkthdr* header, const unsigned char* data) {
  if(header->caplen < 14) {
    // Not enough data to process.
    return;
  }

  // std::cout << '.';

  Watcher* watcher = (Watcher*) user;
  EthernetHeader* eth = (EthernetHeader*) data;

  // Beware!  Big endian!
  if(eth->type == 0x0008) {
    // std::cout << '4';
    watcher->handle_ip4(header, data);
  }
  // else if(eth->type == 0xdd86) {
  //   watcher->handle_ip6(header, data);
  // }
  else {
    // std::cout << '(' << std::hex << eth->type << ')';
  }
}

// void Watcher::handle_dns() {

// }

void Watcher::handle_ip4(const pcap_pkthdr* header, const unsigned char* data) {
  if(header->caplen < 34) {
    // Not enough data to process.
    return;
  }

  const EthernetHeader* eth = (EthernetHeader*) data;
  const IPv4Header* ip4     = (IPv4Header*)    (data + 14);

  Info info(eth, ip4);
  Data& stats = mCountCache[info];

  stats.timestamp = header->ts.tv_sec * 1000 + header->ts.tv_usec / 1000;
  stats.bytes    += header->len - 14;
  stats.packets  += 1;
}

// void Watcher::handle_ip6(const struct pcap_pkthdr* header, const unsigned char* data) {
//   if(header->caplen < 54) {
//     // Not enough data to process.
//     return;
//   }

//   const EthernetHeader* eth = (EthernetHeader*) data;
//   const IPv6Header* ip6     = (IPv6Header*)    (data + 14);

//   Info info(eth, ip6);
//   mCountCache[info] += header->len - 14;
// }

void Watcher::render(std::ostream& stream) const {
  // uint64_t now = time(0) * 1000;

  stream << "HTTP/1.1 200 OK\r\n\r\n";

  stream << "# HELP traffic_bytes The total number of bytes sent across the network.\n";
  stream << "# TYPE traffic_bytes counter\n";
  for(const auto& itr: mCountCache) {
    stream << "traffic_bytes{"
      "src_mac=\"" << itr.first.src.mac << "\","
      "dst_mac=\"" << itr.first.dst.mac << "\","
      "src_ip=\""  << itr.first.src.ip  << "\","
      "dst_ip=\""  << itr.first.dst.ip  << "\"";

    const std::string*  host = mDNSProxy->get(itr.first.src.ip);
    if(host == nullptr) host = mDNSProxy->get(itr.first.dst.ip);
    if(host != nullptr) stream << ",host=\"" << *host << '\"';

    stream << "} " << itr.second.bytes << ' ' << itr.second.timestamp << '\n';
  }

  stream << "# HELP traffic_packets The total number of packets sent across the network.\n";
  stream << "# TYPE traffic_packets counter\n";
  for(const auto& itr: mCountCache) {
    stream
      << "traffic_packets{"
         "src_mac=\"" << itr.first.src.mac << "\","
         "dst_mac=\"" << itr.first.dst.mac << "\","
         "src_ip=\""  << itr.first.src.ip  << "\","
         "dst_ip=\""  << itr.first.dst.ip  << "\"} "
      << itr.second.packets << ' ' << itr.second.timestamp << '\n';
  }
}

void Watcher::start() {
  pcap_loop(mPCAP, -1, &Watcher::callback, (unsigned char*) this);
}

void Watcher::stop() {
  pcap_breakloop(mPCAP);
}
