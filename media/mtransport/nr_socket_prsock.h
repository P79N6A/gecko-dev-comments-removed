












































#ifndef nr_socket_prsock__
#define nr_socket_prsock__

#include <vector>

#include "nspr.h"
#include "prio.h"

#include "nsCOMPtr.h"
#include "nsASocketHandler.h"
#include "nsISocketTransportService.h"
#include "nsXPCOM.h"
#include "nsIEventTarget.h"

#include "m_cpp_utils.h"

namespace mozilla {

class NrSocket : public nsASocketHandler {
public:
  NrSocket() : fd_(nullptr) {
    memset(&my_addr_, 0, sizeof(my_addr_));
    memset(cbs_, 0, sizeof(cbs_));
    memset(cb_args_, 0, sizeof(cb_args_));
  }
  virtual ~NrSocket() {
    PR_Close(fd_);
  }

  
  virtual void OnSocketReady(PRFileDesc *fd, int16_t outflags);
  virtual void OnSocketDetached(PRFileDesc *fd);
  virtual void IsLocal(bool *aIsLocal);
  virtual uint64_t ByteCountSent() { return 0; }
  virtual uint64_t ByteCountReceived() { return 0; }

  
  NS_DECL_ISUPPORTS

  
  int async_wait(int how, NR_async_cb cb, void *cb_arg,
                 char *function, int line);
  int cancel(int how);


  
  int create(nr_transport_addr *addr); 
  int sendto(const void *msg, size_t len,
             int flags, nr_transport_addr *to);
  int recvfrom(void * buf, size_t maxlen,
               size_t *len, int flags,
               nr_transport_addr *from);
  int getaddr(nr_transport_addr *addrp);
  void close();

private:
  DISALLOW_COPY_ASSIGN(NrSocket);

  void fire_callback(int how);

  PRFileDesc *fd_;
  nr_transport_addr my_addr_;
  NR_async_cb cbs_[NR_ASYNC_WAIT_WRITE + 1];
  void *cb_args_[NR_ASYNC_WAIT_WRITE + 1];
  nsCOMPtr<nsIEventTarget> ststhread_;
};

int nr_praddr_to_transport_addr(const PRNetAddr *praddr,
                                nr_transport_addr *addr, int keep);

}  
#endif
