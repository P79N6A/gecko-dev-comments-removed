



































#include "ipcd.h"
#include "ipcdPrivate.h"
#include "ipcLog.h"

#include "prerror.h"










ipcClient *ipcClients = NULL;
int        ipcClientCount = 0;

PRStatus
IPC_PlatformSendMsg(ipcClient  *client, ipcMessage *msg)
{
    const char notimplemented[] = "IPC_PlatformSendMsg not implemented";
    PR_SetErrorText(sizeof(notimplemented), notimplemented);
    return PR_FAILURE;
}

int main(int argc, char **argv)
{
    IPC_InitLog("###");

    LOG(("daemon started...\n"));

    




    
    IPC_NotifyParent();
    return 0;
}
