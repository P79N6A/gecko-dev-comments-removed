




































#ifndef IPCD_H__
#define IPCD_H__

#include "ipcModule.h"
#include "ipcMessage.h"








PRStatus        IPC_DispatchMsg          (ipcClientHandle client, const nsID &target, const void *data, PRUint32 dataLen);
PRStatus        IPC_SendMsg              (ipcClientHandle client, const nsID &target, const void *data, PRUint32 dataLen);
ipcClientHandle IPC_GetClientByID        (PRUint32 id);
ipcClientHandle IPC_GetClientByName      (const char *name);
void            IPC_EnumClients          (ipcClientEnumFunc func, void *closure);
PRUint32        IPC_GetClientID          (ipcClientHandle client);
PRBool          IPC_ClientHasName        (ipcClientHandle client, const char *name);
PRBool          IPC_ClientHasTarget      (ipcClientHandle client, const nsID &target);
void            IPC_EnumClientNames      (ipcClientHandle client, ipcClientNameEnumFunc func, void *closure);
void            IPC_EnumClientTargets    (ipcClientHandle client, ipcClientTargetEnumFunc func, void *closure);








PRStatus IPC_DispatchMsg(ipcClientHandle client, const ipcMessage *msg);




PRStatus IPC_SendMsg(ipcClientHandle client, ipcMessage *msg);




void IPC_NotifyClientUp(ipcClientHandle client);
void IPC_NotifyClientDown(ipcClientHandle client);

#endif 
