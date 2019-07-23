




































#include <stdio.h>
#include "nsCOMPtr.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsNetUtil.h"









static nsCOMPtr<nsIEventQueue>
createEventQueue() {
    nsCOMPtr<nsIEventQueue> result;
    
    nsresult rv = NS_OK;
    nsCOMPtr<nsIEventQueueService> eqs = 
             do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
    if ( NS_SUCCEEDED( rv ) ) {
            eqs->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(result));
    } else {
        printf( "%s %d: NS_WITH_SERVICE(nsIEventQueueService) failed, rv=0x%08X\n",
                (char*)__FILE__, (int)__LINE__, (int)rv );
    }
    return result;
}


static nsCOMPtr<nsIChannel>
createChannel( const char *url ) {
    nsCOMPtr<nsIInputStream> result;

    nsCOMPtr<nsIURI> uri;
    printf( "Calling NS_NewURI for %s...\n", url );
    nsresult rv = NS_NewURI( getter_AddRefs( uri ), url );

    if ( NS_SUCCEEDED( rv ) ) {
        printf( "...NS_NewURI completed OK\n" );

        
        printf( "Calling NS_OpenURI...\n" );
        nsresult rv = NS_OpenURI( getter_AddRefs( result ), uri, 0 );

        if ( NS_SUCCEEDED( rv ) ) {
            printf( "...NS_OpenURI completed OK\n" );
        } else {
            printf( "%s %d: NS_OpenURI failed, rv=0x%08X\n",
                    (char*)__FILE__, (int)__LINE__, (int)rv );
        }
    } else {
        printf( "%s %d: NS_NewURI failed, rv=0x%08X\n",
                (char*)__FILE__, (int)__LINE__, (int)rv );
    }
    return result;
}


class TestListener : public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMOBSERVER

    TestListener();
    ~TestListener();
    static void IOThread( void *p );

private:
    PRBool mDone;
    int    mThreadNo;
    FILE  *mFile;
    static int threadCount;
}; 

int TestListener::threadCount = 0;

TestListener::TestListener()
    : mDone( PR_FALSE ), mThreadNo( ++threadCount ) {
    printf( "TestListener ctor called on thread %d\n", mThreadNo );
}

TestListener::~TestListener() {
    printf( "TestListener dtor called on thread %d\n", mThreadNo );
}

NS_IMPL_ISUPPORTS2( TestListener, nsIStreamListener, nsIRequestObserver )

NS_IMETHODIMP
TestListener::OnStartRequest( nsIChannel *aChannel, nsISupports *aContext ) {
    nsresult rv = NS_OK;

    printf( "TestListener::OnStartRequest called on thread %d\n", mThreadNo );

    
    char fileName[32];
    sprintf( fileName, "%s%d", "thread", mThreadNo );
    mFile = fopen( fileName, "wb" );
    setbuf( mFile, 0 );

    return rv;
}

NS_IMETHODIMP
TestListener::OnStopRequest( nsIChannel *aChannel,
                             nsISupports *aContext,
                             nsresult aStatus,
                             const PRUnichar *aMsg ) {
    nsresult rv = NS_OK;

    printf( "TestListener::OnStopRequest called on thread %d\n", mThreadNo );

    fclose( mFile );
    mDone = PR_TRUE;

    return rv;
}

NS_IMETHODIMP
TestListener::OnDataAvailable( nsIChannel *aChannel,
                               nsISupports *aContext,
                               nsIInputStream *aStream,
                               PRUint32 offset,
                               PRUint32 aLength ) {
    nsresult rv = NS_OK;

    printf( "TestListener::OnDataAvailable called on thread %d\n", mThreadNo );

    
    
    char buffer[ 8192 ];
    unsigned long bytesRemaining = aLength;
    while ( bytesRemaining ) {
        unsigned int bytesRead;
        
        rv = aStream->Read( buffer,
                            PR_MIN( sizeof( buffer ), bytesRemaining ),
                            &bytesRead );
        if ( NS_SUCCEEDED( rv ) ) {
            
            fwrite( buffer, 1, bytesRead, mFile );
            bytesRemaining -= bytesRead;
        } else {
            printf( "%s %d: Read error, rv=0x%08X\n",
                    (char*)__FILE__, (int)__LINE__, (int)rv );
            break;
        }
    }
    printf( "\n" );

    return rv;
}



void
TestListener::IOThread( void *p ) {
    printf( "I/O thread (0x%08X) started...\n", (int)(void*)PR_GetCurrentThread() );

    
    nsIEventQueue *mainThreadQ = NS_STATIC_CAST( nsIEventQueue*, p );

    
    nsCOMPtr<nsIChannel> channel = createChannel( (const char*)p );

    if ( channel ) {
        
        nsCOMPtr<nsIEventQueue> ioEventQ = createEventQueue();

        if ( ioEventQ ) {
            
            TestListener *testListener = new TestListener();
            testListener->AddRef();

            
            printf( "Doing AsyncRead...\n" );
            nsresult rv = channel->AsyncRead( testListener, 0 );

            if ( NS_SUCCEEDED( rv ) ) {
                printf( "...AsyncRead completed OK\n" );

                
                printf( "Start event loop on io thread %d...\n", testListener->mThreadNo );
                while ( !testListener->mDone ) {
                    PLEvent *event;
                    ioEventQ->GetEvent( &event );
                    ioEventQ->HandleEvent( event );
                }
                printf( "...io thread %d event loop exiting\n", testListener->mThreadNo );
            } else {
                printf( "%s %d: AsyncRead failed on thread %d, rv=0x%08X\n",
                        (char*)__FILE__, (int)__LINE__, testListener->mThreadNo, (int)rv );
            }

            
            testListener->Release();
        }
    }

    printf( "...I/O thread terminating\n" );
}

static const int maxThreads = 5;

int
main( int argc, char* argv[] ) {
    setbuf( stdout, 0 );
    if ( argc < 2 || argc > maxThreads + 1 ) {
        printf( "usage: testThreadedIO url1 <url2>...\n"
                "where <url#> is a location to be loaded on a separate thread\n"
                "limit is %d urls/threads", maxThreads );
        return -1;
    }

    nsresult rv= (nsresult)-1;

    printf( "Test starting...\n" );

    
    printf( "Initializing XPCOM...\n" );
    rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if ( NS_FAILED( rv ) ) {
        printf( "%s %d: NS_InitXPCOM failed, rv=0x%08X\n",
                (char*)__FILE__, (int)__LINE__, (int)rv );
        return rv;
    }
    printf( "...XPCOM initialized OK\n" );
    
    printf( "Creating event queue for main thread (0x%08X)...\n",
            (int)(void*)PR_GetCurrentThread() );
    {
        nsCOMPtr<nsIEventQueue> mainThreadQ = createEventQueue();

        if ( mainThreadQ ) {
            printf( "...main thread's event queue created OK\n" );

            
            int goodThreads = 0;
            PRThread *thread[ maxThreads ];
            for ( int threadNo = 1; threadNo < argc; threadNo++ ) {
                printf( "Creating I/O thread %d to load %s...\n", threadNo, argv[threadNo] );
                PRThread *ioThread = PR_CreateThread( PR_USER_THREAD,
                                                      TestListener::IOThread,
                                                      argv[threadNo],
                                                      PR_PRIORITY_NORMAL,
                                                      PR_LOCAL_THREAD,
                                                      PR_JOINABLE_THREAD,
                                                      0 );
                if ( ioThread ) {
                    thread[ goodThreads++ ] = ioThread;
                    printf( "...I/O thread %d (0x%08X) created OK\n",
                            threadNo, (int)(void*)ioThread );
                } else {
                    printf( "%s %d: PR_CreateThread for thread %d failed\n",
                            (char*)__FILE__, (int)__LINE__, threadNo );
                }
            }

            
            for ( int joinThread = 0; joinThread < goodThreads; joinThread++ ) {
                printf( "Waiting for thread %d to terminate...\n", joinThread+1 );
                PR_JoinThread( thread[ joinThread ] );
            }
        }
    } 
    
    
    printf( "Shutting down XPCOM...\n" );
    NS_ShutdownXPCOM( 0 );
    printf( "...XPCOM shutdown complete\n" );

    
    printf( "...test complete, rv=0x%08X\n", (int)rv );
    return rv;
}
