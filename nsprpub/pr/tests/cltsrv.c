



























#include "prclist.h"
#include "prcvar.h"
#include "prerror.h"
#include "prinit.h"
#include "prinrval.h"
#include "prio.h"
#include "prlock.h"
#include "prlog.h"
#include "prtime.h"
#include "prmem.h"
#include "prnetdb.h"
#include "prprf.h"
#include "prthread.h"

#include "pprio.h"
#include "primpl.h"

#include "plstr.h"
#include "plerror.h"
#include "plgetopt.h"

#include <stdlib.h>
#include <string.h>

#if defined(XP_UNIX)
#include <math.h>
#endif





#define RECV_FLAGS 0
#define SEND_FLAGS 0
#define DEFAULT_LOW 0
#define DEFAULT_HIGH 0
#define BUFFER_SIZE 1024
#define DEFAULT_BACKLOG 5
#define DEFAULT_PORT 12849
#define DEFAULT_CLIENTS 1
#define ALLOWED_IN_ACCEPT 1
#define DEFAULT_CLIPPING 1000
#define DEFAULT_WORKERS_MIN 1
#define DEFAULT_WORKERS_MAX 1
#define DEFAULT_SERVER "localhost"
#define DEFAULT_EXECUTION_TIME 10
#define DEFAULT_CLIENT_TIMEOUT 4000
#define DEFAULT_SERVER_TIMEOUT 4000
#define DEFAULT_SERVER_PRIORITY PR_PRIORITY_HIGH

typedef enum CSState_e {cs_init, cs_run, cs_stop, cs_exit} CSState_t;

static void PR_CALLBACK Worker(void *arg);
typedef struct CSPool_s CSPool_t;
typedef struct CSWorker_s CSWorker_t;
typedef struct CSServer_s CSServer_t;
typedef enum Verbosity
{
    TEST_LOG_ALWAYS,
    TEST_LOG_ERROR,
    TEST_LOG_WARNING,
    TEST_LOG_NOTICE,
    TEST_LOG_INFO,
    TEST_LOG_STATUS,
    TEST_LOG_VERBOSE
} Verbosity;

static PRInt32 domain = AF_INET;
static PRInt32 protocol = 6;  
static PRFileDesc *debug_out = NULL;
static PRBool debug_mode = PR_FALSE;
static PRBool pthread_stats = PR_FALSE;
static Verbosity verbosity = TEST_LOG_ALWAYS;
static PRThreadScope thread_scope = PR_LOCAL_THREAD;

struct CSWorker_s
{
    PRCList element;        

    PRThread *thread;       
    CSServer_t *server;     
};

struct CSPool_s
{
    PRCondVar *exiting;
    PRCondVar *acceptComplete;
    PRUint32 accepting, active, workers;
};

struct CSServer_s
{
    PRCList list;           

    PRLock *ml;
    PRThread *thread;       
    PRCondVar *stateChange;

    PRUint16 port;          
    PRUint32 backlog;       
    PRFileDesc *listener;   

    CSPool_t pool;          
    CSState_t state;        
    struct                  
    {
        PRUint32 minimum, maximum, accepting;
    } workers;

    
    PRIntervalTime started, stopped;
    PRUint32 operations, bytesTransferred;
};

typedef struct CSDescriptor_s
{
    PRInt32 size;       
    char filename[60];  
} CSDescriptor_t;

typedef struct CSClient_s
{
    PRLock *ml;
    PRThread *thread;
    PRCondVar *stateChange;
    PRNetAddr serverAddress;

    CSState_t state;

    
    PRIntervalTime started, stopped;
    PRUint32 operations, bytesTransferred;
} CSClient_t;

#define TEST_LOG(l, p, a) \
    do { \
        if (debug_mode || (p <= verbosity)) printf a; \
    } while (0)

PRLogModuleInfo *cltsrv_log_file = NULL;

#define MY_ASSERT(_expr) \
    ((_expr)?((void)0):_MY_Assert(# _expr,__FILE__,__LINE__))

#define TEST_ASSERT(_expr) \
    ((_expr)?((void)0):_MY_Assert(# _expr,__FILE__,__LINE__))

static void _MY_Assert(const char *s, const char *file, PRIntn ln)
{
    PL_PrintError(NULL);
    PR_Assert(s, file, ln);
}  

static PRBool Aborted(PRStatus rv)
{
    return ((PR_FAILURE == rv) && (PR_PENDING_INTERRUPT_ERROR == PR_GetError())) ?
        PR_TRUE : PR_FALSE;
}

static void TimeOfDayMessage(const char *msg, PRThread* me)
{
    char buffer[100];
    PRExplodedTime tod;
    PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &tod);
    (void)PR_FormatTime(buffer, sizeof(buffer), "%H:%M:%S", &tod);

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_ALWAYS,
        ("%s(0x%p): %s\n", msg, me, buffer));
}  


static void PR_CALLBACK Client(void *arg)
{
    PRStatus rv;
    PRIntn index;
    char buffer[1024];
    PRFileDesc *fd = NULL;
    PRUintn clipping = DEFAULT_CLIPPING;
    PRThread *me = PR_GetCurrentThread();
    CSClient_t *client = (CSClient_t*)arg;
    CSDescriptor_t *descriptor = PR_NEW(CSDescriptor_t);
    PRIntervalTime timeout = PR_MillisecondsToInterval(DEFAULT_CLIENT_TIMEOUT);


    for (index = 0; index < sizeof(buffer); ++index)
        buffer[index] = (char)index;

    client->started = PR_IntervalNow();

    PR_Lock(client->ml);
    client->state = cs_run;
    PR_NotifyCondVar(client->stateChange);
    PR_Unlock(client->ml);

    TimeOfDayMessage("Client started at", me);

    while (cs_run == client->state)
    {
        PRInt32 bytes, descbytes, filebytes, netbytes;

        (void)PR_NetAddrToString(&client->serverAddress, buffer, sizeof(buffer));
        TEST_LOG(cltsrv_log_file, TEST_LOG_INFO, 
            ("\tClient(0x%p): connecting to server at %s\n", me, buffer));

        fd = PR_Socket(domain, SOCK_STREAM, protocol);
        TEST_ASSERT(NULL != fd);
        rv = PR_Connect(fd, &client->serverAddress, timeout);
        if (PR_FAILURE == rv)
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_ERROR,
                ("\tClient(0x%p): conection failed (%d, %d)\n",
                me, PR_GetError(), PR_GetOSError()));
            goto aborted;
        }

        memset(descriptor, 0, sizeof(*descriptor));
        descriptor->size = PR_htonl(descbytes = rand() % clipping);
        PR_snprintf(
            descriptor->filename, sizeof(descriptor->filename),
            "CS%p%p-%p.dat", client->started, me, client->operations);
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\tClient(0x%p): sending descriptor for %u bytes\n", me, descbytes));
        bytes = PR_Send(
            fd, descriptor, sizeof(*descriptor), SEND_FLAGS, timeout);
        if (sizeof(CSDescriptor_t) != bytes)
        {
            if (Aborted(PR_FAILURE)) goto aborted;
            if (PR_IO_TIMEOUT_ERROR == PR_GetError())
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\tClient(0x%p): send descriptor timeout\n", me));
                goto retry;
            }
        }
        TEST_ASSERT(sizeof(*descriptor) == bytes);

        netbytes = 0;
        while (netbytes < descbytes)
        {
            filebytes = sizeof(buffer);
            if ((descbytes - netbytes) < filebytes)
                filebytes = descbytes - netbytes;
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_VERBOSE,
                ("\tClient(0x%p): sending %d bytes\n", me, filebytes));
            bytes = PR_Send(fd, buffer, filebytes, SEND_FLAGS, timeout);
            if (filebytes != bytes)
            {
                if (Aborted(PR_FAILURE)) goto aborted;
                if (PR_IO_TIMEOUT_ERROR == PR_GetError())
                {
                    TEST_LOG(
                        cltsrv_log_file, TEST_LOG_ERROR,
                        ("\tClient(0x%p): send data timeout\n", me));
                    goto retry;
                }
            }
            TEST_ASSERT(bytes == filebytes);
            netbytes += bytes;
        }
        filebytes = 0;
        while (filebytes < descbytes)
        {
            netbytes = sizeof(buffer);
            if ((descbytes - filebytes) < netbytes)
                netbytes = descbytes - filebytes;
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_VERBOSE,
                ("\tClient(0x%p): receiving %d bytes\n", me, netbytes));
            bytes = PR_Recv(fd, buffer, netbytes, RECV_FLAGS, timeout);
            if (-1 == bytes)
            {
                if (Aborted(PR_FAILURE))
                {
                    TEST_LOG(
                        cltsrv_log_file, TEST_LOG_ERROR,
                        ("\tClient(0x%p): receive data aborted\n", me));
                    goto aborted;
                }
                else if (PR_IO_TIMEOUT_ERROR == PR_GetError())
                    TEST_LOG(
                        cltsrv_log_file, TEST_LOG_ERROR,
                        ("\tClient(0x%p): receive data timeout\n", me));
				else
                    TEST_LOG(
                        cltsrv_log_file, TEST_LOG_ERROR,
                        ("\tClient(0x%p): receive error (%d, %d)\n",
						me, PR_GetError(), PR_GetOSError()));
                goto retry;
           }
            if (0 == bytes)
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tClient(0x%p): unexpected end of stream\n",
                    PR_GetCurrentThread()));
                break;
            }
            filebytes += bytes;
        }

        rv = PR_Shutdown(fd, PR_SHUTDOWN_BOTH);
        if (Aborted(rv)) goto aborted;
        TEST_ASSERT(PR_SUCCESS == rv);
retry:
        (void)PR_Close(fd); fd = NULL;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_INFO,
            ("\tClient(0x%p): disconnected from server\n", me));

        PR_Lock(client->ml);
        client->operations += 1;
        client->bytesTransferred += 2 * descbytes;
        rv = PR_WaitCondVar(client->stateChange, rand() % clipping);
        PR_Unlock(client->ml);
        if (Aborted(rv)) break;
    }

aborted:
    client->stopped = PR_IntervalNow();

    PR_ClearInterrupt();
    if (NULL != fd) rv = PR_Close(fd);

    PR_Lock(client->ml);
    client->state = cs_exit;
    PR_NotifyCondVar(client->stateChange);
    PR_Unlock(client->ml);
    PR_DELETE(descriptor);
    TEST_LOG(
        cltsrv_log_file, TEST_LOG_ALWAYS,
        ("\tClient(0x%p): stopped after %u operations and %u bytes\n",
        PR_GetCurrentThread(), client->operations, client->bytesTransferred));

}  

static PRStatus ProcessRequest(PRFileDesc *fd, CSServer_t *server)
{
    PRStatus drv, rv;
    char buffer[1024];
    PRFileDesc *file = NULL;
    PRThread * me = PR_GetCurrentThread();
    PRInt32 bytes, descbytes, netbytes, filebytes = 0;
    CSDescriptor_t *descriptor = PR_NEW(CSDescriptor_t);
    PRIntervalTime timeout = PR_MillisecondsToInterval(DEFAULT_SERVER_TIMEOUT);

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_VERBOSE,
        ("\tProcessRequest(0x%p): receiving desciptor\n", me));
    bytes = PR_Recv(
        fd, descriptor, sizeof(*descriptor), RECV_FLAGS, timeout);
    if (-1 == bytes)
    {
        rv = PR_FAILURE;
        if (Aborted(rv)) goto exit;
        if (PR_IO_TIMEOUT_ERROR == PR_GetError())
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_ERROR,
                ("\tProcessRequest(0x%p): receive timeout\n", me));
        }
        goto exit;
    }
    if (0 == bytes)
    {
        rv = PR_FAILURE;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_ERROR,
            ("\tProcessRequest(0x%p): unexpected end of file\n", me));
        goto exit;
    }
    descbytes = PR_ntohl(descriptor->size);
    TEST_ASSERT(sizeof(*descriptor) == bytes);

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_VERBOSE, 
        ("\t\tProcessRequest(0x%p): read descriptor {%d, %s}\n",
        me, descbytes, descriptor->filename));

    file = PR_Open(
        descriptor->filename, (PR_CREATE_FILE | PR_WRONLY), 0666);
    if (NULL == file)
    {
        rv = PR_FAILURE;
        if (Aborted(rv)) goto aborted;
        if (PR_IO_TIMEOUT_ERROR == PR_GetError())
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_ERROR,
                ("\tProcessRequest(0x%p): open file timeout\n", me));
            goto aborted;
        }
    }
    TEST_ASSERT(NULL != file);

    filebytes = 0;
    while (filebytes < descbytes)
    {
        netbytes = sizeof(buffer);
        if ((descbytes - filebytes) < netbytes)
            netbytes = descbytes - filebytes;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\tProcessRequest(0x%p): receive %d bytes\n", me, netbytes));
        bytes = PR_Recv(fd, buffer, netbytes, RECV_FLAGS, timeout);
        if (-1 == bytes)
        {
            rv = PR_FAILURE;
            if (Aborted(rv)) goto aborted;
            if (PR_IO_TIMEOUT_ERROR == PR_GetError())
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tProcessRequest(0x%p): receive data timeout\n", me));
                goto aborted;
            }
            




            TEST_LOG(
                cltsrv_log_file, TEST_LOG_WARNING,
                ("\t\tProcessRequest(0x%p): unexpected error (%d, %d)\n",
                me, PR_GetError(), PR_GetOSError()));
            goto aborted;
        }
        if(0 == bytes)
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_WARNING,
                ("\t\tProcessRequest(0x%p): unexpected end of stream\n", me));
            rv = PR_FAILURE;
            goto aborted;
        }
        filebytes += bytes;
        netbytes = bytes;
        
        MY_ASSERT(netbytes > 0);
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\tProcessRequest(0x%p): write %d bytes to file\n", me, netbytes));
        bytes = PR_Write(file, buffer, netbytes);
        if (netbytes != bytes)
        {
            rv = PR_FAILURE;
            if (Aborted(rv)) goto aborted;
            if (PR_IO_TIMEOUT_ERROR == PR_GetError())
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tProcessRequest(0x%p): write file timeout\n", me));
                goto aborted;
            }
        }
        TEST_ASSERT(bytes > 0);
    }

    PR_Lock(server->ml);
    server->operations += 1;
    server->bytesTransferred += filebytes;
    PR_Unlock(server->ml);

    rv = PR_Close(file);
    if (Aborted(rv)) goto aborted;
    TEST_ASSERT(PR_SUCCESS == rv);
    file = NULL;

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_VERBOSE,
        ("\t\tProcessRequest(0x%p): opening %s\n", me, descriptor->filename));
    file = PR_Open(descriptor->filename, PR_RDONLY, 0);
    if (NULL == file)
    {
        rv = PR_FAILURE;
        if (Aborted(rv)) goto aborted;
        if (PR_IO_TIMEOUT_ERROR == PR_GetError())
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_ERROR,
                ("\t\tProcessRequest(0x%p): open file timeout\n",
                PR_GetCurrentThread()));
            goto aborted;
        }
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_ERROR,
            ("\t\tProcessRequest(0x%p): other file open error (%u, %u)\n",
            me, PR_GetError(), PR_GetOSError()));
        goto aborted;
    }
    TEST_ASSERT(NULL != file);

    netbytes = 0;
    while (netbytes < descbytes)
    {
        filebytes = sizeof(buffer);
        if ((descbytes - netbytes) < filebytes)
            filebytes = descbytes - netbytes;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\tProcessRequest(0x%p): read %d bytes from file\n", me, filebytes));
        bytes = PR_Read(file, buffer, filebytes);
        if (filebytes != bytes)
        {
            rv = PR_FAILURE;
            if (Aborted(rv)) goto aborted;
            if (PR_IO_TIMEOUT_ERROR == PR_GetError())
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tProcessRequest(0x%p): read file timeout\n", me));
            else
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tProcessRequest(0x%p): other file error (%d, %d)\n",
                    me, PR_GetError(), PR_GetOSError()));
            goto aborted;
        }
        TEST_ASSERT(bytes > 0);
        netbytes += bytes;
        filebytes = bytes;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\t\tProcessRequest(0x%p): sending %d bytes\n", me, filebytes));
        bytes = PR_Send(fd, buffer, filebytes, SEND_FLAGS, timeout);
        if (filebytes != bytes)
        {
            rv = PR_FAILURE;
            if (Aborted(rv)) goto aborted;
            if (PR_IO_TIMEOUT_ERROR == PR_GetError())
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tProcessRequest(0x%p): send data timeout\n", me));
                goto aborted;
            }
            break;
        }
       TEST_ASSERT(bytes > 0);
    }
    
    PR_Lock(server->ml);
    server->bytesTransferred += filebytes;
    PR_Unlock(server->ml);

    rv = PR_Shutdown(fd, PR_SHUTDOWN_BOTH);
    if (Aborted(rv)) goto aborted;

    rv = PR_Close(file);
    if (Aborted(rv)) goto aborted;
    TEST_ASSERT(PR_SUCCESS == rv);
    file = NULL;

aborted:
    PR_ClearInterrupt();
    if (NULL != file) PR_Close(file);
    drv = PR_Delete(descriptor->filename);
    TEST_ASSERT(PR_SUCCESS == drv);
exit:
    TEST_LOG(
        cltsrv_log_file, TEST_LOG_VERBOSE,
        ("\t\tProcessRequest(0x%p): Finished\n", me));

    PR_DELETE(descriptor);

#if defined(WIN95)
    PR_Sleep(PR_MillisecondsToInterval(200)); 
#endif
    return rv;
}  

static PRStatus CreateWorker(CSServer_t *server, CSPool_t *pool)
{
    CSWorker_t *worker = PR_NEWZAP(CSWorker_t);
    worker->server = server;
    PR_INIT_CLIST(&worker->element);
    worker->thread = PR_CreateThread(
        PR_USER_THREAD, Worker, worker,
        DEFAULT_SERVER_PRIORITY, thread_scope,
        PR_UNJOINABLE_THREAD, 0);
    if (NULL == worker->thread)
    {
        PR_DELETE(worker);
        return PR_FAILURE;
    }

    TEST_LOG(cltsrv_log_file, TEST_LOG_STATUS, 
        ("\tCreateWorker(0x%p): create new worker (0x%p)\n",
        PR_GetCurrentThread(), worker->thread));

    return PR_SUCCESS;
}  

static void PR_CALLBACK Worker(void *arg)
{
    PRStatus rv;
    PRNetAddr from;
    PRFileDesc *fd = NULL;
    PRThread *me = PR_GetCurrentThread();
    CSWorker_t *worker = (CSWorker_t*)arg;
    CSServer_t *server = worker->server;
    CSPool_t *pool = &server->pool;

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_NOTICE,
        ("\t\tWorker(0x%p): started [%u]\n", me, pool->workers + 1));

    PR_Lock(server->ml);
    PR_APPEND_LINK(&worker->element, &server->list);
    pool->workers += 1;  

    while (cs_run == server->state)
    {
        while (pool->accepting >= server->workers.accepting)
        {
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_VERBOSE,
                ("\t\tWorker(0x%p): waiting for accept slot[%d]\n",
                me, pool->accepting));
            rv = PR_WaitCondVar(pool->acceptComplete, PR_INTERVAL_NO_TIMEOUT);
            if (Aborted(rv) || (cs_run != server->state))
            {
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_NOTICE,
                    ("\tWorker(0x%p): has been %s\n",
                    me, (Aborted(rv) ? "interrupted" : "stopped")));
                goto exit;
            }
        } 
        pool->accepting += 1;  
        PR_Unlock(server->ml);

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\t\tWorker(0x%p): calling accept\n", me));
        fd = PR_Accept(server->listener, &from, PR_INTERVAL_NO_TIMEOUT);

        PR_Lock(server->ml);        
        pool->accepting -= 1;
        PR_NotifyCondVar(pool->acceptComplete);

        if ((NULL == fd) && Aborted(PR_FAILURE))
        {
            if (NULL != server->listener)
            {
                PR_Close(server->listener);
                server->listener = NULL;
            }
            goto exit;
        }

        if (NULL != fd)
        {
            







            PRBool another =
                ((pool->workers < server->workers.minimum) ||
                ((0 == pool->accepting)
                    && (pool->workers < server->workers.maximum))) ?
                    PR_TRUE : PR_FALSE;
            pool->active += 1;
            PR_Unlock(server->ml);

            if (another) (void)CreateWorker(server, pool);

            rv = ProcessRequest(fd, server);
            if (PR_SUCCESS != rv)
                TEST_LOG(
                    cltsrv_log_file, TEST_LOG_ERROR,
                    ("\t\tWorker(0x%p): server process ended abnormally\n", me));
            (void)PR_Close(fd); fd = NULL;

            PR_Lock(server->ml);
            pool->active -= 1;
        }
    }

exit:
    PR_ClearInterrupt();    
    PR_Unlock(server->ml);

    if (NULL != fd)
    {
        (void)PR_Shutdown(fd, PR_SHUTDOWN_BOTH);
        (void)PR_Close(fd);
    }

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_NOTICE,
        ("\t\tWorker(0x%p): exiting [%u]\n", PR_GetCurrentThread(), pool->workers));

    PR_Lock(server->ml);
    pool->workers -= 1;  
    PR_REMOVE_AND_INIT_LINK(&worker->element);
    PR_NotifyCondVar(pool->exiting);
    PR_Unlock(server->ml);

    PR_DELETE(worker);  

}  

static void PR_CALLBACK Server(void *arg)
{
    PRStatus rv;
    PRNetAddr serverAddress;
    PRThread *me = PR_GetCurrentThread();
    CSServer_t *server = (CSServer_t*)arg;
    PRSocketOptionData sockOpt;

    server->listener = PR_Socket(domain, SOCK_STREAM, protocol);

    sockOpt.option = PR_SockOpt_Reuseaddr;
    sockOpt.value.reuse_addr = PR_TRUE;
    rv = PR_SetSocketOption(server->listener, &sockOpt);
    TEST_ASSERT(PR_SUCCESS == rv);

    memset(&serverAddress, 0, sizeof(serverAddress));
	if (PR_AF_INET6 != domain)
		rv = PR_InitializeNetAddr(PR_IpAddrAny, DEFAULT_PORT, &serverAddress);
	else
		rv = PR_SetNetAddr(PR_IpAddrAny, PR_AF_INET6, DEFAULT_PORT,
													&serverAddress);
    rv = PR_Bind(server->listener, &serverAddress);
    TEST_ASSERT(PR_SUCCESS == rv);

    rv = PR_Listen(server->listener, server->backlog);
    TEST_ASSERT(PR_SUCCESS == rv);

    server->started = PR_IntervalNow();
    TimeOfDayMessage("Server started at", me);

    PR_Lock(server->ml);
    server->state = cs_run;
    PR_NotifyCondVar(server->stateChange);
    PR_Unlock(server->ml);

    





    rv = CreateWorker(server, &server->pool);
    TEST_ASSERT(PR_SUCCESS == rv);

    




    TEST_LOG(
        cltsrv_log_file, TEST_LOG_VERBOSE,
        ("\tServer(0x%p): waiting for state change\n", me));

    PR_Lock(server->ml);
    while ((cs_run == server->state) && !Aborted(rv))
    {
        rv = PR_WaitCondVar(server->stateChange, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(server->ml);
    PR_ClearInterrupt();

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_INFO,
        ("\tServer(0x%p): shutting down workers\n", me));

    





    server->workers.minimum = server->workers.maximum = 0;

    PR_Lock(server->ml);
    while (!PR_CLIST_IS_EMPTY(&server->list))
    {
        PRCList *head = PR_LIST_HEAD(&server->list);
        CSWorker_t *worker = (CSWorker_t*)head;
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("\tServer(0x%p): interrupting worker(0x%p)\n", me, worker));
        rv = PR_Interrupt(worker->thread);
        TEST_ASSERT(PR_SUCCESS == rv);
        PR_REMOVE_AND_INIT_LINK(head);
    }

    while (server->pool.workers > 0)
    {
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_NOTICE,
            ("\tServer(0x%p): waiting for %u workers to exit\n",
            me, server->pool.workers));
        (void)PR_WaitCondVar(server->pool.exiting, PR_INTERVAL_NO_TIMEOUT);
    }

    server->state = cs_exit;
    PR_NotifyCondVar(server->stateChange);
    PR_Unlock(server->ml);

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_ALWAYS,
        ("\tServer(0x%p): stopped after %u operations and %u bytes\n",
        me, server->operations, server->bytesTransferred));

    if (NULL != server->listener) PR_Close(server->listener);
    server->stopped = PR_IntervalNow();

}  

static void WaitForCompletion(PRIntn execution)
{
    while (execution > 0)
    { 
        PRIntn dally = (execution > 30) ? 30 : execution;
        PR_Sleep(PR_SecondsToInterval(dally));
        if (pthread_stats) PT_FPrintStats(debug_out, "\nPThread Statistics\n");
        execution -= dally;
    }
}  

static void Help(void)
{
    PR_fprintf(debug_out, "cltsrv test program usage:\n");
    PR_fprintf(debug_out, "\t-a <n>       threads allowed in accept        (5)\n");
    PR_fprintf(debug_out, "\t-b <n>       backlock for listen              (5)\n");
    PR_fprintf(debug_out, "\t-c <threads> number of clients to create      (1)\n");
    PR_fprintf(debug_out, "\t-f <low>     low water mark for fd caching    (0)\n");
    PR_fprintf(debug_out, "\t-F <high>    high water mark for fd caching   (0)\n");
    PR_fprintf(debug_out, "\t-w <threads> minimal number of server threads (1)\n");
    PR_fprintf(debug_out, "\t-W <threads> maximum number of server threads (1)\n");
    PR_fprintf(debug_out, "\t-e <seconds> duration of the test in seconds  (10)\n");
    PR_fprintf(debug_out, "\t-s <string>  dsn name of server               (localhost)\n");
    PR_fprintf(debug_out, "\t-G           use GLOBAL threads               (LOCAL)\n");
    PR_fprintf(debug_out, "\t-X           use XTP as transport             (TCP)\n");
    PR_fprintf(debug_out, "\t-6           Use IPv6                         (IPv4)\n");
    PR_fprintf(debug_out, "\t-v           verbosity (accumulative)         (0)\n");
    PR_fprintf(debug_out, "\t-p           pthread statistics               (FALSE)\n");
    PR_fprintf(debug_out, "\t-d           debug mode                       (FALSE)\n");
    PR_fprintf(debug_out, "\t-h           this message\n");
}  

static Verbosity IncrementVerbosity(void)
{
    PRIntn verboge = (PRIntn)verbosity + 1;
    return (Verbosity)verboge;
}  

int main(int argc, char** argv)
{
    PRUintn index;
    PRBool boolean;
    CSClient_t *client;
    PRStatus rv, joinStatus;
    CSServer_t *server = NULL;

    PRUintn backlog = DEFAULT_BACKLOG;
    PRUintn clients = DEFAULT_CLIENTS;
    const char *serverName = DEFAULT_SERVER;
    PRBool serverIsLocal = PR_TRUE;
    PRUintn accepting = ALLOWED_IN_ACCEPT;
    PRUintn workersMin = DEFAULT_WORKERS_MIN;
    PRUintn workersMax = DEFAULT_WORKERS_MAX;
    PRIntn execution = DEFAULT_EXECUTION_TIME;
    PRIntn low = DEFAULT_LOW, high = DEFAULT_HIGH;

    













    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "GX6b:a:c:f:F:w:W:e:s:vdhp");

    debug_out = PR_GetSpecialFD(PR_StandardError);

    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'G':  
            thread_scope = PR_GLOBAL_THREAD;
            break;
        case 'X':  
            protocol = 36;
            break;
        case '6':  
            domain = PR_AF_INET6;
            break;
        case 'a':  
            accepting = atoi(opt->value);
            break;
        case 'b':  
            backlog = atoi(opt->value);
            break;
        case 'c':  
            clients = atoi(opt->value);
            break;
        case 'f':  
            low = atoi(opt->value);
            break;
        case 'F':  
            high = atoi(opt->value);
            break;
        case 'w':  
            workersMin = atoi(opt->value);
            break;
        case 'W':  
            workersMax = atoi(opt->value);
            break;
        case 'e':  
            execution = atoi(opt->value);
            break;
        case 's':  
            serverName = opt->value;
            break;
        case 'v':  
            verbosity = IncrementVerbosity();
            break;
        case 'd':  
            debug_mode = PR_TRUE;
            break;
        case 'p':  
            pthread_stats = PR_TRUE;
            break;
        case 'h':
        default:
            Help();
            return 2;
        }
    }
    PL_DestroyOptState(opt);

    if (0 != PL_strcmp(serverName, DEFAULT_SERVER)) serverIsLocal = PR_FALSE;
    if (0 == execution) execution = DEFAULT_EXECUTION_TIME;
    if (0 == workersMax) workersMax = DEFAULT_WORKERS_MAX;
    if (0 == workersMin) workersMin = DEFAULT_WORKERS_MIN;
    if (0 == accepting) accepting = ALLOWED_IN_ACCEPT;
    if (0 == backlog) backlog = DEFAULT_BACKLOG;

    if (workersMin > accepting) accepting = workersMin;

    PR_STDIO_INIT();
    TimeOfDayMessage("Client/Server started at", PR_GetCurrentThread());

    cltsrv_log_file = PR_NewLogModule("cltsrv_log");
    MY_ASSERT(NULL != cltsrv_log_file);
    boolean = PR_SetLogFile("cltsrv.log");
    MY_ASSERT(boolean);

    rv = PR_SetFDCacheSize(low, high);
    PR_ASSERT(PR_SUCCESS == rv);

    if (serverIsLocal)
    {
        
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_INFO,
            ("main(0x%p): starting server\n", PR_GetCurrentThread()));

        server = PR_NEWZAP(CSServer_t);
        PR_INIT_CLIST(&server->list);
        server->state = cs_init;
        server->ml = PR_NewLock();
        server->backlog = backlog;
        server->port = DEFAULT_PORT;
        server->workers.minimum = workersMin;
        server->workers.maximum = workersMax;
        server->workers.accepting = accepting;
        server->stateChange = PR_NewCondVar(server->ml);
        server->pool.exiting = PR_NewCondVar(server->ml);
        server->pool.acceptComplete = PR_NewCondVar(server->ml);

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_NOTICE,
            ("main(0x%p): creating server thread\n", PR_GetCurrentThread()));

        server->thread = PR_CreateThread(
            PR_USER_THREAD, Server, server, PR_PRIORITY_HIGH,
            thread_scope, PR_JOINABLE_THREAD, 0);
        TEST_ASSERT(NULL != server->thread);

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("main(0x%p): waiting for server init\n", PR_GetCurrentThread()));

        PR_Lock(server->ml);
        while (server->state == cs_init)
            PR_WaitCondVar(server->stateChange, PR_INTERVAL_NO_TIMEOUT);
        PR_Unlock(server->ml);

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("main(0x%p): server init complete (port #%d)\n",
            PR_GetCurrentThread(), server->port));
    }

    if (clients != 0)
    {
        
        PRHostEnt host;
        char buffer[BUFFER_SIZE];
        client = (CSClient_t*)PR_CALLOC(clients * sizeof(CSClient_t));

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_VERBOSE,
            ("main(0x%p): creating %d client threads\n",
            PR_GetCurrentThread(), clients));
        
        if (!serverIsLocal)
        {
            rv = PR_GetHostByName(serverName, buffer, BUFFER_SIZE, &host);
            if (PR_SUCCESS != rv)
            {
                PL_FPrintError(PR_STDERR, "PR_GetHostByName");
                return 2;
            }
        }

        for (index = 0; index < clients; ++index)
        {
            client[index].state = cs_init;
            client[index].ml = PR_NewLock();
            if (serverIsLocal)
            {
				if (PR_AF_INET6 != domain)
                	(void)PR_InitializeNetAddr(
                    	PR_IpAddrLoopback, DEFAULT_PORT,
                    	&client[index].serverAddress);
				else
					rv = PR_SetNetAddr(PR_IpAddrLoopback, PR_AF_INET6,
							DEFAULT_PORT, &client[index].serverAddress);
            }
            else
            {
                (void)PR_EnumerateHostEnt(
                    0, &host, DEFAULT_PORT, &client[index].serverAddress);
            }
            client[index].stateChange = PR_NewCondVar(client[index].ml);
            TEST_LOG(
                cltsrv_log_file, TEST_LOG_INFO,
                ("main(0x%p): creating client threads\n", PR_GetCurrentThread()));
            client[index].thread = PR_CreateThread(
                PR_USER_THREAD, Client, &client[index], PR_PRIORITY_NORMAL,
                thread_scope, PR_JOINABLE_THREAD, 0);
            TEST_ASSERT(NULL != client[index].thread);
            PR_Lock(client[index].ml);
            while (cs_init == client[index].state)
                PR_WaitCondVar(client[index].stateChange, PR_INTERVAL_NO_TIMEOUT);
            PR_Unlock(client[index].ml);
        }
    }

    
    TEST_LOG(
        cltsrv_log_file, TEST_LOG_ALWAYS,
        ("main(0x%p): waiting for execution interval (%d seconds)\n",
        PR_GetCurrentThread(), execution));

    WaitForCompletion(execution);

    TimeOfDayMessage("Shutting down", PR_GetCurrentThread());

    if (clients != 0)
    {
        for (index = 0; index < clients; ++index)
        {
            TEST_LOG(cltsrv_log_file, TEST_LOG_STATUS, 
                ("main(0x%p): notifying client(0x%p) to stop\n",
                PR_GetCurrentThread(), client[index].thread));

            PR_Lock(client[index].ml);
            if (cs_run == client[index].state)
            {
                client[index].state = cs_stop;
                PR_Interrupt(client[index].thread);
                while (cs_stop == client[index].state)
                    PR_WaitCondVar(
                        client[index].stateChange, PR_INTERVAL_NO_TIMEOUT);
            }
            PR_Unlock(client[index].ml);

            TEST_LOG(cltsrv_log_file, TEST_LOG_VERBOSE, 
                ("main(0x%p): joining client(0x%p)\n",
                PR_GetCurrentThread(), client[index].thread));

		    joinStatus = PR_JoinThread(client[index].thread);
		    TEST_ASSERT(PR_SUCCESS == joinStatus);
            PR_DestroyCondVar(client[index].stateChange);
            PR_DestroyLock(client[index].ml);
        }
        PR_DELETE(client);
    }

    if (NULL != server)
    {
        
        TEST_LOG(
            cltsrv_log_file, TEST_LOG_NOTICE, 
            ("main(0x%p): notifying server(0x%p) to stop\n",
            PR_GetCurrentThread(), server->thread));

        PR_Lock(server->ml);
        server->state = cs_stop;
        PR_Interrupt(server->thread);
        while (cs_exit != server->state)
            PR_WaitCondVar(server->stateChange, PR_INTERVAL_NO_TIMEOUT);
        PR_Unlock(server->ml);

        TEST_LOG(
            cltsrv_log_file, TEST_LOG_NOTICE, 
            ("main(0x%p): joining server(0x%p)\n",
            PR_GetCurrentThread(), server->thread));
        joinStatus = PR_JoinThread(server->thread);
        TEST_ASSERT(PR_SUCCESS == joinStatus);

        PR_DestroyCondVar(server->stateChange);
        PR_DestroyCondVar(server->pool.exiting);
        PR_DestroyCondVar(server->pool.acceptComplete);
        PR_DestroyLock(server->ml);
        PR_DELETE(server);
    }

    TEST_LOG(
        cltsrv_log_file, TEST_LOG_ALWAYS, 
        ("main(0x%p): test complete\n", PR_GetCurrentThread()));

    PT_FPrintStats(debug_out, "\nPThread Statistics\n");

    TimeOfDayMessage("Test exiting at", PR_GetCurrentThread());
    PR_Cleanup();
    return 0;
}  


