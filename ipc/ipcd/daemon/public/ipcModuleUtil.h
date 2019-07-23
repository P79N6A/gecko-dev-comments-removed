




































#ifndef ipcModuleUtil_h__
#define ipcModuleUtil_h__

#include "prlog.h"
#include "ipcModule.h"

extern const ipcDaemonMethods *gIPCDaemonMethods;








inline PRStatus
IPC_DispatchMsg(ipcClientHandle client, const nsID &target, const void *data, PRUint32 dataLen)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->dispatchMsg(client, target, data, dataLen);
}

inline PRStatus
IPC_SendMsg(ipcClientHandle client, const nsID &target, const void *data, PRUint32 dataLen)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->sendMsg(client, target, data, dataLen);
}

inline ipcClientHandle
IPC_GetClientByID(PRUint32 id)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->getClientByID(id);
}

inline ipcClientHandle
IPC_GetClientByName(const char *name)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->getClientByName(name);
}

inline void
IPC_EnumClients(ipcClientEnumFunc func, void *closure)
{
    PR_ASSERT(gIPCDaemonMethods);
    gIPCDaemonMethods->enumClients(func, closure);
}

inline PRUint32
IPC_GetClientID(ipcClientHandle client)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->getClientID(client);
}

inline PRBool
IPC_ClientHasName(ipcClientHandle client, const char *name)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->clientHasName(client, name);
}

inline PRBool
IPC_ClientHasTarget(ipcClientHandle client, const nsID &target)
{
    PR_ASSERT(gIPCDaemonMethods);
    return gIPCDaemonMethods->clientHasTarget(client, target);
}

inline void
IPC_EnumClientNames(ipcClientHandle client, ipcClientNameEnumFunc func, void *closure)
{
    PR_ASSERT(gIPCDaemonMethods);
    gIPCDaemonMethods->enumClientNames(client, func, closure);
}

inline void
IPC_EnumClientTargets(ipcClientHandle client, ipcClientTargetEnumFunc func, void *closure)
{
    PR_ASSERT(gIPCDaemonMethods);
    gIPCDaemonMethods->enumClientTargets(client, func, closure);
}





inline PRStatus
IPC_SendMsg(PRUint32 clientID, const nsID &target, const void *data, PRUint32 dataLen)
{
    ipcClient *client = IPC_GetClientByID(clientID);
    if (!client)
        return PR_FAILURE;
    return IPC_SendMsg(client, target, data, dataLen);
}





#define IPC_IMPL_GETMODULES(_modName, _modEntries)                  \
    const ipcDaemonMethods *gIPCDaemonMethods;                      \
    IPC_EXPORT int                                                  \
    IPC_GetModules(const ipcDaemonMethods *dmeths,                  \
                   const ipcModuleEntry **ents) {                   \
        /* XXX do version checking */                               \
        gIPCDaemonMethods = dmeths;                                 \
        *ents = _modEntries;                                        \
        return sizeof(_modEntries) / sizeof(ipcModuleEntry);        \
    }

#endif 
