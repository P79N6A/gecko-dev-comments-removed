





#ifndef SharedSSLState_h
#define SharedSSLState_h

#include "mozilla/RefPtr.h"
#include "nsNSSIOLayer.h"

class nsClientAuthRememberService;
class nsIObserver;

namespace mozilla {
namespace psm {

class SharedSSLState {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SharedSSLState)
  SharedSSLState();
  ~SharedSSLState();

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

  
  bool SocketCreated();
  void NoteSocketCreated();
  static void NoteCertOverrideServiceInstantiated();
  static void NoteCertDBServiceInstantiated();

private:
  void Cleanup();

  nsCOMPtr<nsIObserver> mObserver;
  RefPtr<nsClientAuthRememberService> mClientAuthRemember;
  nsSSLIOLayerHelpers mIOLayerHelpers;

  
  
  
  Mutex mMutex;
  bool mSocketCreated;
};

SharedSSLState* PublicSSLState();
SharedSSLState* PrivateSSLState();

} 
} 

#endif
