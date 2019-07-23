




































#include "ipcIDConnectService.h"
#include "ipcdclient.h"

#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"

#include "nsCOMPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "xptcall.h"
#include "xptinfo.h"

class DConnectInstance;
typedef nsClassHashtable<nsVoidPtrHashKey, DConnectInstance> DConnectInstanceSet;

class ipcDConnectService : public ipcIDConnectService
                         , public ipcIMessageObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IPCIDCONNECTSERVICE
  NS_DECL_IPCIMESSAGEOBSERVER

  NS_HIDDEN_(nsresult) Init();

  NS_HIDDEN_(nsresult) GetInterfaceInfo(const nsID &iid, nsIInterfaceInfo **);
  NS_HIDDEN_(nsresult) GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                                            const nsXPTMethodInfo *methodInfo,
                                            const nsXPTParamInfo &paramInfo,
                                            const nsXPTType &type,
                                            PRUint16 methodIndex,
                                            PRUint8 paramIndex,
                                            nsXPTCMiniVariant *dispatchParams,
                                            PRBool isFullVariantArray,
                                            nsID &result);

  NS_HIDDEN_(nsresult) StoreInstance(DConnectInstance *);
  NS_HIDDEN_(void)     DeleteInstance(DConnectInstance *);

private:

  NS_HIDDEN ~ipcDConnectService();

  NS_HIDDEN_(void) OnSetup(PRUint32 peer, const struct DConnectSetup *, PRUint32 opLen);
  NS_HIDDEN_(void) OnRelease(PRUint32 peer, const struct DConnectRelease *);
  NS_HIDDEN_(void) OnInvoke(PRUint32 peer, const struct DConnectInvoke *, PRUint32 opLen);

private:
  nsCOMPtr<nsIInterfaceInfoManager> mIIM;

  
  DConnectInstanceSet mInstances;
};

#define IPC_DCONNECTSERVICE_CLASSNAME \
  "ipcDConnectService"
#define IPC_DCONNECTSERVICE_CONTRACTID \
  "@mozilla.org/ipc/dconnect-service;1"
#define IPC_DCONNECTSERVICE_CID                    \
{ /* 63a5d9dc-4828-425a-bd50-bd10a4b26f2c */       \
  0x63a5d9dc,                                      \
  0x4828,                                          \
  0x425a,                                          \
  {0xbd, 0x50, 0xbd, 0x10, 0xa4, 0xb2, 0x6f, 0x2c} \
}
