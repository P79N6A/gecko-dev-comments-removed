



































#ifndef ipcdclient_h__
#define ipcdclient_h__











#include "nscore.h"
#include "nsID.h"
#include "nsError.h"
#include "ipcIMessageObserver.h"
#include "ipcIClientObserver.h"



#define IPC_METHOD NS_HIDDEN_(nsresult)



#define IPC_SENDER_ANY PR_UINT32_MAX




#define IPC_WAIT_NEXT_MESSAGE \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 10)


#define IPC_ERROR_WOULD_BLOCK NS_BASE_STREAM_WOULD_BLOCK















IPC_METHOD IPC_Init();








IPC_METHOD IPC_Shutdown();


























IPC_METHOD IPC_DefineTarget(
  const nsID          &aTarget,
  ipcIMessageObserver *aObserver,
  PRBool               aOnCurrentThread = PR_TRUE
);





IPC_METHOD IPC_DisableMessageObserver(
  const nsID          &aTarget
);





IPC_METHOD IPC_EnableMessageObserver(
  const nsID          &aTarget
);









IPC_METHOD IPC_SendMessage(
  PRUint32             aReceiverID,
  const nsID          &aTarget,
  const PRUint8       *aData,
  PRUint32             aDataLen
);






































IPC_METHOD IPC_WaitMessage(
  PRUint32             aSenderID,
  const nsID          &aTarget,
  ipcIMessageObserver *aObserver = nsnull,
  PRIntervalTime       aTimeout = PR_INTERVAL_NO_TIMEOUT
);






IPC_METHOD IPC_GetID(
  PRUint32 *aClientID
);






IPC_METHOD IPC_AddName(
  const char *aName
);




IPC_METHOD IPC_RemoveName(
  const char *aName
);




IPC_METHOD IPC_AddClientObserver(
  ipcIClientObserver *aObserver
);




IPC_METHOD IPC_RemoveClientObserver(
  ipcIClientObserver *aObserver
);





IPC_METHOD IPC_ResolveClientName(
  const char *aName,
  PRUint32   *aClientID
);




IPC_METHOD IPC_ClientExists(
  PRUint32  aClientID,
  PRBool   *aResult
);







class ipcDisableMessageObserverForScope
{
public:
  ipcDisableMessageObserverForScope(const nsID &aTarget)
    : mTarget(aTarget)
  {
    IPC_DisableMessageObserver(mTarget);
  }

  ~ipcDisableMessageObserverForScope()
  {
    IPC_EnableMessageObserver(mTarget);
  }

private:
  const nsID &mTarget;
};

#define IPC_DISABLE_MESSAGE_OBSERVER_FOR_SCOPE(_t) \
  ipcDisableMessageObserverForScope ipc_dmo_for_scope##_t(_t)

#endif 
