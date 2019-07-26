
















#ifndef mozilla_dom_system_b2g_systemworkermanager_h__
#define mozilla_dom_system_b2g_systemworkermanager_h__

#include "nsIInterfaceRequestor.h"
#include "nsISystemWorkerManager.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsXULAppAPI.h" 

class nsIWorkerHolder;

namespace mozilla {

namespace ipc {
  class KeyStore;
}

namespace dom {
namespace gonk {

class SystemWorkerManager : public nsIObserver,
                            public nsIInterfaceRequestor,
                            public nsISystemWorkerManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSISYSTEMWORKERMANAGER

  nsresult Init();
  void Shutdown();

  static already_AddRefed<SystemWorkerManager>
  FactoryCreate();

  static nsIInterfaceRequestor*
  GetInterfaceRequestor();

private:
  SystemWorkerManager();
  ~SystemWorkerManager();

  nsresult InitNetd(JSContext *cx);
  nsresult InitWifi(JSContext *cx);
  nsresult InitKeyStore(JSContext *cx);

  nsCOMPtr<nsIWorkerHolder> mNetdWorker;
  nsCOMPtr<nsIWorkerHolder> mWifiWorker;

  nsRefPtr<ipc::KeyStore> mKeyStore;

  bool mShutdown;
};

}
}
}

#endif 
