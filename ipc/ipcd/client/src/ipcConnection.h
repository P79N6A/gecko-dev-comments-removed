




































#ifndef ipcConnection_h__
#define ipcConnection_h__

#include "nscore.h"

class ipcMessage;

#define IPC_METHOD_PRIVATE_(type)   NS_HIDDEN_(type)
#define IPC_METHOD_PRIVATE          IPC_METHOD_PRIVATE_(nsresult)





typedef void (* ipcCallbackFunc)(void *);












IPC_METHOD_PRIVATE IPC_Connect(const char *daemonPath);










IPC_METHOD_PRIVATE IPC_Disconnect();













IPC_METHOD_PRIVATE IPC_SendMsg(ipcMessage *msg);













IPC_METHOD_PRIVATE IPC_DoCallback(ipcCallbackFunc func, void *arg);















IPC_METHOD_PRIVATE IPC_SpawnDaemon(const char *daemonPath);















IPC_METHOD_PRIVATE_(void) IPC_OnConnectionEnd(nsresult error);








IPC_METHOD_PRIVATE_(void) IPC_OnMessageAvailable(ipcMessage *msg);

#endif 
