#ifndef DNSPROXY_H
#define DNSPROXY_H

#include "Address.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

class DNSProxy {
  struct Query {
    Address  address;
    int      socket;
    uint16_t id;
    uint64_t time;
  };

  Address mUpstream;
  Address mBinding;

  bool mRunning;
  int  mUDPSocket;
  // int  mTCPSocket;

  mutable std::mutex       mMutex;
  std::map<IP,std::string> mCache;

  mutable std::mutex       mQueryMutex;
  std::map<uint16_t,Query> mQueries;
  uint16_t mQueryID;

public:
  DNSProxy(const Address& upstream, const Address& binding);
  ~DNSProxy();

  void parse(const uint8_t* buffer, ssize_t length);

  const std::string* get(const IP& ip) const;

  void clean(uint64_t timeout);
  void start();
  void stop();
};

#endif
