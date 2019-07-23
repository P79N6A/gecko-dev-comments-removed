




































#ifndef ipcdPrivate_h__
#define ipcdPrivate_h__

class ipcClient;





#define IPC_MAX_CLIENTS 100




extern ipcClient *ipcClients;
extern int        ipcClientCount;




PRStatus IPC_PlatformSendMsg(ipcClient  *client, ipcMessage *msg);




void IPC_NotifyParent();

#endif 
