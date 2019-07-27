





#ifndef mozilla_dom_cache_PrincipalVerifier_h
#define mozilla_dom_cache_PrincipalVerifier_h

#include "mozilla/ipc/PBackgroundSharedTypes.h"
#include "nsThreadUtils.h"
#include "nsTObserverArray.h"

namespace mozilla {

namespace ipc {
  class PBackgroundParent;
}

namespace dom {
namespace cache {

class ManagerId;

class PrincipalVerifier final : public nsRunnable
{
public:
  
  
  
  
  class Listener
  {
  public:
    virtual void OnPrincipalVerified(nsresult aRv, ManagerId* aManagerId) = 0;
  };

  static already_AddRefed<PrincipalVerifier>
  CreateAndDispatch(Listener* aListener, mozilla::ipc::PBackgroundParent* aActor,
                    const mozilla::ipc::PrincipalInfo& aPrincipalInfo);

  void AddListener(Listener* aListener);

  
  
  void RemoveListener(Listener* aListener);

private:
  PrincipalVerifier(Listener* aListener, mozilla::ipc::PBackgroundParent* aActor,
                    const mozilla::ipc::PrincipalInfo& aPrincipalInfo);
  virtual ~PrincipalVerifier();

  void VerifyOnMainThread();
  void CompleteOnInitiatingThread();

  void DispatchToInitiatingThread(nsresult aRv);

  
  typedef nsTObserverArray<Listener*> ListenerList;
  ListenerList mListenerList;

  
  
  nsRefPtr<ContentParent> mActor;

  const mozilla::ipc::PrincipalInfo mPrincipalInfo;
  nsCOMPtr<nsIThread> mInitiatingThread;
  nsresult mResult;
  nsRefPtr<ManagerId> mManagerId;

public:
  NS_DECL_NSIRUNNABLE
};

} 
} 
} 

#endif
