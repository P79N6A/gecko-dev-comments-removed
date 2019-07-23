













































#include <string.h>
#include "primpl.h"


#define DEFAULT_TRACE_BUFSIZE ( 1024 * 1024 )
#define DEFAULT_BUFFER_SEGMENTS    2




typedef enum TraceState
{
    Running = 1,
    Suspended = 2
} TraceState;




typedef struct QName
{
    PRCList link;
    PRCList rNameList;
    char    name[PRTRACE_NAME_MAX+1];
} QName;




typedef struct RName
{
    PRCList link;
    PRLock  *lock;
    QName   *qName;
    TraceState state;
    char    name[PRTRACE_NAME_MAX+1];
    char    desc[PRTRACE_DESC_MAX+1];
} RName;






static PRLogModuleInfo *lm;

static PRLock      *traceLock;      
static PRCList     qNameList;       
static TraceState traceState = Running;




static  PRTraceEntry    *tBuf;      
static  PRInt32         bufSize;    
static  volatile PRInt32  next;     
static  PRInt32         last;       




static PRInt32 fetchLastSeen = 0;
static PRBool  fetchLostData = PR_FALSE;




static  PRLock      *logLock;               
static  PRCondVar   *logCVar;               









static  enum LogState
{
    LogNotRunning,  
    LogReset,       
    LogActive,      
    LogSuspend,      
    LogResume,       
    LogStop         
}   logOrder, logState, localState;         
static  PRInt32     logSegments;            
static  PRInt32     logEntries;             
static  PRInt32     logEntriesPerSegment;   
static  PRInt32     logSegSize;             
static  PRInt32     logCount;               
static  PRInt32     logLostData;            









static void NewTraceBuffer( PRInt32 size )
{
    




    logSegments = DEFAULT_BUFFER_SEGMENTS;
    logEntries = size / sizeof(PRTraceEntry);
    logEntriesPerSegment = logEntries / logSegments;
    logEntries = logSegments * logEntriesPerSegment;
    bufSize = logEntries * sizeof(PRTraceEntry);
    logSegSize = logEntriesPerSegment * sizeof(PRTraceEntry);
    PR_ASSERT( bufSize != 0);
    PR_LOG( lm, PR_LOG_ERROR,
        ("NewTraceBuffer: logSegments: %ld, logEntries: %ld, logEntriesPerSegment: %ld, logSegSize: %ld",
            logSegments, logEntries, logEntriesPerSegment, logSegSize ));


    tBuf = PR_Malloc( bufSize );
    if ( tBuf == NULL )
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("PRTrace: Failed to get trace buffer"));
        PR_ASSERT( 0 );
    } 
    else
    {
        PR_LOG( lm, PR_LOG_NOTICE,
            ("PRTrace: Got trace buffer of size: %ld, at %p", bufSize, tBuf));
    }

    next = 0;
    last = logEntries -1;
    logCount = 0;
    logLostData = PR_TRUE; 
    logOrder = LogReset;

} 




static void _PR_InitializeTrace( void )
{
    
    PR_ASSERT( traceLock == NULL );

    traceLock = PR_NewLock();
    PR_ASSERT( traceLock != NULL );

    PR_Lock( traceLock );
    
    PR_INIT_CLIST( &qNameList );

    lm = PR_NewLogModule("trace");

    bufSize = DEFAULT_TRACE_BUFSIZE;
    NewTraceBuffer( bufSize );

    
    logLock = PR_NewLock();
    logCVar = PR_NewCondVar( logLock );

    PR_Unlock( traceLock );
    return;    
} 




PR_IMPLEMENT(PRTraceHandle)
	PR_CreateTrace( 
    	const char *qName,          
	    const char *rName,          
	    const char *description     
)
{
    QName   *qnp;
    RName   *rnp;
    PRBool  matchQname = PR_FALSE;

    
    if ( traceLock == NULL )
        _PR_InitializeTrace();

    
    PR_ASSERT( strlen(qName) <= PRTRACE_NAME_MAX );
    PR_ASSERT( strlen(rName) <= PRTRACE_NAME_MAX );
    PR_ASSERT( strlen(description) <= PRTRACE_DESC_MAX );

    PR_LOG( lm, PR_LOG_DEBUG,
            ("PRTRACE: CreateTrace: Qname: %s, RName: %s", qName, rName));

    
    PR_Lock( traceLock );

    
    if (!PR_CLIST_IS_EMPTY( &qNameList ))
    {
        qnp = (QName *) PR_LIST_HEAD( &qNameList );
        do {
            if ( strcmp(qnp->name, qName) == 0)
            {
                matchQname = PR_TRUE;
                break;
            }
            qnp = (QName *)PR_NEXT_LINK( &qnp->link );
        } while( qnp != (QName *)PR_LIST_HEAD( &qNameList ));
    }
    





    if ( matchQname != PR_TRUE )
    {
        qnp = PR_NEWZAP( QName );
        PR_ASSERT( qnp != NULL );
        PR_INIT_CLIST( &qnp->link ); 
        PR_INIT_CLIST( &qnp->rNameList ); 
        strcpy( qnp->name, qName );
        PR_APPEND_LINK( &qnp->link, &qNameList ); 
    }

    
    if (!PR_CLIST_IS_EMPTY( &qnp->rNameList ))
    {
        rnp = (RName *) PR_LIST_HEAD( &qnp->rNameList );
        do {
            



            PR_ASSERT( strcmp(rnp->name, rName));
            rnp = (RName *)PR_NEXT_LINK( &rnp->link );
        } while( rnp != (RName *)PR_LIST_HEAD( &qnp->rNameList ));
    }

    
    rnp = PR_NEWZAP( RName );
    PR_ASSERT( rnp != NULL );
    PR_INIT_CLIST( &rnp->link );
    strcpy( rnp->name, rName );
    strcpy( rnp->desc, description );
    rnp->lock = PR_NewLock();
    rnp->state = Running;
    if ( rnp->lock == NULL )
    {
        PR_ASSERT(0);
    }

    PR_APPEND_LINK( &rnp->link, &qnp->rNameList );     
    rnp->qName = qnp;                       

    
    PR_Unlock( traceLock );
    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: Create: QName: %s %p, RName: %s %p\n\t",
        qName, qnp, rName, rnp ));

    return((PRTraceHandle)rnp);
} 




PR_IMPLEMENT(void) 
	PR_DestroyTrace( 
		PRTraceHandle handle    
)
{
    RName   *rnp = (RName *)handle;
    QName   *qnp = rnp->qName;

    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: Deleting: QName: %s, RName: %s", 
        qnp->name, rnp->name));

    
    PR_Lock( traceLock );

    



    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: Deleting RName: %s, %p", 
        rnp->name, rnp));
    PR_REMOVE_LINK( &rnp->link );
    PR_Free( rnp->lock );
    PR_DELETE( rnp );

    



    if ( PR_CLIST_IS_EMPTY( &qnp->rNameList ) )
    {
        PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: Deleting unused QName: %s, %p", 
            qnp->name, qnp));
        PR_REMOVE_LINK( &qnp->link );
        PR_DELETE( qnp );
    } 

    
    PR_Unlock( traceLock );
    return;
} 




PR_IMPLEMENT(void) 
	PR_Trace( 
    	PRTraceHandle handle,       
	    PRUint32    userData0,      
	    PRUint32    userData1,      
	    PRUint32    userData2,      
	    PRUint32    userData3,      
	    PRUint32    userData4,      
	    PRUint32    userData5,      
	    PRUint32    userData6,      
	    PRUint32    userData7       
)
{
    PRTraceEntry   *tep;
    PRInt32         mark;

    if ( (traceState == Suspended ) 
        || ( ((RName *)handle)->state == Suspended )) 
        return;

    


    PR_Lock( traceLock );

    tep = &tBuf[next++]; 
    if ( next > last )
        next = 0;
    if ( fetchLostData == PR_FALSE && next == fetchLastSeen )
        fetchLostData = PR_TRUE;
    
    mark = next;
        
    PR_Unlock( traceLock );

    


    tep->thread = PR_GetCurrentThread();
    tep->handle = handle;
    tep->time   = PR_Now();
    tep->userData[0] = userData0;
    tep->userData[1] = userData1;
    tep->userData[2] = userData2;
    tep->userData[3] = userData3;
    tep->userData[4] = userData4;
    tep->userData[5] = userData5;
    tep->userData[6] = userData6;
    tep->userData[7] = userData7;

    
    if (( mark % logEntriesPerSegment) == 0 )
    {
        PR_Lock( logLock );
        logCount++;
        PR_NotifyCondVar( logCVar );
        PR_Unlock( logLock );
        








        
    }

    return;
} 




PR_IMPLEMENT(void) 
	PR_SetTraceOption( 
	    PRTraceOption command,  
	    void *value             
)
{
    RName * rnp;

    switch ( command )
    {
        case PRTraceBufSize :
            PR_Lock( traceLock );
            PR_Free( tBuf );
            bufSize = *(PRInt32 *)value;
            NewTraceBuffer( bufSize );
            PR_Unlock( traceLock );
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceBufSize: %ld", bufSize));
            break;
        
        case PRTraceEnable :
            rnp = *(RName **)value;
            rnp->state = Running;
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceEnable: %p", rnp));
            break;
        
        case PRTraceDisable :
            rnp = *(RName **)value;
            rnp->state = Suspended;
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceDisable: %p", rnp));
            break;
        
        case PRTraceSuspend :
            traceState = Suspended;
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceSuspend"));
            break;
        
        case PRTraceResume :
            traceState = Running;
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceResume"));
            break;
        
        case PRTraceSuspendRecording :
            PR_Lock( logLock );
            logOrder = LogSuspend;
            PR_NotifyCondVar( logCVar );
            PR_Unlock( logLock );
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceSuspendRecording"));
            break;
        
        case PRTraceResumeRecording :
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceResumeRecording"));
            if ( logState != LogSuspend )
                break;
            PR_Lock( logLock );
            logOrder = LogResume;
            PR_NotifyCondVar( logCVar );
            PR_Unlock( logLock );
            break;
        
        case PRTraceStopRecording :
            PR_Lock( logLock );
            logOrder = LogStop;
            PR_NotifyCondVar( logCVar );
            PR_Unlock( logLock );
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceStopRecording"));
            break;

        case PRTraceLockHandles :
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceLockTraceHandles"));
            PR_Lock( traceLock );
            break;
        
        case PRTraceUnLockHandles :
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRSetTraceOption: PRTraceUnLockHandles"));
            PR_Unlock( traceLock );
            break;

        default:
            PR_LOG( lm, PR_LOG_ERROR,
                ("PRSetTraceOption: Invalid command %ld", command ));
            PR_ASSERT( 0 );
            break;
    } 
    return;
} 




PR_IMPLEMENT(void) 
	PR_GetTraceOption( 
    	PRTraceOption command,  
	    void *value             
)
{
    switch ( command )
    {
        case PRTraceBufSize :
            *((PRInt32 *)value) = bufSize;
            PR_LOG( lm, PR_LOG_DEBUG,
                ("PRGetTraceOption: PRTraceBufSize: %ld", bufSize ));
            break;
        
        default:
            PR_LOG( lm, PR_LOG_ERROR,
                ("PRGetTraceOption: Invalid command %ld", command ));
            PR_ASSERT( 0 );
            break;
    } 
    return;
} 




PR_IMPLEMENT(PRTraceHandle) 
	PR_GetTraceHandleFromName( 
    	const char *qName,      
        const char *rName       
)
{
    const char    *qn, *rn, *desc;
    PRTraceHandle     qh, rh = NULL;
    RName   *rnp = NULL;

    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: GetTraceHandleFromName:\n\t"
        "QName: %s, RName: %s", qName, rName ));

    qh = PR_FindNextTraceQname( NULL );
    while (qh != NULL)
    {
        rh = PR_FindNextTraceRname( NULL, qh );
        while ( rh != NULL )
        {
            PR_GetTraceNameFromHandle( rh, &qn, &rn, &desc );
            if ( (strcmp( qName, qn ) == 0)
                && (strcmp( rName, rn ) == 0 ))
            {
                rnp = (RName *)rh;
                goto foundIt;
            }
            rh = PR_FindNextTraceRname( rh, qh );
        }
        qh = PR_FindNextTraceQname( NULL );
    }

foundIt:
    PR_LOG( lm, PR_LOG_DEBUG, ("PR_Counter: GetConterHandleFromName: %p", rnp ));
    return(rh);
} 




PR_IMPLEMENT(void) 
	PR_GetTraceNameFromHandle( 
    	PRTraceHandle handle,       
	    const char **qName,         
	    const char **rName,         
    	const char **description    
)
{
    RName   *rnp = (RName *)handle;
    QName   *qnp = rnp->qName;

    *qName = qnp->name;
    *rName = rnp->name;
    *description = rnp->desc;

    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: GetConterNameFromHandle: "
        "QNp: %p, RNp: %p,\n\tQName: %s, RName: %s, Desc: %s", 
        qnp, rnp, qnp->name, rnp->name, rnp->desc ));

    return;
} 




PR_IMPLEMENT(PRTraceHandle) 
	PR_FindNextTraceQname( 
        PRTraceHandle handle
)
{
    QName *qnp = (QName *)handle;

    if ( PR_CLIST_IS_EMPTY( &qNameList ))
            qnp = NULL;
    else if ( qnp == NULL )
        qnp = (QName *)PR_LIST_HEAD( &qNameList );
    else if ( PR_NEXT_LINK( &qnp->link ) ==  &qNameList )
        qnp = NULL;
    else  
        qnp = (QName *)PR_NEXT_LINK( &qnp->link );

    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: FindNextQname: Handle: %p, Returns: %p", 
        handle, qnp ));

    return((PRTraceHandle)qnp);
} 




PR_IMPLEMENT(PRTraceHandle) 
	PR_FindNextTraceRname( 
        PRTraceHandle rhandle,
        PRTraceHandle qhandle
)
{
    RName *rnp = (RName *)rhandle;
    QName *qnp = (QName *)qhandle;


    if ( PR_CLIST_IS_EMPTY( &qnp->rNameList ))
        rnp = NULL;
    else if ( rnp == NULL )
        rnp = (RName *)PR_LIST_HEAD( &qnp->rNameList );
    else if ( PR_NEXT_LINK( &rnp->link ) ==  &qnp->rNameList )
        rnp = NULL;
    else
        rnp = (RName *)PR_NEXT_LINK( &rnp->link );

    PR_LOG( lm, PR_LOG_DEBUG, ("PRTrace: FindNextRname: Rhandle: %p, QHandle: %p, Returns: %p", 
        rhandle, qhandle, rnp ));

    return((PRTraceHandle)rnp);
} 
    



static PRFileDesc * InitializeRecording( void )
{
    char    *logFileName;
    PRFileDesc  *logFile;

    
    if ( traceLock == NULL )
        _PR_InitializeTrace();

    PR_LOG( lm, PR_LOG_DEBUG,
        ("PR_RecordTraceEntries: begins"));

    logLostData = 0; 
    logState = LogReset;

#ifdef XP_UNIX
    if ((getuid() != geteuid()) || (getgid() != getegid())) {
        return NULL;
    }
#endif 

    
    logFileName = PR_GetEnv( "NSPR_TRACE_LOG" );
    if ( logFileName == NULL )
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("RecordTraceEntries: Environment variable not defined. Exiting"));
        return NULL;
    }
    
    
    logFile = PR_Open( logFileName, PR_WRONLY | PR_CREATE_FILE, 0666 );
    if ( logFile == NULL )
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("RecordTraceEntries: Cannot open %s as trace log file. OS error: %ld", 
		logFileName, PR_GetOSError()));
        return NULL;
    }
    return logFile;
} 




static void ProcessOrders( void )
{
    switch ( logOrder )
    {
    case LogReset :
        logOrder = logState = localState;
        PR_LOG( lm, PR_LOG_DEBUG,
            ("RecordTraceEntries: LogReset"));
        break;

    case LogSuspend :
        localState = logOrder = logState = LogSuspend;
        PR_LOG( lm, PR_LOG_DEBUG,
            ("RecordTraceEntries: LogSuspend"));
        break;

    case LogResume :
        localState = logOrder = logState = LogActive;
        PR_LOG( lm, PR_LOG_DEBUG,
            ("RecordTraceEntries: LogResume"));
        break;

    case LogStop :
        logOrder = logState = LogStop;
        PR_LOG( lm, PR_LOG_DEBUG,
            ("RecordTraceEntries: LogStop"));
        break;

    default :
        PR_LOG( lm, PR_LOG_ERROR,
            ("RecordTraceEntries: Invalid logOrder: %ld", logOrder ));
        PR_ASSERT( 0 );
        break;
    } 
    return ;
} 




static void WriteTraceSegment( PRFileDesc *logFile, void *buf, PRInt32 amount )
{
    PRInt32 rc;


    PR_LOG( lm, PR_LOG_ERROR,
        ("WriteTraceSegment: Buffer: %p, Amount: %ld", buf, amount));
    rc = PR_Write( logFile, buf , amount );
    if ( rc == -1 )
        PR_LOG( lm, PR_LOG_ERROR,
            ("RecordTraceEntries: PR_Write() failed. Error: %ld", PR_GetError() ));
    else if ( rc != amount )
        PR_LOG( lm, PR_LOG_ERROR,
            ("RecordTraceEntries: PR_Write() Tried to write: %ld, Wrote: %ld", amount, rc));
    else 
        PR_LOG( lm, PR_LOG_DEBUG,
            ("RecordTraceEntries: PR_Write(): Buffer: %p, bytes: %ld", buf, amount));

    return;
} 




PR_IMPLEMENT(void)
	PR_RecordTraceEntries(
        void 
)
{
    PRFileDesc  *logFile;
    PRInt32     lostSegments;
    PRInt32     currentSegment = 0;
    void        *buf;
    PRBool      doWrite;

    logFile = InitializeRecording();
    if ( logFile == NULL )
    {
        PR_LOG( lm, PR_LOG_DEBUG,
            ("PR_RecordTraceEntries: Failed to initialize"));
        return;
    }

    
    while ( logState != LogStop )
    {

        PR_Lock( logLock );

        while ( (logCount == 0) && ( logOrder == logState ) )
            PR_WaitCondVar( logCVar, PR_INTERVAL_NO_TIMEOUT );

        
        if ( logOrder != logState )
            ProcessOrders();

        
        if ( logCount )
        {
            lostSegments = logCount - logSegments;
            if ( lostSegments > 0 )
            {
                logLostData += ( logCount - logSegments );
                logCount = (logCount % logSegments);
                currentSegment = logCount;
                PR_LOG( lm, PR_LOG_DEBUG,
                    ("PR_RecordTraceEntries: LostData segments: %ld", logLostData));
            }
            else
            {
                logCount--;
            }

            buf = tBuf + ( logEntriesPerSegment * currentSegment );
            if (++currentSegment >= logSegments )
                currentSegment = 0;
            doWrite = PR_TRUE;
        }
        else
            doWrite = PR_FALSE;

        PR_Unlock( logLock );

        if ( doWrite == PR_TRUE )
        {
            if ( localState != LogSuspend )
                WriteTraceSegment( logFile, buf, logSegSize );
            else
                PR_LOG( lm, PR_LOG_DEBUG,
                    ("RecordTraceEntries: PR_Write(): is suspended" ));
        }

    } 

    PR_Close( logFile );
    PR_LOG( lm, PR_LOG_DEBUG,
        ("RecordTraceEntries: exiting"));
    return;
} 




PR_IMPLEMENT(PRIntn)
    PR_GetTraceEntries(
        PRTraceEntry    *buffer,    
        PRInt32         count,      
        PRInt32         *found      
)
{
    PRInt32 rc; 
    PRInt32 copied = 0;
    
    PR_Lock( traceLock );
    
    



    PR_LOG( lm, PR_LOG_ERROR,
        ("PR_GetTraceEntries: Next: %ld, LastSeen: %ld", next, fetchLastSeen));

    if ( fetchLastSeen <= next )
    {
        while (( count-- > 0 ) && (fetchLastSeen < next ))
        {
            *(buffer + copied++) = *(tBuf + fetchLastSeen++);
        }
        PR_LOG( lm, PR_LOG_ERROR,
            ("PR_GetTraceEntries: Copied: %ld, LastSeen: %ld", copied, fetchLastSeen));
    }
    else 
    {
        while ( count-- > 0  && fetchLastSeen <= last )
        {
            *(buffer + copied++) = *(tBuf + fetchLastSeen++);
        }
        fetchLastSeen = 0;

        PR_LOG( lm, PR_LOG_ERROR,
            ("PR_GetTraceEntries: Copied: %ld, LastSeen: %ld", copied, fetchLastSeen));

        while ( count-- > 0  && fetchLastSeen < next )
        {
            *(buffer + copied++) = *(tBuf + fetchLastSeen++);
        }
        PR_LOG( lm, PR_LOG_ERROR,
            ("PR_GetTraceEntries: Copied: %ld, LastSeen: %ld", copied, fetchLastSeen));
    }

    *found = copied;
    rc = ( fetchLostData == PR_TRUE )? 1 : 0;
    fetchLostData = PR_FALSE;

    PR_Unlock( traceLock );
    return rc;
} 


