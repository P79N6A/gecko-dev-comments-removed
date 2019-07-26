





#ifndef NetworkActivityMonitor_h___
#define NetworkActivityMonitor_h___

#include "nsCOMPtr.h"
#include "prio.h"
#include "prinrval.h"

class nsIObserverService;

namespace mozilla { namespace net {

class NetworkActivityMonitor
{
public:
  enum Direction {
    kUpload   = 0,
    kDownload = 1
  };

  NetworkActivityMonitor();
  ~NetworkActivityMonitor();

  static nsresult Init(int32_t blipInterval);
  static nsresult Shutdown();

  static nsresult AttachIOLayer(PRFileDesc *fd);
  static nsresult DataInOut(Direction direction);

private:
  nsresult Init_Internal(int32_t blipInterval);
  void PostNotification(Direction direction);

  static NetworkActivityMonitor * gInstance;
  PRDescIdentity                  mLayerIdentity;
  PRIOMethods                     mLayerMethods;
  PRIntervalTime                  mBlipInterval;
  PRIntervalTime                  mLastNotificationTime[2];
  nsCOMPtr<nsIObserverService>    mObserverService;
};

}} 

#endif
