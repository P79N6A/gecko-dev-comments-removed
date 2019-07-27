





#ifndef RtspController_h
#define RtspController_h

#include "mozilla/Mutex.h"
#include "nsIStreamingProtocolController.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsITimer.h"
#include "RTSPSource.h"

namespace mozilla {
namespace net {

class RtspController : public nsIStreamingProtocolController
                     , public nsIStreamingProtocolListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMINGPROTOCOLCONTROLLER
  NS_DECL_NSISTREAMINGPROTOCOLLISTENER

  RtspController(nsIChannel *channel);
  ~RtspController();

  
  static void PlayTimerCallback(nsITimer *aTimer, void *aClosure);
  static void PauseTimerCallback(nsITimer *aTimer, void *aClosure);

private:
  enum State {
    INIT,
    CONNECTED,
    DISCONNECTED
  };

  
  nsCOMPtr<nsIURI> mURI;
  
  nsCOMPtr<nsIStreamingProtocolListener> mListener;
  
  nsCString mSpec;
  
  nsCString mUserAgent;
  
  
  State mState;
  
  android::sp<android::RTSPSource> mRtspSource;
  
  Mutex mTimerLock;
  
  
  
  nsCOMPtr<nsITimer> mPlayTimer;
  nsCOMPtr<nsITimer> mPauseTimer;
};

}
} 
#endif
