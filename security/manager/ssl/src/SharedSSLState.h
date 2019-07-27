





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
  void SetOCSPStaplingEnabled(bool staplingEnabled)
  {
    mOCSPStaplingEnabled = staplingEnabled;
  }

  
  bool SocketCreated();
  void NoteSocketCreated();
  static void NoteCertOverrideServiceInstantiated();
  static void NoteCertDBServiceInstantiated();
  bool IsOCSPStaplingEnabled() const { return mOCSPStaplingEnabled; }

private:
  ~SharedSSLState();

  void Cleanup();

  nsCOMPtr<nsIObserver> mObserver;
  RefPtr<nsClientAuthRememberService> mClientAuthRemember;
  nsSSLIOLayerHelpers mIOLayerHelpers;

  
  
  
  Mutex mMutex;
  bool mSocketCreated;
  bool mOCSPStaplingEnabled;
};

SharedSSLState* PublicSSLState();
SharedSSLState* PrivateSSLState();

} 
} 

#endif
