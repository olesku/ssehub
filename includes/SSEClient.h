#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <deque>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "HTTPRequest.h"

#define IOVEC_SIZE 512
#define SND_NO_FLUSH false

class SSEChannel;

enum SubscriptionType {
  SUBSCRIPTION_ID,
  SUBSCRIPTION_EVENT_TYPE
};

typedef struct {
  string key;
  SubscriptionType type;
} SubscriptionElement;

using namespace std;

class SSEClient {
  public:
    SSEClient(int, struct sockaddr_in* csin);
    ~SSEClient();
    int Send(const string &data, bool flush=true);
    int Read(void* buf, int len);
    int Getfd();
    SSEChannel* GetChannel();
    HTTPRequest* GetHttpReq();
    const string GetIP();
    uint32_t GetSockAddr();
    void SetChannel(SSEChannel* chan);
    void MarkAsDead();
    bool IsDead();
    void Destroy();
    void DeleteHttpReq();
    bool isSubscribed(const string key, SubscriptionType type);
    void Subscribe(const string key, SubscriptionType type);
    bool isFilterAcceptable(const string& data);
    void FlushAndDestroy();
    bool isDestroyAfterFlush();
    size_t Flush();

   private:
    int _fd;
    struct sockaddr_in _csin;
    bool _dead;
    SSEChannel* _channel;
    bool _isEventFiltered;
    bool _isIdFiltered;
    bool _destroyAfterFlush;
    string _sndBuf;
    vector<SubscriptionElement> _subscriptions;
    boost::mutex _sndBufLock;
    boost::shared_ptr<HTTPRequest> m_httpReq;
    size_t _prune_sendbuffer_bytes(size_t bytes);
    int _write_sndbuf();
    const string _get_sse_field(const string& data, const string& fieldName);
};

#endif
