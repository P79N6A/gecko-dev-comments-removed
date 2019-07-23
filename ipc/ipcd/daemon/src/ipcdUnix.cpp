




































#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prio.h"
#include "prerror.h"
#include "prthread.h"
#include "prinrval.h"
#include "plstr.h"
#include "prprf.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessage.h"
#include "ipcClient.h"
#include "ipcModuleReg.h"
#include "ipcdPrivate.h"
#include "ipcd.h"

#if 0
static void
IPC_Sleep(int seconds)
{
    while (seconds > 0) {
        LOG(("\rsleeping for %d seconds...", seconds));
        PR_Sleep(PR_SecondsToInterval(1));
        --seconds;
    }
    LOG(("\ndone sleeping\n"));
}
#endif











#if !defined(XP_OS2)
#define IPC_USE_FILE_LOCK
#endif

#ifdef IPC_USE_FILE_LOCK

static int ipcLockFD = 0;

static PRBool AcquireDaemonLock(const char *baseDir)
{
    const char lockName[] = "lock";

    int dirLen = strlen(baseDir);
    int len = dirLen            
            + 1                 
            + sizeof(lockName); 

    char *lockFile = (char *) malloc(len);
    memcpy(lockFile, baseDir, dirLen);
    lockFile[dirLen] = '/';
    memcpy(lockFile + dirLen + 1, lockName, sizeof(lockName));

    
    
    
    ipcLockFD = open(lockFile, O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR);

    free(lockFile);

    if (ipcLockFD == -1)
        return PR_FALSE;

    
    
    
    
    
    
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;
    if (fcntl(ipcLockFD, F_SETLK, &lock) == -1)
        return PR_FALSE;

    
    
    
    ftruncate(ipcLockFD, 0);

    
    
    
    
    char buf[32];
    int nb = PR_snprintf(buf, sizeof(buf), "%u\n", (unsigned long) getpid());
    write(ipcLockFD, buf, nb);

    return PR_TRUE;
}

static PRBool InitDaemonDir(const char *socketPath)
{
    LOG(("InitDaemonDir [sock=%s]\n", socketPath));

    char *baseDir = PL_strdup(socketPath);

    
    
    
    char *p = strrchr(baseDir, '/');
    if (p)
        p[0] = '\0';
    mkdir(baseDir, 0700);

    
    
    
    
    PRBool haveLock = AcquireDaemonLock(baseDir);

    PL_strfree(baseDir);

    if (haveLock) {
        
        unlink(socketPath);
    }
    return haveLock;
}

static void ShutdownDaemonDir()
{
    LOG(("ShutdownDaemonDir\n"));

    
    
    
    

    
    if (ipcLockFD) {
        close(ipcLockFD);
        ipcLockFD = 0;
    }
}

#endif 








ipcClient *ipcClients = NULL;
int        ipcClientCount = 0;






static ipcClient ipcClientArray[IPC_MAX_CLIENTS + 1];




static PRPollDesc ipcPollList[IPC_MAX_CLIENTS + 1];



static int AddClient(PRFileDesc *fd)
{
    if (ipcClientCount == IPC_MAX_CLIENTS) {
        LOG(("reached maximum client limit\n"));
        return -1;
    }

    int pollCount = ipcClientCount + 1;

    ipcClientArray[pollCount].Init();

    ipcPollList[pollCount].fd = fd;
    ipcPollList[pollCount].in_flags = PR_POLL_READ;
    ipcPollList[pollCount].out_flags = 0;

    ++ipcClientCount;
    return 0;
}

static int RemoveClient(int clientIndex)
{
    PRPollDesc *pd = &ipcPollList[clientIndex];

    PR_Close(pd->fd);

    ipcClientArray[clientIndex].Finalize();

    
    
    
    
    int toIndex = clientIndex;
    int fromIndex = ipcClientCount;
    if (fromIndex != toIndex) {
        memcpy(&ipcClientArray[toIndex], &ipcClientArray[fromIndex], sizeof(ipcClient));
        memcpy(&ipcPollList[toIndex], &ipcPollList[fromIndex], sizeof(PRPollDesc));
    }

    
    
    
    memset(&ipcClientArray[fromIndex], 0, sizeof(ipcClient));
    memset(&ipcPollList[fromIndex], 0, sizeof(PRPollDesc));

    --ipcClientCount;
    return 0;
}



static void PollLoop(PRFileDesc *listenFD)
{
    
    memset(ipcClientArray, 0, sizeof(ipcClientArray));
    ipcClients = ipcClientArray + 1;
    ipcClientCount = 0;

    ipcPollList[0].fd = listenFD;
    ipcPollList[0].in_flags = PR_POLL_EXCEPT | PR_POLL_READ;
    
    while (1) {
        PRInt32 rv;
        PRIntn i;

        int pollCount = ipcClientCount + 1;

        ipcPollList[0].out_flags = 0;

        
        
        
        
        
        
        
        
        
        LOG(("calling PR_Poll [pollCount=%d]\n", pollCount));
        rv = PR_Poll(ipcPollList, pollCount, PR_SecondsToInterval(60 * 5));
        if (rv == -1) {
            LOG(("PR_Poll failed [%d]\n", PR_GetError()));
            return;
        }

        if (rv > 0) {
            
            
            
            for (i = 1; i < pollCount; ++i) {
                if (ipcPollList[i].out_flags != 0) {
                    ipcPollList[i].in_flags =
                        ipcClientArray[i].Process(ipcPollList[i].fd,
                                                  ipcPollList[i].out_flags);
                    ipcPollList[i].out_flags = 0;
                }
            }

            
            
            
            for (i = pollCount - 1; i >= 1; --i) {
                if (ipcPollList[i].in_flags == 0)
                    RemoveClient(i);
            }

            
            
            
            if (ipcPollList[0].out_flags & PR_POLL_READ) {
                LOG(("got new connection\n"));

                PRNetAddr clientAddr;
                memset(&clientAddr, 0, sizeof(clientAddr));
                PRFileDesc *clientFD;

                clientFD = PR_Accept(listenFD, &clientAddr, PR_INTERVAL_NO_WAIT);
                if (clientFD == NULL) {
                    
                    LOG(("PR_Accept failed [%d]\n", PR_GetError()));
                }
                else {
                    
                    PRSocketOptionData opt;
                    opt.option = PR_SockOpt_Nonblocking;
                    opt.value.non_blocking = PR_TRUE;
                    PR_SetSocketOption(clientFD, &opt);

                    if (AddClient(clientFD) != 0)
                        PR_Close(clientFD);
                }
            }
        }

        
        
        
        if (ipcClientCount == 0) {
            LOG(("shutting down\n"));
            break;
        }
    }
}



PRStatus
IPC_PlatformSendMsg(ipcClient  *client, ipcMessage *msg)
{
    LOG(("IPC_PlatformSendMsg\n"));

    
    
    
    client->EnqueueOutboundMsg(msg);

    
    
    
    
    int clientIndex = client - ipcClientArray;
    ipcPollList[clientIndex].in_flags |= PR_POLL_WRITE;

    return PR_SUCCESS;
}



int main(int argc, char **argv)
{
    PRFileDesc *listenFD = NULL;
    PRNetAddr addr;

    
    
    
    
    signal(SIGINT, SIG_IGN);
    

    
    umask(0077);

    IPC_InitLog("###");

    LOG(("daemon started...\n"));

    
    

    
    addr.local.family = PR_AF_LOCAL;
    if (argc < 2)
        IPC_GetDefaultSocketPath(addr.local.path, sizeof(addr.local.path));
    else
        PL_strncpyz(addr.local.path, argv[1], sizeof(addr.local.path));

#ifdef IPC_USE_FILE_LOCK
    if (!InitDaemonDir(addr.local.path)) {
        LOG(("InitDaemonDir failed\n"));
        goto end;
    }
#endif

    listenFD = PR_OpenTCPSocket(PR_AF_LOCAL);
    if (!listenFD) {
        LOG(("PR_OpenTCPSocket failed [%d]\n", PR_GetError()));
        goto end;
    }

    if (PR_Bind(listenFD, &addr) != PR_SUCCESS) {
        LOG(("PR_Bind failed [%d]\n", PR_GetError()));
        goto end;
    }

    IPC_InitModuleReg(argv[0]);

    if (PR_Listen(listenFD, 5) != PR_SUCCESS) {
        LOG(("PR_Listen failed [%d]\n", PR_GetError()));
        goto end;
    }

    IPC_NotifyParent();

    PollLoop(listenFD);

end:
    IPC_ShutdownModuleReg();

    IPC_NotifyParent();

    

#ifdef IPC_USE_FILE_LOCK
    
    
    
 
    ShutdownDaemonDir();
#endif

    if (listenFD) {
        LOG(("closing socket\n"));
        PR_Close(listenFD);
    }
    return 0;
}
