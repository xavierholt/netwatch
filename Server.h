#ifndef SERVER_H
#define SERVER_H

class Watcher;

class Server {
  Watcher* mWatcher;
  bool     mRunning;
  int      mSocket;
public:
  Server(Watcher* watcher, unsigned short port = 9999);
  ~Server();

  void start();
  void stop();
};

#endif
