#include "Address.h"

#include <iomanip>
#include <iostream>

Address::Address(const char* host, uint16_t port) {
  memset(&this->address, 0, sizeof(this->address));
  sockaddr_in*  a4 = (sockaddr_in*)  &this->address;
  sockaddr_in6* a6 = (sockaddr_in6*) &this->address;

  if(inet_pton(AF_INET, host, &a4->sin_addr) == 1) {
    a4->sin_family = AF_INET;
    a4->sin_port   = htons(port);
    this->length   = sizeof(sockaddr_in);
  }
  else if(inet_pton(AF_INET6, host, &a6->sin6_addr) == 1) {
    a6->sin6_family = AF_INET6;
    a6->sin6_port   = htons(port);
    this->length    = sizeof(sockaddr_in6);
  }
  else {
    std::string message = "Could not interpret hostname:\n";
    throw std::runtime_error(message + host);
  }

  // std::cout << "[" << std::string((char*) &a4->sin_addr,   4) << "]\n";
  // std::cout << "[" << std::string((char*) &a6->sin6_addr, 16) << "]\n";
  // std::cout << std::endl;

  std::cout << *this << std::endl;
}

Address Address::IPv4(uint16_t port) {
  Address result;
  sockaddr_in* a4 = (sockaddr_in*) &result.address;

  a4->sin_family      = AF_INET;
  a4->sin_addr.s_addr = INADDR_ANY,
  a4->sin_port        = htons(port);

  result.length = sizeof(sockaddr_in);
  return result;
}

uint16_t Address::port() const {
  if(address.ss_family == AF_INET) {
    sockaddr_in* a4 = (sockaddr_in*) &address;
    return ntohs(a4->sin_port);
  }
  else if(address.ss_family == AF_INET6) {
    sockaddr_in6* a6 = (sockaddr_in6*) &address;
    return ntohs(a6->sin6_port);
  }
  else {
    std::string message = "Unknown address family:\n";
    throw std::runtime_error(message + std::to_string(address.ss_family));
  }
}

std::ostream& operator << (std::ostream& stream, const Address& address) {
  char buffer[INET6_ADDRSTRLEN] = "[unknown]";
  if(address.address.ss_family == AF_INET) {
    sockaddr_in* a4 = (sockaddr_in*) &address.address;
    inet_ntop(AF_INET, &a4->sin_addr, buffer, INET6_ADDRSTRLEN);
  }
  else if(address.address.ss_family == AF_INET6) {
    sockaddr_in6* a6 = (sockaddr_in6*) &address.address;
    inet_ntop(AF_INET6, &a6->sin6_addr, buffer, INET6_ADDRSTRLEN);
  }

  return stream << buffer << ':' << address.port();
}

std::ostream& operator << (std::ostream& stream, const IP& ip) {
  char buffer[INET6_ADDRSTRLEN] = "[unknown]";
  if(ip.v6()) {
    inet_ntop(AF_INET6, ip.bytes, buffer, INET6_ADDRSTRLEN);
  }
  else {
    inet_ntop(AF_INET, ip.bytes + 12, buffer, INET6_ADDRSTRLEN);
  }

  return stream << buffer;
}

std::ostream& operator << (std::ostream& stream, const MAC& mac) {
  stream << std::setfill('0') << std::setw(2) << std::hex << (mac.bytes[0] & 0xff);
  for(int i = 1; i < 6; ++i) {
    stream << ':' << std::setw(2) << (mac.bytes[i] & 0xff);
  }

  return stream << std::setfill(' ') << std::dec;
}

// Address Address::IPv4(const char* host, uint16_t port) {

// }

// Address Address::IPv6(uint16_t port) {

// }

// Address Address::IPv6(const char* host, uint16_t port) {

// }

ssize_t bind(int sock, const Address& address) {
  return bind(sock, (const struct sockaddr*) &address.address, address.length);
}

ssize_t recvfrom(int sock, void* buffer, size_t buflen, Address& address, int flags) {
  address.length = sizeof(address.address);
  return recvfrom(sock, buffer, buflen, flags, (struct sockaddr*) &address.address, &address.length);
}

ssize_t sendto(int sock, void* buffer, size_t buflen, const Address& address, int flags) {
  // std::cout << "Sending " << buflen << " bytes to " << address << '\n';
  return sendto(sock, buffer, buflen, flags, (const struct sockaddr*) &address.address, address.length);
}
