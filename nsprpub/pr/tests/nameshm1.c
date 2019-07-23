



































































































#include <plgetopt.h> 
#include <nspr.h>
#include <stdlib.h>
#include <string.h>
#include <private/primpl.h>

#ifdef SYMBIAN
#define SEM_NAME1 "c:\\data\\nameshmSEM1"
#define SEM_NAME2 "c:\\data\\nameshmSEM2"
#define OPT_NAME "c:\\data\\xxxNSPRshm"
#define EXE_NAME "nspr_tests_nameshm1.exe"
#else
#define SEM_NAME1 "/tmp/nameshmSEM1"
#define SEM_NAME2 "/tmp/nameshmSEM2"
#define OPT_NAME "/tmp/xxxNSPRshm"
#define EXE_NAME "nameshm1"
#endif
#define SEM_MODE  0666
#define SHM_MODE  0666

#define NameSize (1024)

PRIntn  debug = 0;
PRIntn  failed_already = 0;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRLogModuleInfo *lm;


PRIntn      optDebug = 0;
PRIntn      optVerbose = 0;
PRUint32    optWriteRO = 0;     
PRUint32    optClient = 0;
PRUint32    optCreate = 1;
PRUint32    optAttachRW = 1;
PRUint32    optAttachRO = 1;
PRUint32    optClose = 1;
PRUint32    optDelete = 1;
PRInt32     optPing = 1000;
PRUint32    optSize = (10 * 1024 );
PRInt32     optClientIterations = 3;
char        optName[NameSize] = OPT_NAME;

char buf[1024] = "";


static void BasicTest( void ) 
{
    PRSharedMemory  *shm;
    char *addr; 
    PRUint32  i;
    PRInt32 rc;

    PR_LOG( lm, msgLevel,
             ( "nameshm1: Begin BasicTest" ));

    if ( PR_FAILURE == PR_DeleteSharedMemory( optName )) {
        PR_LOG( lm, msgLevel,
            ("nameshm1: Initial PR_DeleteSharedMemory() failed. No problem"));
    } else
        PR_LOG( lm, msgLevel,
            ("nameshm1: Initial PR_DeleteSharedMemory() success"));


    shm = PR_OpenSharedMemory( optName, optSize, (PR_SHM_CREATE | PR_SHM_EXCL), SHM_MODE );
    if ( NULL == shm )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RW Create: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RW Create: success: %p", shm ));

    addr = PR_AttachSharedMemory( shm , 0 );
    if ( NULL == addr ) 
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RW Attach: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RW Attach: success: %p", addr ));

    
    for ( i = 0; i < optSize ;  i++ )
    {
         *(addr + i) = i;
    }

    rc = PR_DetachSharedMemory( shm, addr );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RW Detach: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RW Detach: success: " ));

    rc = PR_CloseSharedMemory( shm );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RW Close: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RW Close: success: " ));

    rc = PR_DeleteSharedMemory( optName );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RW Delete: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RW Delete: success: " ));

    PR_LOG( lm, msgLevel,
            ("nameshm1: BasicTest(): Passed"));

    return;
} 

static void ReadOnlyTest( void )
{
    PRSharedMemory  *shm;
    char *roAddr; 
    PRInt32 rc;

    PR_LOG( lm, msgLevel,
             ( "nameshm1: Begin ReadOnlyTest" ));

    shm = PR_OpenSharedMemory( optName, optSize, (PR_SHM_CREATE | PR_SHM_EXCL), SHM_MODE);
    if ( NULL == shm )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RO Create: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RO Create: success: %p", shm ));


    roAddr = PR_AttachSharedMemory( shm , PR_SHM_READONLY );
    if ( NULL == roAddr ) 
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RO Attach: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RO Attach: success: %p", roAddr ));

    if ( optWriteRO )
    {
        *roAddr = 0x00; 
        failed_already = 1;
        PR_LOG( lm, msgLevel, ("nameshm1: Wrote to read-only memory segment!"));
        return;
    }

    rc = PR_DetachSharedMemory( shm, roAddr );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RO Detach: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RO Detach: success: " ));

    rc = PR_CloseSharedMemory( shm );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RO Close: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RO Close: success: " ));

    rc = PR_DeleteSharedMemory( optName );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: RO Destroy: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: RO Destroy: success: " ));

    PR_LOG( lm, msgLevel,
        ("nameshm1: ReadOnlyTest(): Passed"));

    return;
} 

static void DoClient( void )
{
    PRStatus rc;
    PRSem *sem1, *sem2;
    PRSharedMemory  *shm;
    PRUint32 *addr; 
    PRInt32 i;

    PR_LOG( lm, msgLevel,
            ("nameshm1: DoClient(): Starting"));

    sem1 = PR_OpenSemaphore( SEM_NAME1, 0, 0, 0 );
    PR_ASSERT( sem1 );

    sem2 = PR_OpenSemaphore( SEM_NAME2, 0, 0, 0 );
    PR_ASSERT( sem1 );

    shm = PR_OpenSharedMemory( optName, optSize, 0, SHM_MODE );
    if ( NULL == shm )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: DoClient(): Create: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: DoClient(): Create: success: %p", shm ));

    addr = PR_AttachSharedMemory( shm , 0 );
    if ( NULL == addr ) 
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: DoClient(): Attach: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: DoClient(): Attach: success: %p", addr ));

    PR_LOG( lm, msgLevel,
        ( "Client found: %s", addr));

    PR_Sleep(PR_SecondsToInterval(4));
    for ( i = 0 ; i < optPing ; i++ )
    {
        rc = PR_WaitSemaphore( sem2 );
        PR_ASSERT( PR_FAILURE != rc );
        
        (*addr)++;
        PR_ASSERT( (*addr % 2) == 0 );        
        if ( optVerbose )
            PR_LOG( lm, msgLevel,
                 ( "nameshm1: Client ping: %d, i: %d", *addr, i));

        rc = PR_PostSemaphore( sem1 );
        PR_ASSERT( PR_FAILURE != rc );
    }

    rc = PR_CloseSemaphore( sem1 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_CloseSemaphore( sem2 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_DetachSharedMemory( shm, addr );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: DoClient(): Detach: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: DoClient(): Detach: success: " ));

    rc = PR_CloseSharedMemory( shm );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: DoClient(): Close: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: DoClient(): Close: success: " ));

    return;
}    

static void ClientServerTest( void )
{
    PRStatus rc;
    PRSem *sem1, *sem2;
    PRProcess *proc;
    PRInt32 exit_status;
    PRSharedMemory  *shm;
    PRUint32 *addr; 
    PRInt32 i;
    char *child_argv[8];
    char buf[24];

    PR_LOG( lm, msgLevel,
             ( "nameshm1: Begin ClientServerTest" ));

    rc = PR_DeleteSharedMemory( optName );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: Server: Destroy: failed. No problem"));
    } else
        PR_LOG( lm, msgLevel,
            ( "nameshm1: Server: Destroy: success" ));


    shm = PR_OpenSharedMemory( optName, optSize, (PR_SHM_CREATE | PR_SHM_EXCL), SHM_MODE);
    if ( NULL == shm )
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: Server: Create: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: Server: Create: success: %p", shm ));

    addr = PR_AttachSharedMemory( shm , 0 );
    if ( NULL == addr ) 
    {
        PR_LOG( lm, msgLevel,
                 ( "nameshm1: Server: Attach: Error: %ld. OSError: %ld", PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: Server: Attach: success: %p", addr ));

    sem1 = PR_OpenSemaphore( SEM_NAME1, PR_SEM_CREATE, SEM_MODE, 0 );
    PR_ASSERT( sem1 );

    sem2 = PR_OpenSemaphore( SEM_NAME2, PR_SEM_CREATE, SEM_MODE, 1 );
    PR_ASSERT( sem1 );

    strcpy( (char*)addr, "FooBar" );

    child_argv[0] = EXE_NAME;
    child_argv[1] = "-C";
    child_argv[2] = "-p";
    sprintf( buf, "%d", optPing );
    child_argv[3] = buf;
    child_argv[4] = optName;
    child_argv[5] = NULL;

    proc = PR_CreateProcess(child_argv[0], child_argv, NULL, NULL);
    PR_ASSERT( proc );

    PR_Sleep( PR_SecondsToInterval(4));

    *addr = 1;
    for ( i = 0 ; i < optPing ; i++ )
    { 
        rc = PR_WaitSemaphore( sem1 );
        PR_ASSERT( PR_FAILURE != rc );

        (*addr)++;
        PR_ASSERT( (*addr % 2) == 1 );
        if ( optVerbose )
            PR_LOG( lm, msgLevel,
                 ( "nameshm1: Server pong: %d, i: %d", *addr, i));

    
        rc = PR_PostSemaphore( sem2 );
        PR_ASSERT( PR_FAILURE != rc );
    }

    rc = PR_WaitProcess( proc, &exit_status );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_CloseSemaphore( sem1 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_CloseSemaphore( sem2 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_DeleteSemaphore( SEM_NAME1 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_DeleteSemaphore( SEM_NAME2 );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_DetachSharedMemory( shm, addr );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: Server: Detach: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
             ( "nameshm1: Server: Detach: success: " ));

    rc = PR_CloseSharedMemory( shm );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: Server: Close: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
        ( "nameshm1: Server: Close: success: " ));

    rc = PR_DeleteSharedMemory( optName );
    if ( PR_FAILURE == rc )
    {
        PR_LOG( lm, msgLevel,
            ( "nameshm1: Server: Destroy: Error: %ld. OSError: %ld", 
                PR_GetError(), PR_GetOSError()));
        failed_already = 1;
        return;
    }
    PR_LOG( lm, msgLevel,
        ( "nameshm1: Server: Destroy: success" ));

    return;
} 

int main(int argc, char **argv)
{
    {
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "Cdvw:s:p:i:");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'v':  
                optVerbose = 1;
                
            case 'd':  
                debug = 1;
			    msgLevel = PR_LOG_DEBUG;
                break;
            case 'w':  
                optWriteRO = 1;
                break;
            case 'C':
                optClient = 1;
                break;
            case 's':
                optSize = atol(opt->value) * 1024;
                break;
            case 'p':
                optPing = atol(opt->value);
                break;
            case 'i':
                optClientIterations = atol(opt->value);
                break;
            default:
                strcpy( optName, opt->value );
                break;
            }
        }
	    PL_DestroyOptState(opt);
    }

    lm = PR_NewLogModule("Test");       
    
    PR_LOG( lm, msgLevel,
             ( "nameshm1: Starting" ));

    if ( optClient )
    {
        DoClient();
    } else {
        BasicTest();
        if ( failed_already != 0 )
            goto Finished;
        ReadOnlyTest();
        if ( failed_already != 0 )
            goto Finished;
        ClientServerTest();
    }

Finished:
    if ( debug ) printf("%s\n", (failed_already)? "FAIL" : "PASS" );
    return( (failed_already)? 1 : 0 );
}  

