#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "ConcurrentQueue.h"

using namespace std;

// Forward declarations.
class SSEClient;

typedef boost::shared_ptr<SSEClient> SSEClientPtr;
typedef list<SSEClientPtr> SSEClientPtrList;

typedef struct {
  string data;
  string target;
} msg_t;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();
    void AddClient(SSEClient* client);
    void Broadcast(msg_t& msg);
    size_t GetNumClients();

  private:
    int _id;
    size_t _connected_clients;
    SSEClientPtrList _clientlist;
    boost::mutex _clientlist_lock;
    boost::thread _processorthread;
    ConcurrentQueue<msg_t> _msgqueue;

    void ProcessQueue();
};

#endif
