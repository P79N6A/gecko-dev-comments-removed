








































































#include <plgetopt.h> 
#include <nspr.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




PRLogModuleInfo *lm;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRIntn  debug = 0;
PRIntn  verbose = 0;
PRUint32  failed_already = 0;



#define JITTER_DEFAULT  100000
#define BASE_PORT 9867

PRIntervalTime timeout;
PRNetAddr   listenAddr;
PRFileDesc  *listenSock;
PRLock      *ml;
PRCondVar   *cv;
volatile enum  {
    RunJitter,
    RunAcceptRead,
    AllDone
} state = RunAcceptRead;
PRFileDesc  *file1;
PRIntn  iCounter = 0;
PRIntn  jitter = JITTER_DEFAULT;
PRBool  resume = PR_FALSE;




static void Help( void )
{
    printf("Template: Help(): display your help message(s) here");
    exit(1);
} 





#define ACCEPT_READ_DATASIZE 10
#define ACCEPT_READ_BUFSIZE (PR_ACCEPT_READ_BUF_OVERHEAD + ACCEPT_READ_DATASIZE)

static void AcceptThread(void *arg)
{
    PRIntn bytesRead;
    char dataBuf[ACCEPT_READ_BUFSIZE];
    PRFileDesc  *arSock;
    PRNetAddr   *arAddr;

    bytesRead = PR_AcceptRead( listenSock, 
        &arSock,
        &arAddr,
        dataBuf,
        ACCEPT_READ_DATASIZE,
        PR_SecondsToInterval(1));

    if ( bytesRead == -1 && PR_GetError() == PR_IO_TIMEOUT_ERROR ) {
        if ( debug ) printf("AcceptRead timed out\n");
    } else {
        if ( debug ) printf("Oops! read: %d, error: %d\n", bytesRead, PR_GetError());
    }

    while( state != AllDone )  {
        PR_Lock( ml );
        while( state != RunAcceptRead )
            PR_WaitCondVar( cv, PR_INTERVAL_NO_TIMEOUT );
        if ( ++iCounter >= jitter )
            state = AllDone;
        else
            state = RunJitter;
        if ( verbose ) printf(".");
        PR_NotifyCondVar( cv );
        PR_Unlock( ml );
        PR_Write( file1, ".", 1 );
    }

    return;
} 

static void JitterThread(void *arg)
{
    while( state != AllDone )  {
        PR_Lock( ml );
        while( state != RunJitter && state != AllDone )
            PR_WaitCondVar( cv, PR_INTERVAL_NO_TIMEOUT );
        if ( state != AllDone)
            state = RunAcceptRead;
        if ( verbose ) printf("+");
        PR_NotifyCondVar( cv );
        PR_Unlock( ml );
        PR_Write( file1, "+", 1 );
    }
    return;
} 

static void ConnectThread( void *arg )
{
    PRStatus    rv;
    PRFileDesc  *clientSock;
    PRNetAddr   serverAddress;
    clientSock = PR_NewTCPSocket();

    PR_ASSERT(clientSock);

    if ( resume ) {
        if ( debug ) printf("pausing 3 seconds before connect\n");
        PR_Sleep( PR_SecondsToInterval(3));
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    rv = PR_InitializeNetAddr(PR_IpAddrLoopback, BASE_PORT, &serverAddress);
    PR_ASSERT( PR_SUCCESS == rv );
    rv = PR_Connect( clientSock, 
        &serverAddress, 
        PR_SecondsToInterval(1));
    PR_ASSERT( PR_SUCCESS == rv );

    
    while( state != AllDone )
        PR_Sleep( PR_SecondsToInterval(1));
    return;
} 


int main(int argc, char **argv)
{
    PRThread *tJitter;
    PRThread *tAccept;
    PRThread *tConnect;
    PRStatus rv;
    

#if !defined(WINNT)
    return 0;
#endif

    {
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "hdrvj:");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  
                debug = 1;
			    msgLevel = PR_LOG_ERROR;
                break;
            case 'v':  
                verbose = 1;
			    msgLevel = PR_LOG_DEBUG;
                break;
            case 'j':
                jitter = atoi(opt->value);
                if ( jitter == 0)
                    jitter = JITTER_DEFAULT;
                break;
            case 'r':
                resume = PR_TRUE;
                break;
            case 'h':  
			    Help();
                break;
             default:
                break;
            }
        }
	    PL_DestroyOptState(opt);
    }

    lm = PR_NewLogModule("Test");       

    
    PR_SetConcurrency( 4 );

    
    ml = PR_NewLock();
    cv = PR_NewCondVar( ml );

    
    memset(&listenAddr, 0, sizeof(listenAddr));
    rv = PR_InitializeNetAddr(PR_IpAddrAny, BASE_PORT, &listenAddr);
    PR_ASSERT( PR_SUCCESS == rv );

    listenSock = PR_NewTCPSocket();
    PR_ASSERT( listenSock );

    rv = PR_Bind( listenSock, &listenAddr);
    PR_ASSERT( PR_SUCCESS == rv );

    rv = PR_Listen( listenSock, 5 );
    PR_ASSERT( PR_SUCCESS == rv );

    
    file1 = PR_Open("xxxTestFile", PR_CREATE_FILE | PR_RDWR, 666);

    
    tConnect = PR_CreateThread(
        PR_USER_THREAD, ConnectThread, NULL,
        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
        PR_JOINABLE_THREAD, 0 );
    PR_ASSERT( tConnect );

    
    tJitter = PR_CreateThread(
        PR_USER_THREAD, JitterThread, NULL,
        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
        PR_JOINABLE_THREAD, 0 );
    PR_ASSERT( tJitter );

    
    tAccept = PR_CreateThread(
        PR_USER_THREAD, AcceptThread, NULL,
        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
        PR_JOINABLE_THREAD, 0 );
    PR_ASSERT( tAccept );

    
    PR_JoinThread( tConnect );
    PR_JoinThread( tAccept );
    PR_JoinThread( tJitter );
    PR_Close( listenSock );
    PR_DestroyCondVar(cv);
    PR_DestroyLock(ml);
    PR_Close( file1 );
    PR_Delete( "xxxTestFile");

    
    if (debug) printf("%s\n", (failed_already)? "FAIL" : "PASS");
    return( (failed_already == PR_TRUE )? 1 : 0 );
}  

