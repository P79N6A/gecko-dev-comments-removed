



#ifndef peerconnectionctx_h___h__
#define peerconnectionctx_h___h__

#include <string>

#include "mozilla/Attributes.h"
#include "StaticPtr.h"
#include "PeerConnectionImpl.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsIRunnable.h"

namespace mozilla {
class PeerConnectionCtxShutdown;

namespace dom {
class WebrtcGlobalInformation;
}





class PeerConnectionCtx {
 public:
  static nsresult InitializeGlobal(nsIThread *mainThread, nsIEventTarget *stsThread);
  static PeerConnectionCtx* GetInstance();
  static bool isActive();
  static void Destroy();

  bool isReady() {
    
    if (mGMPService) {
      return mGMPReady;
    }
    return true;
  }

  void queueJSEPOperation(nsIRunnable* aJSEPOperation);
  void onGMPReady();

  bool gmpHasH264();

  
  friend class PeerConnectionImpl;
  friend class PeerConnectionWrapper;
  friend class mozilla::dom::WebrtcGlobalInformation;

#ifdef MOZILLA_INTERNAL_API
  
  
  mozilla::dom::Sequence<mozilla::dom::RTCStatsReportInternal>
    mStatsForClosedPeerConnections;
#endif

 private:
  
  std::map<const std::string, PeerConnectionImpl *> mPeerConnections;

  PeerConnectionCtx() :  mGMPReady(false) {}
  
  PeerConnectionCtx(const PeerConnectionCtx& other) = delete;
  void operator=(const PeerConnectionCtx& other) = delete;
  virtual ~PeerConnectionCtx();

  nsresult Initialize();
  nsresult Cleanup();

  void initGMP();

  static void
  EverySecondTelemetryCallback_m(nsITimer* timer, void *);

#ifdef MOZILLA_INTERNAL_API
  
  int mConnectionCounter;

  nsCOMPtr<nsITimer> mTelemetryTimer;

public:
  
  
  nsTArray<nsAutoPtr<mozilla::dom::RTCStatsReportInternal>> mLastReports;
private:
#endif

  
  
  
  
  
  nsCOMPtr<mozIGeckoMediaPluginService> mGMPService;
  bool mGMPReady;
  nsTArray<nsCOMPtr<nsIRunnable>> mQueuedJSEPOperations;

  static PeerConnectionCtx *gInstance;
public:
  static nsIThread *gMainThread;
  static mozilla::StaticRefPtr<mozilla::PeerConnectionCtxShutdown> gPeerConnectionCtxShutdown;
};

}  

#endif
