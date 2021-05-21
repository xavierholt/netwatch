#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>
#include <cstring>
#include <ostream>
#include <sys/socket.h>
#include <arpa/inet.h>

struct MAC {
  uint8_t bytes[6];

  friend std::ostream& operator << (std::ostream& stream, const MAC& mac);
};

union IP {
  uint64_t quads[ 2];
  uint32_t words[ 4];
  uint8_t  bytes[16];

  IP() = default;
  IP(const IP& other) = default;

  IP(const in_addr* data) {
    this->quads[0] = 0;
    this->words[2] = htonl(0xff);
    memcpy(this->bytes + 12, data, 4);
  }

  IP(const in6_addr* data) {
    memcpy(this->bytes, data, 16);
  }

  bool v4() const {
    return quads[0] == 0 && words[2] == htonl(0xff);
  }

  bool v6() const {
    return !v4();
  }

  bool operator < (const IP& other) const {
    if(quads[0] != other.quads[0]) {
      return quads[0] < other.quads[0];
    }
    else {
      return quads[1] < other.quads[1];
    }
  }

  friend std::ostream& operator << (std::ostream& stream, const IP& ip);
};

struct Address {
  socklen_t        length;
  sockaddr_storage address;

  static Address IPv4(uint16_t port = 0);
  static Address IPv4(const char* host, uint16_t port = 0);

  static Address IPv6(uint16_t port = 0);
  static Address IPv6(const char* host, uint16_t port = 0);

  Address() = default;
  Address(const Address& other) = default;
  Address(const char* host, uint16_t port);

  uint16_t port() const;

  friend std::ostream& operator << (std::ostream& stream, const Address& address);
};

ssize_t bind(int sock, const Address& address);
ssize_t recvfrom(int sock, void* buffer, size_t buflen, Address& address, int flags = 0);
ssize_t sendto(int sock, void* buffer, size_t buflen, const Address& address, int flags = 0);

#endif
