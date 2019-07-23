





































#include <stdlib.h>
#include "nsDependentString.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "ipcLockService.h"
#include "ipcLockProtocol.h"
#include "ipcLog.h"
#include "prthread.h"

static const nsID kLockTargetID = IPC_LOCK_TARGETID;



struct ipcPendingLock
{
    const char *name;
    nsresult    status;
    PRBool      complete;
};



nsresult
ipcLockService::Init()
{
    if (PR_NewThreadPrivateIndex(&mTPIndex, nsnull) != PR_SUCCESS)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    

    return IPC_DefineTarget(kLockTargetID, this, PR_FALSE);
}

NS_IMPL_THREADSAFE_ISUPPORTS2(ipcLockService, ipcILockService, ipcIMessageObserver)

NS_IMETHODIMP
ipcLockService::AcquireLock(const char *lockName, PRBool waitIfBusy)
{
    LOG(("ipcLockService::AcquireLock [lock=%s wait=%u]\n", lockName, waitIfBusy));

    ipcLockMsg msg;
    msg.opcode = IPC_LOCK_OP_ACQUIRE;
    msg.flags = (waitIfBusy ? 0 : IPC_LOCK_FL_NONBLOCKING);
    msg.key = lockName;

    PRUint32 bufLen;
    nsAutoPtr<PRUint8> buf( IPC_FlattenLockMsg(&msg, &bufLen) );
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    ipcPendingLock pendingLock;
    pendingLock.name = lockName;
    pendingLock.status = 0xDEADBEEF; 
    pendingLock.complete = PR_FALSE;
    if (PR_SetThreadPrivate(mTPIndex, &pendingLock) != PR_SUCCESS)
        return NS_ERROR_UNEXPECTED;

    
    
    IPC_DISABLE_MESSAGE_OBSERVER_FOR_SCOPE(kLockTargetID);

    nsresult rv = IPC_SendMessage(0, kLockTargetID, buf, bufLen);
    if (NS_SUCCEEDED(rv)) {
        do {
            
            rv = IPC_WaitMessage(0, kLockTargetID, this, PR_INTERVAL_NO_TIMEOUT);
        }
        while (NS_SUCCEEDED(rv) && !pendingLock.complete);

        if (NS_SUCCEEDED(rv))
            rv = pendingLock.status;
    }

    

    return rv;
}

NS_IMETHODIMP
ipcLockService::ReleaseLock(const char *lockName)
{
    LOG(("ipcLockService::ReleaseLock [lock=%s]\n", lockName));

    ipcLockMsg msg;
    msg.opcode = IPC_LOCK_OP_RELEASE;
    msg.flags = 0;
    msg.key = lockName;

    PRUint32 bufLen;
    PRUint8 *buf = IPC_FlattenLockMsg(&msg, &bufLen);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = IPC_SendMessage(0, kLockTargetID, buf, bufLen);
    delete buf;

    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}


NS_IMETHODIMP
ipcLockService::OnMessageAvailable(PRUint32 unused, const nsID &target,
                                   const PRUint8 *data, PRUint32 dataLen)
{
    ipcLockMsg msg;
    IPC_UnflattenLockMsg(data, dataLen, &msg);

    LOG(("ipcLockService::OnMessageAvailable [lock=%s opcode=%u]\n", msg.key, msg.opcode));

    ipcPendingLock *pendingLock = (ipcPendingLock *) PR_GetThreadPrivate(mTPIndex);
    if (strcmp(pendingLock->name, msg.key) == 0) {
        pendingLock->complete = PR_TRUE;
        if (msg.opcode == IPC_LOCK_OP_STATUS_ACQUIRED)
            pendingLock->status = NS_OK;
        else
            pendingLock->status = NS_ERROR_FAILURE;
        return NS_OK;
    }

    LOG(("message does not match; waiting for another...\n"));

    
    return IPC_WAIT_NEXT_MESSAGE;
}
