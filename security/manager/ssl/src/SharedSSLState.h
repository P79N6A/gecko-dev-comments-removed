





#ifndef SharedSSLState_h
#define SharedSSLState_h

#include "mozilla/RefPtr.h"
#include "nsNSSIOLayer.h"

class nsClientAuthRememberService;
class nsIRecentBadCertsService;
class nsICertOverrideService;
class nsIObserver;

namespace mozilla {
namespace psm {

class SharedSSLState {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SharedSSLState)
  SharedSSLState();

  static void GlobalInit();
  static void GlobalCleanup();

  nsClientAuthRememberService* GetClientAuthRememberService() {
    return mClientAuthRemember;
  }

  nsSSLIOLayerHelpers& IOLayerHelpers() {
    return mIOLayerHelpers;
  }

  void ResetStoredData();
  void NotePrivateBrowsingStatus();

private:
  void Cleanup();

  nsCOMPtr<nsIObserver> mObserver;
  RefPtr<nsClientAuthRememberService> mClientAuthRemember;
  nsSSLIOLayerHelpers mIOLayerHelpers;
};

SharedSSLState* PublicSSLState();
SharedSSLState* PrivateSSLState();

} 
} 

#endif
