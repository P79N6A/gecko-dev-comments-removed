






































#ifndef mozilla_dom_system_b2g_systemworkermanager_h__
#define mozilla_dom_system_b2g_systemworkermanager_h__

#include "TelephonyCommon.h"

#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"

class nsIWorkerHolder;

BEGIN_TELEPHONY_NAMESPACE

class SystemWorkerManager : public nsIObserver,
                            public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR

  nsresult Init();
  void Shutdown();

  static already_AddRefed<SystemWorkerManager>
  FactoryCreate();

  static nsIInterfaceRequestor*
  GetInterfaceRequestor();

private:
  SystemWorkerManager();
  ~SystemWorkerManager();

  nsresult InitTelephone(JSContext *cx);
  nsresult InitWifi(JSContext *cx);

  nsCOMPtr<nsIWorkerHolder> mTelephoneWorker;
  nsCOMPtr<nsIWorkerHolder> mWifiWorker;

  bool mShutdown;
};

END_TELEPHONY_NAMESPACE

#endif 
