












































#ifndef nr_socket_prsock__
#define nr_socket_prsock__

#include <queue>

#include "nspr.h"
#include "prio.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsASocketHandler.h"
#include "nsISocketTransportService.h"
#include "nsXPCOM.h"
#include "nsIEventTarget.h"
#include "nsIUDPSocketChild.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "databuffer.h"
#include "m_cpp_utils.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/ClearOnShutdown.h"


typedef struct nr_socket_vtbl_ nr_socket_vtbl;

namespace mozilla {

namespace net {
  union NetAddr;
}

class NrSocketBase {
public:
  NrSocketBase() : connect_invoked_(false), poll_flags_(0) {
    memset(cbs_, 0, sizeof(cbs_));
    memset(cb_args_, 0, sizeof(cb_args_));
    memset(&my_addr_, 0, sizeof(my_addr_));
  }
  virtual ~NrSocketBase() {}

  
  virtual int create(nr_transport_addr *addr) = 0;
  virtual int sendto(const void *msg, size_t len,
                     int flags, nr_transport_addr *to) = 0;
  virtual int recvfrom(void * buf, size_t maxlen,
                       size_t *len, int flags,
                       nr_transport_addr *from) = 0;
  virtual int getaddr(nr_transport_addr *addrp) = 0;
  virtual void close() = 0;
  virtual int connect(nr_transport_addr *addr) = 0;
  virtual int write(const void *msg, size_t len, size_t *written) = 0;
  virtual int read(void* buf, size_t maxlen, size_t *len) = 0;

   
  virtual int async_wait(int how, NR_async_cb cb, void *cb_arg,
                         char *function, int line);
  virtual int cancel(int how);

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  uint32_t poll_flags() {
    return poll_flags_;
  }

  virtual nr_socket_vtbl *vtbl();  

  static TimeStamp short_term_violation_time();
  static TimeStamp long_term_violation_time();
  const nr_transport_addr& my_addr() const {
    return my_addr_;
  }

protected:
  void fire_callback(int how);

  bool connect_invoked_;
  nr_transport_addr my_addr_;

private:
  NR_async_cb cbs_[NR_ASYNC_WAIT_WRITE + 1];
  void *cb_args_[NR_ASYNC_WAIT_WRITE + 1];
  uint32_t poll_flags_;
};

class NrSocket : public NrSocketBase,
                 public nsASocketHandler {
public:
  NrSocket() : fd_(nullptr) {}

  
  virtual void OnSocketReady(PRFileDesc *fd, int16_t outflags) override;
  virtual void OnSocketDetached(PRFileDesc *fd) override;
  virtual void IsLocal(bool *aIsLocal) override;
  virtual uint64_t ByteCountSent() override { return 0; }
  virtual uint64_t ByteCountReceived() override { return 0; }

  
  NS_DECL_THREADSAFE_ISUPPORTS

  
  virtual int async_wait(int how, NR_async_cb cb, void *cb_arg,
                         char *function, int line) override;
  virtual int cancel(int how) override;


  
  virtual int create(nr_transport_addr *addr) override; 
  virtual int sendto(const void *msg, size_t len,
                     int flags, nr_transport_addr *to) override;
  virtual int recvfrom(void * buf, size_t maxlen,
                       size_t *len, int flags,
                       nr_transport_addr *from) override;
  virtual int getaddr(nr_transport_addr *addrp) override;
  virtual void close() override;
  virtual int connect(nr_transport_addr *addr) override;
  virtual int write(const void *msg, size_t len, size_t *written) override;
  virtual int read(void* buf, size_t maxlen, size_t *len) override;

protected:
  virtual ~NrSocket() {
    if (fd_)
      PR_Close(fd_);
  }

  DISALLOW_COPY_ASSIGN(NrSocket);

  PRFileDesc *fd_;
  nsCOMPtr<nsIEventTarget> ststhread_;
};

struct nr_udp_message {
  nr_udp_message(const PRNetAddr &from, nsAutoPtr<DataBuffer> &data)
      : from(from), data(data) {
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nr_udp_message);

  PRNetAddr from;
  nsAutoPtr<DataBuffer> data;

private:
  ~nr_udp_message() {}
  DISALLOW_COPY_ASSIGN(nr_udp_message);
};

class NrSocketIpc : public NrSocketBase {
public:

  enum NrSocketIpcState {
    NR_INIT,
    NR_CONNECTING,
    NR_CONNECTED,
    NR_CLOSING,
    NR_CLOSED,
  };

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrSocketIpc, override)

  NS_IMETHODIMP CallListenerError(const nsACString &message,
                                  const nsACString &filename,
                                  uint32_t line_number);
  NS_IMETHODIMP CallListenerReceivedData(const nsACString &host,
                                         uint16_t port,
                                         const uint8_t *data,
                                         uint32_t data_length);
  NS_IMETHODIMP CallListenerOpened();
  NS_IMETHODIMP CallListenerClosed();

  NrSocketIpc();

  
  virtual int create(nr_transport_addr *addr) override;
  virtual int sendto(const void *msg, size_t len,
                     int flags, nr_transport_addr *to) override;
  virtual int recvfrom(void * buf, size_t maxlen,
                       size_t *len, int flags,
                       nr_transport_addr *from) override;
  virtual int getaddr(nr_transport_addr *addrp) override;
  virtual void close() override;
  virtual int connect(nr_transport_addr *addr) override;
  virtual int write(const void *msg, size_t len, size_t *written) override;
  virtual int read(void* buf, size_t maxlen, size_t *len) override;

private:
  virtual ~NrSocketIpc();

  DISALLOW_COPY_ASSIGN(NrSocketIpc);

  static nsIThread* GetIOThreadAndAddUse_s();

  
  void create_i(const nsACString &host, const uint16_t port);
  void sendto_i(const net::NetAddr &addr, nsAutoPtr<DataBuffer> buf);
  void close_i();
#if defined(MOZILLA_INTERNAL_API) && !defined(MOZILLA_XPCOMRT_API)
  static void release_child_i(nsIUDPSocketChild* aChild, nsCOMPtr<nsIEventTarget> ststhread);
  static void release_use_s();
#endif
  
  void recv_callback_s(RefPtr<nr_udp_message> msg);

  bool err_;
  NrSocketIpcState state_;
  std::queue<RefPtr<nr_udp_message> > received_msgs_;

  nsRefPtr<nsIUDPSocketChild> socket_child_; 
  nsCOMPtr<nsIEventTarget> sts_thread_;
  const nsCOMPtr<nsIEventTarget> io_thread_;
  ReentrantMonitor monitor_;
};



class NrSocketIpcProxy : public nsIUDPSocketInternal {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKETINTERNAL

  nsresult Init(const nsRefPtr<NrSocketIpc>& socket);

private:
  virtual ~NrSocketIpcProxy();

  nsRefPtr<NrSocketIpc> socket_;
  nsCOMPtr<nsIEventTarget> sts_thread_;
};

int nr_netaddr_to_transport_addr(const net::NetAddr *netaddr,
                                 nr_transport_addr *addr,
                                 int protocol);
int nr_praddr_to_transport_addr(const PRNetAddr *praddr,
                                nr_transport_addr *addr,
                                int protocol, int keep);
int nr_transport_addr_get_addrstring_and_port(nr_transport_addr *addr,
                                              nsACString *host, int32_t *port);
}  
#endif
