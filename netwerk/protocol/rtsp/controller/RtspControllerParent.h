





#ifndef RtspControllerParent_h
#define RtspControllerParent_h

#include "mozilla/net/PRtspControllerParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsIStreamingProtocolController.h"
#include "nsILoadContext.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {

class RtspControllerParent : public PRtspControllerParent
                           , public nsIStreamingProtocolListener
{
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMINGPROTOCOLLISTENER

  RtspControllerParent();

  bool RecvAsyncOpen(const URIParams& aURI);
  bool RecvPlay();
  bool RecvPause();
  bool RecvResume();
  bool RecvSuspend();
  bool RecvSeek(const uint64_t& offset);
  bool RecvStop();
  bool RecvPlaybackEnded();

 protected:
  ~RtspControllerParent();

 private:
  bool mIPCOpen;
  void ActorDestroy(ActorDestroyReason why);
  
  nsCOMPtr<nsIURI> mURI;
  
  nsCOMPtr<nsIStreamingProtocolController> mController;
  uint32_t mTotalTracks;
  
  void Destroy();
};

} 
} 
#endif 
