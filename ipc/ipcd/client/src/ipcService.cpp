




































#include "ipcService.h"




NS_IMPL_THREADSAFE_ISUPPORTS1(ipcService, ipcIService)

NS_IMETHODIMP
ipcService::GetID(PRUint32 *aID)
{
    return IPC_GetID(aID);
}

NS_IMETHODIMP
ipcService::AddName(const char *aName)
{
    return IPC_AddName(aName);
}

NS_IMETHODIMP
ipcService::RemoveName(const char *aName)
{
    return IPC_RemoveName(aName);
}

NS_IMETHODIMP
ipcService::AddClientObserver(ipcIClientObserver *aObserver)
{
    return IPC_AddClientObserver(aObserver);
}

NS_IMETHODIMP
ipcService::RemoveClientObserver(ipcIClientObserver *aObserver)
{
    return IPC_RemoveClientObserver(aObserver);
}

NS_IMETHODIMP
ipcService::ResolveClientName(const char *aName, PRUint32 *aID)
{
    return IPC_ResolveClientName(aName, aID);
}

NS_IMETHODIMP
ipcService::ClientExists(PRUint32 aClientID, PRBool *aResult)
{
    return IPC_ClientExists(aClientID, aResult);
}

NS_IMETHODIMP
ipcService::DefineTarget(const nsID &aTarget, ipcIMessageObserver *aObserver,
                         PRBool aOnCurrentThread)
{
    return IPC_DefineTarget(aTarget, aObserver, aOnCurrentThread);
}

NS_IMETHODIMP
ipcService::SendMessage(PRUint32 aReceiverID, const nsID &aTarget,
                        const PRUint8 *aData, PRUint32 aDataLen)
{
    return IPC_SendMessage(aReceiverID, aTarget, aData, aDataLen);
}

NS_IMETHODIMP
ipcService::WaitMessage(PRUint32 aSenderID, const nsID &aTarget,
                        ipcIMessageObserver *aObserver,
                        PRUint32 aTimeout)
{
    return IPC_WaitMessage(aSenderID, aTarget, aObserver,
                           PR_MillisecondsToInterval(aTimeout));
}

NS_IMETHODIMP
ipcService::DisableMessageObserver(const nsID &aTarget)
{
    return IPC_DisableMessageObserver(aTarget);
}

NS_IMETHODIMP
ipcService::EnableMessageObserver(const nsID &aTarget)
{
    return IPC_EnableMessageObserver(aTarget);
}
