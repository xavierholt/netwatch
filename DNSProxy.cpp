#include "DNSProxy.h"

#include "Headers.h"

#include <string>
#include <unistd.h>

#include <iostream>

DNSProxy::DNSProxy(const Address& upstream, const Address& binding): mUpstream(upstream), mBinding(binding) {
  mUDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
  mRunning   = true;

  int dummy;
  if(setsockopt(mUDPSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int)) < 0) {
    throw std::runtime_error("Could not make socket reusable.");
  }

  if(bind(mUDPSocket, mBinding) < 0) {
    throw std::runtime_error("Could not bind to socket.");
  }

  // Address sockname;
  // getsockname(mUDPSocket, (struct sockaddr *) &sockname.address, &sockname.length);
  // std::cout << "Sockname: " << sockname << "\n";
}

DNSProxy::~DNSProxy() {
  close(mUDPSocket);
}

void DNSProxy::clean(uint64_t timeout) {
  std::lock_guard lock(mQueryMutex);
  auto itr = mQueries.begin();
  while(itr != mQueries.end()) {
    if(itr->second.time < timeout) {
      itr = mQueries.erase(itr);
    }
    else {
      ++itr;
    }
  }
}

const std::string* DNSProxy::get(const IP& ip) const {
  std::lock_guard lock(mMutex);

  auto itr = mCache.find(ip);
  if(itr == mCache.end()) return nullptr;
  return &itr->second;
}

std::string readname(const uint8_t* buffer, ssize_t length, ssize_t offset) {
  std::string result;

  while(true) {
    uint8_t len = buffer[offset];

    if(len >= 192) {
      // Pointer. Follow it!
      offset = buffer[offset + 1];
    }
    else if(offset + len >= length) {
      throw std::runtime_error("readname(): Index out of bounds!");
    }
    else if(len == 0) {
      // End of name. We're done.
      return result;
    }
    else {
      // Domain chunk. Concatenate...
      if(result.length() > 0) result += '.';
      result.append((const char*) &buffer[offset + 1], len);
      offset += len + 1;
    }
  }
}

ssize_t skipname(const uint8_t* buffer, ssize_t length, ssize_t offset) {
  while(true) {
    uint8_t len = buffer[offset];
    // std::cout << "skipname: " << offset << '/' << length  << " (" << int(len) << ")\n";

    if(len >= 192) {
      // Pointer. Skip it and we're done.
      return offset + 2;
    }
    else if(offset + len >= length) {
      throw std::runtime_error("skipname(): Index out of bounds!");
    }
    else if(len == 0) {
      // End of name. We're done.
      return offset + 1;
    }
    else {
      // Length. Skip ahead.
      offset += len + 1;
    }
  }
}

void DNSProxy::parse(const uint8_t* buffer, ssize_t length) {
  DNSHeader* header = (DNSHeader*) buffer;

  int nquestions   = ntohs(header->nquestions);
  int nanswers     = ntohs(header->nanswers);
  // int nauthorities = ntohs(header->nauthorities);
  // int nextras      = ntohs(header->nextras);

  // std::cout << nquestions << ' ' << nanswers << '\n';

  ssize_t offset = sizeof(DNSHeader);
  for(int i = 0; i < nquestions; ++i) {
    // std::cout << "Question " << (i + 1) << '@' << offset << '\n';
    offset = skipname(buffer, length, offset) + 4; // Query data is 4 bytes.
  }

  for(int i = 0; i < nanswers; ++i) {
    ssize_t tmpoff = offset;
    offset = skipname(buffer, length, offset);

    DNSRecord* record = (DNSRecord*) &buffer[offset];
    uint16_t rdlen = ntohs(record->rdlength);


    // std::cout << "Answer " << (i + 1) << '@' << tmpoff << ": " << ntohs(record->type) << " " << rdlen << '\n';

    offset += 10 + rdlen; // Record data is 10 bytes, but sizeof(DNSRecord) is 12...


    if(record->type == htons(1) && rdlen == 4) {
      std::string name = readname(buffer, length, tmpoff);
      std::cout << IP((in_addr*) record->rdata) << " => " << name << '\n';

      std::lock_guard lock(mMutex);
      mCache[IP((in_addr*) record->rdata)] = name;
    }
    else if(record->type == htons(28) && rdlen == 16) {
      std::string name = readname(buffer, length, tmpoff);
      std::cout << IP((in6_addr*) record->rdata) << " => " << name << '\n';

      std::lock_guard lock(mMutex);
      mCache[IP((in6_addr*) record->rdata)] = name;
    }
  }
}

void DNSProxy::start() {
  uint8_t buffer[4096];
  ::listen(mUDPSocket, 5);

  Query query;
  query.time = time(0) * 1000;

  while(mRunning) {
    ssize_t length = recvfrom(mUDPSocket, buffer, 4096, query.address);
    // std::cout << "Got a " << length << "-byte UDP packet.\n";
    if(length < 0) continue;

    DNSHeader* header = (DNSHeader*) buffer;
    uint16_t   flags  = ntohs(header->flags);

    if((flags & 0x8000) == 0) {
      // Query: forward to upstream resolver.
      uint16_t id  = ++mQueryID;
      query.id     = header->id;
      header->id   = id;

      // std::cout << "  It's a query - sending to " << mUpstream << "\n";
      if(sendto(mUDPSocket, buffer, length, mUpstream) < 0) {
        perror("Could not forward DNS query");
      }

      std::lock_guard lock(mQueryMutex);
      mQueries[id] = query;
    }
    else {
      // Response: parse and return to client.
      auto itr = mQueries.find(header->id);
      if(itr == mQueries.end()) continue;

      // std::cout << "  It's a reply - sending back to " << itr->second.address << "\n";

      header->id = itr->second.id;
      if(sendto(mUDPSocket, buffer, length, itr->second.address) < 0) {
        perror("Could not forward DNS response");
      }

      mQueries.erase(itr);
      if((flags & 0x000f) == 0) {
        // std::cout << std::string((char*) buffer, length) << std::endl;
        // No error: parse it!
        parse(buffer, length);
      }
    }
  }
}

void DNSProxy::stop() {
  mRunning = false;
}
