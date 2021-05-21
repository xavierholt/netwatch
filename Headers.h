#ifndef HEADERS_H
#define HEADERS_H

#include <cstdint>
#include <sys/socket.h>

struct EthernetHeader {
  uint8_t  dst[6];
  uint8_t  src[6];
  uint16_t type;
};

struct IPv4Header {
  uint8_t  head;
  uint8_t  tos;
  uint16_t len;
  uint16_t id;
  uint16_t frag;
  uint8_t  ttl;
  uint8_t  prot;
  uint16_t sum;

  in_addr  src;
  in_addr  dst;
};

struct IPv6Header {
  uint32_t flags;
  uint16_t len;
  uint8_t  next;
  uint8_t  hops;

  in6_addr src;
  in6_addr dst;
};

struct DNSHeader {
  uint16_t id;
  uint16_t flags;
  uint16_t nquestions;
  uint16_t nanswers;
  uint16_t nauthorities;
  uint16_t nextras;
};

struct DNSRecord {
  uint16_t type;
  uint16_t clas;
  uint32_t ttl;
  uint16_t rdlength;

  uint8_t  rdata[];
};

#endif
