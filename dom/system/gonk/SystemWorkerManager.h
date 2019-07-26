
















#ifndef mozilla_dom_system_b2g_systemworkermanager_h__
#define mozilla_dom_system_b2g_systemworkermanager_h__

#include "nsIInterfaceRequestor.h"
#include "nsISystemWorkerManager.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsDOMEventTargetHelper.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

class nsIWorkerHolder;

namespace mozilla {

namespace ipc {
  class RilConsumer;
  class UnixSocketRawData;
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

  static bool SendRilRawData(unsigned long aClientId,
                             ipc::UnixSocketRawData* aRaw);

private:
  SystemWorkerManager();
  ~SystemWorkerManager();

#ifdef MOZ_WIDGET_GONK
  nsresult InitNetd(JSContext *cx);
#endif
  nsresult InitWifi(JSContext *cx);

#ifdef MOZ_WIDGET_GONK
  nsCOMPtr<nsIWorkerHolder> mNetdWorker;
#endif
  nsCOMPtr<nsIWorkerHolder> mWifiWorker;

  nsTArray<nsRefPtr<ipc::RilConsumer> > mRilConsumers;

  bool mShutdown;
};

}
}
}

#endif 
