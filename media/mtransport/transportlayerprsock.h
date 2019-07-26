







#ifndef transportlayerprsock_h__
#define transportlayerprsock_h__

#include "nspr.h"
#include "prio.h"

#include "nsASocketHandler.h"
#include "nsCOMPtr.h"
#include "nsISocketTransportService.h"
#include "nsXPCOM.h"

#include "m_cpp_utils.h"
#include "transportflow.h"
#include "transportlayer.h"

namespace mozilla {

class TransportLayerPrsock : public TransportLayer {
 public:
  TransportLayerPrsock() : fd_(nullptr), handler_() {}

  virtual ~TransportLayerPrsock() {
    Detach();
  }


  
  virtual nsresult InitInternal();

  void Import(PRFileDesc *fd, nsresult *result);

  void Detach() {
    handler_->Detach();
  }

  
  virtual TransportResult SendPacket(const unsigned char *data, size_t len);

  TRANSPORT_LAYER_ID("prsock")

 private:
  DISALLOW_COPY_ASSIGN(TransportLayerPrsock);

  
  class SocketHandler : public nsASocketHandler {
   public:
      SocketHandler(TransportLayerPrsock *prsock, PRFileDesc *fd) :
        prsock_(prsock), fd_(fd) {
        mPollFlags = PR_POLL_READ;
      }
      virtual ~SocketHandler() {}

      void Detach() {
        mCondition = NS_BASE_STREAM_CLOSED;
        prsock_ = nullptr;
      }

      
      virtual void OnSocketReady(PRFileDesc *fd, int16_t outflags) {
        if (prsock_) {
          prsock_->OnSocketReady(fd, outflags);
        }
      }

      virtual void OnSocketDetached(PRFileDesc *fd) {
        if (prsock_) {
          prsock_->OnSocketDetached(fd);
        }
        PR_Close(fd_);
      }

      virtual void IsLocal(bool *aIsLocal) {
        
        *aIsLocal = false;
      }

      
      NS_DECL_ISUPPORTS

      private:
      TransportLayerPrsock *prsock_;
      PRFileDesc *fd_;
   private:
    DISALLOW_COPY_ASSIGN(SocketHandler);
  };

  
  friend class SocketHandler;

  
  void OnSocketReady(PRFileDesc *fd, int16_t outflags);
  void OnSocketDetached(PRFileDesc *fd) {
    SetState(TS_CLOSED);
  }
  void IsLocal(bool *aIsLocal) {
    
    *aIsLocal = false;
  }

  PRFileDesc *fd_;
  nsCOMPtr<SocketHandler> handler_;
  nsCOMPtr<nsISocketTransportService> stservice_;

};



}  
#endif
