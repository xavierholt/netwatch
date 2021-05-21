#include "Server.h"
#include "Watcher.h"

#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(Watcher* watcher, unsigned short port) {
  mRunning = true;
  mWatcher = watcher;
  mSocket  = socket(AF_INET , SOCK_STREAM , 0);

  sockaddr_in binding{
    .sin_family      = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port        = htons(port)
  };

  int dummy;
  if(setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int)) < 0) {
    throw std::runtime_error("Could not make socket reusable.");
  }

  if(bind(mSocket, (struct sockaddr*) &binding, sizeof(binding)) < 0) {
    throw std::runtime_error("Could not bind to socket.");
  }
}

Server::~Server() {
  close(mSocket);
}

void Server::start() {
  ::listen(mSocket, 5);

  sockaddr_in remote;
  socklen_t   remlen;

  while(mRunning) {
    int client = accept(mSocket, (struct sockaddr*) &remote, &remlen);
    if(client < 0) {
      // Accept failed...
      continue;
    }

    std::ostringstream stream;
    mWatcher->render(stream);
    std::string str = stream.str();
    write(client, str.data(), str.length());
    shutdown(client, SHUT_RDWR);
    close(client);
  }
}

void Server::stop() {
  mRunning = false;
}
