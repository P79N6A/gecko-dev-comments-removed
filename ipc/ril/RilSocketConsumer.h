





#ifndef mozilla_ipc_RilSocketConsumer_h
#define mozilla_ipc_RilSocketConsumer_h

#include "nsAutoPtr.h"

class JSContext;

namespace mozilla {
namespace ipc {

class UnixSocketBuffer;




class RilSocketConsumer
{
public:
  






  virtual void ReceiveSocketData(JSContext* aCx,
                                 int aIndex,
                                 nsAutoPtr<UnixSocketBuffer>& aBuffer) = 0;

  




  virtual void OnConnectSuccess(int aIndex) = 0;

  




  virtual void OnConnectError(int aIndex) = 0;

  




  virtual void OnDisconnect(int aIndex) = 0;

protected:
  virtual ~RilSocketConsumer();
};

}
}

#endif
