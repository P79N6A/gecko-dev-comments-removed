




































#include "prlog.h"
#include "prio.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessage.h"
#include "ipcClient.h"
#include "ipcModuleReg.h"
#include "ipcModule.h"
#include "ipcCommandModule.h"
#include "ipcdPrivate.h"
#include "ipcd.h"



void
IPC_NotifyParent()
{
    PRFileDesc *fd = PR_GetInheritedFD(IPC_STARTUP_PIPE_NAME);
    if (fd) {
        char c = IPC_STARTUP_PIPE_MAGIC;
        PR_Write(fd, &c, 1);
        PR_Close(fd);
    }
}



PRStatus
IPC_DispatchMsg(ipcClient *client, const ipcMessage *msg)
{
    PR_ASSERT(client);
    PR_ASSERT(msg);

    
    
    if (msg->TestFlag(IPC_MSG_FLAG_SYNC_QUERY)) {
        PR_ASSERT(client->GetExpectsSyncReply() == PR_FALSE);
        
        
        client->SetExpectsSyncReply(PR_TRUE);
    }

    if (msg->Target().Equals(IPCM_TARGET)) {
        IPCM_HandleMsg(client, msg);
        return PR_SUCCESS;
    }

    return IPC_DispatchMsg(client, msg->Target(), msg->Data(), msg->DataLen());
}

PRStatus
IPC_SendMsg(ipcClient *client, ipcMessage *msg)
{
    PR_ASSERT(msg);

    if (client == NULL) {
        
        
        
        for (int i=0; i<ipcClientCount; ++i)
            IPC_SendMsg(&ipcClients[i], msg->Clone());
        delete msg;
        return PR_SUCCESS;
    }

    
    if (client->GetExpectsSyncReply()) {
        msg->SetFlag(IPC_MSG_FLAG_SYNC_REPLY);
        client->SetExpectsSyncReply(PR_FALSE);
    }

    if (client->HasTarget(msg->Target()))
        return IPC_PlatformSendMsg(client, msg);

    LOG(("  no registered message handler\n"));
    return PR_FAILURE;
}

void
IPC_NotifyClientUp(ipcClient *client)
{
    
    IPC_NotifyModulesClientUp(client);

    for (int i=0; i<ipcClientCount; ++i) {
        if (&ipcClients[i] != client)
            IPC_SendMsg(&ipcClients[i],
                new ipcmMessageClientState(client->ID(), IPCM_CLIENT_STATE_UP));
    }
}

void
IPC_NotifyClientDown(ipcClient *client)
{
    
    IPC_NotifyModulesClientDown(client);

    for (int i=0; i<ipcClientCount; ++i) {
        if (&ipcClients[i] != client)
            IPC_SendMsg(&ipcClients[i],
                new ipcmMessageClientState(client->ID(), IPCM_CLIENT_STATE_DOWN));
    }
}





PRStatus
IPC_SendMsg(ipcClient *client, const nsID &target, const void *data, PRUint32 dataLen)
{
    return IPC_SendMsg(client, new ipcMessage(target, (const char *) data, dataLen));
}

ipcClient *
IPC_GetClientByID(PRUint32 clientID)
{
    
    for (int i = 0; i < ipcClientCount; ++i) {
        if (ipcClients[i].ID() == clientID)
            return &ipcClients[i];
    }
    return NULL;
}

ipcClient *
IPC_GetClientByName(const char *name)
{
    
    for (int i = 0; i < ipcClientCount; ++i) {
        if (ipcClients[i].HasName(name))
            return &ipcClients[i];
    }
    return NULL;
}

void
IPC_EnumClients(ipcClientEnumFunc func, void *closure)
{
    PR_ASSERT(func);
    for (int i = 0; i < ipcClientCount; ++i) {
        if (func(closure, &ipcClients[i], ipcClients[i].ID()) == PR_FALSE)
            break;
    }
}

PRUint32
IPC_GetClientID(ipcClient *client)
{
    PR_ASSERT(client);
    return client->ID();
}

PRBool
IPC_ClientHasName(ipcClient *client, const char *name)
{
    PR_ASSERT(client);
    PR_ASSERT(name);
    return client->HasName(name);
}

PRBool
IPC_ClientHasTarget(ipcClient *client, const nsID &target)
{
    PR_ASSERT(client);
    return client->HasTarget(target);
}

void
IPC_EnumClientNames(ipcClient *client, ipcClientNameEnumFunc func, void *closure)
{
    PR_ASSERT(client);
    PR_ASSERT(func);
    const ipcStringNode *node = client->Names();
    while (node) {
        if (func(closure, client, node->Value()) == PR_FALSE)
            break;
        node = node->mNext;
    }
}

void
IPC_EnumClientTargets(ipcClient *client, ipcClientTargetEnumFunc func, void *closure)
{
    PR_ASSERT(client);
    PR_ASSERT(func);
    const ipcIDNode *node = client->Targets();
    while (node) {
        if (func(closure, client, node->Value()) == PR_FALSE)
            break;
        node = node->mNext;
    }
}
