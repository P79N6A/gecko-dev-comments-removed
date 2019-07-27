




#include "nsNativeAppSupportBase.h"
#include "nsNativeAppSupportWin.h"
#include "nsAppRunner.h"
#include "nsXULAppAPI.h"
#include "nsString.h"
#include "nsIBrowserDOMWindow.h"
#include "nsICommandLineRunner.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDOMChromeWindow.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIWindowWatcher.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIAppShellService.h"
#include "nsIXULWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPromptService.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIDOMLocation.h"
#include "nsIWebNavigation.h"
#include "nsIWindowMediator.h"
#include "nsNativeCharsetUtils.h"
#include "nsIAppStartup.h"

#include <windows.h>
#include <shellapi.h>
#include <ddeml.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>

using namespace mozilla;

static HWND hwndForDOMWindow( nsISupports * );

static
nsresult
GetMostRecentWindow(const char16_t* aType, nsIDOMWindow** aWindow) {
    nsresult rv;
    nsCOMPtr<nsIWindowMediator> med( do_GetService( NS_WINDOWMEDIATOR_CONTRACTID, &rv ) );
    if ( NS_FAILED( rv ) )
        return rv;

    if ( med )
        return med->GetMostRecentWindow( aType, aWindow );

    return NS_ERROR_FAILURE;
}

static
void
activateWindow( nsIDOMWindow *win ) {
    
    HWND hwnd = hwndForDOMWindow( win );
    if ( hwnd ) {
        
        if ( ::IsIconic( hwnd ) ) {
            ::ShowWindow( hwnd, SW_RESTORE );
        }
        
        ::SetForegroundWindow( hwnd );
    } else {
        
        win->Focus();
    }
}


#ifdef DEBUG_law
#undef MOZ_DEBUG_DDE
#define MOZ_DEBUG_DDE 1
#endif


struct Mutex {
    Mutex( const char16_t *name )
        : mName( name ),
          mHandle( 0 ),
          mState( -1 ) {
        mHandle = CreateMutexW( 0, FALSE, mName.get() );
#if MOZ_DEBUG_DDE
        printf( "CreateMutex error = 0x%08X\n", (int)GetLastError() );
#endif
    }
    ~Mutex() {
        if ( mHandle ) {
            
            Unlock();

            BOOL rc = CloseHandle( mHandle );
#if MOZ_DEBUG_DDE
            if ( !rc ) {
                printf( "CloseHandle error = 0x%08X\n", (int)GetLastError() );
            }
#endif
        }
    }
    BOOL Lock( DWORD timeout ) {
        if ( mHandle ) {
#if MOZ_DEBUG_DDE
            printf( "Waiting (%d msec) for DDE mutex...\n", (int)timeout );
#endif
            mState = WaitForSingleObject( mHandle, timeout );
#if MOZ_DEBUG_DDE
            printf( "...wait complete, result = 0x%08X, GetLastError=0x%08X\n", (int)mState, (int)::GetLastError() );
#endif
            return mState == WAIT_OBJECT_0 || mState == WAIT_ABANDONED;
        } else {
            return FALSE;
        }
    }
    void Unlock() {
        if ( mHandle && mState == WAIT_OBJECT_0 ) {
#if MOZ_DEBUG_DDE
            printf( "Releasing DDE mutex\n" );
#endif
            ReleaseMutex( mHandle );
            mState = -1;
        }
    }
private:
    nsString  mName;
    HANDLE    mHandle;
    DWORD     mState;
};


























































































































class nsNativeAppSupportWin : public nsNativeAppSupportBase,
                              public nsIObserver
{
public:
    NS_DECL_NSIOBSERVER
    NS_DECL_ISUPPORTS_INHERITED

    
    NS_IMETHOD Start( bool *aResult );
    NS_IMETHOD Stop( bool *aResult );
    NS_IMETHOD Quit();
    NS_IMETHOD Enable();
    
    NS_IMETHOD StartDDE();
    
    
    
    void CheckConsole();

private:
    ~nsNativeAppSupportWin() {}
    static void HandleCommandLine(const char* aCmdLineString, nsIFile* aWorkingDir, uint32_t aState);
    static HDDEDATA CALLBACK HandleDDENotification( UINT     uType,
                                                    UINT     uFmt,
                                                    HCONV    hconv,
                                                    HSZ      hsz1,
                                                    HSZ      hsz2,
                                                    HDDEDATA hdata,
                                                    ULONG_PTR dwData1,
                                                    ULONG_PTR dwData2 );
    static void ParseDDEArg( HSZ args, int index, nsString& string);
    static void ParseDDEArg( const WCHAR* args, int index, nsString& aString);
    static HDDEDATA CreateDDEData( DWORD value );
    static HDDEDATA CreateDDEData( LPBYTE value, DWORD len );
    static bool     InitTopicStrings();
    static int      FindTopic( HSZ topic );
    static void ActivateLastWindow();
    static nsresult OpenWindow( const char *urlstr, const char *args );
    static nsresult OpenBrowserWindow();
    static void     SetupSysTrayIcon();
    static void     RemoveSysTrayIcon();

    static int   mConversations;
    enum {
        topicOpenURL,
        topicActivate,
        topicCancelProgress,
        topicVersion,
        topicRegisterViewer,
        topicUnRegisterViewer,
        topicGetWindowInfo,
        
        topicCount 
    };
    static HSZ   mApplication, mTopics[ topicCount ];
    static DWORD mInstance;
    static bool mCanHandleRequests;
    static char16_t mMutexName[];
    friend struct MessageWindow;
}; 

NS_INTERFACE_MAP_BEGIN(nsNativeAppSupportWin)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END_INHERITING(nsNativeAppSupportBase)

NS_IMPL_ADDREF_INHERITED(nsNativeAppSupportWin, nsNativeAppSupportBase)
NS_IMPL_RELEASE_INHERITED(nsNativeAppSupportWin, nsNativeAppSupportBase)

void
UseParentConsole()
{
    
    
    
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (_fileno(stdout) == -2 ||
            _get_osfhandle(fileno(stdout)) == -2)
            freopen("CONOUT$", "w", stdout);
        
        
        if (_fileno(stderr) == -2 ||
            _get_osfhandle(fileno(stderr)) == -2)
            freopen("CONOUT$", "w", stderr);
        if (_fileno(stdin) == -2 || _get_osfhandle(fileno(stdin)) == -2)
            freopen("CONIN$", "r", stdin);
    }
}

void
nsNativeAppSupportWin::CheckConsole() {
    for ( int i = 1; i < gArgc; i++ ) {
        if ( strcmp( "-console", gArgv[i] ) == 0 ||
             strcmp( "--console", gArgv[i] ) == 0 ||
             strcmp( "/console", gArgv[i] ) == 0 ) {
            
            
            BOOL rc = ::AllocConsole();
            if ( rc ) {
                
                

                
                int hCrt = ::_open_osfhandle( (intptr_t)GetStdHandle( STD_OUTPUT_HANDLE ),
                                            _O_TEXT );
                if ( hCrt != -1 ) {
                    FILE *hf = ::_fdopen( hCrt, "w" );
                    if ( hf ) {
                        *stdout = *hf;
#ifdef DEBUG
                        ::fprintf( stdout, "stdout directed to dynamic console\n" );
#endif
                    }
                }

                
                hCrt = ::_open_osfhandle( (intptr_t)::GetStdHandle( STD_ERROR_HANDLE ),
                                          _O_TEXT );
                if ( hCrt != -1 ) {
                    FILE *hf = ::_fdopen( hCrt, "w" );
                    if ( hf ) {
                        *stderr = *hf;
#ifdef DEBUG
                        ::fprintf( stderr, "stderr directed to dynamic console\n" );
#endif
                    }
                }

                
                









            } else {
                
                
            }
            
            do {
                gArgv[i] = gArgv[i + 1];
                ++i;
            } while (gArgv[i]);

            --gArgc;

        } else if ( strcmp( "-attach-console", gArgv[i] ) == 0
                    ||
                    strcmp( "/attach-console", gArgv[i] ) == 0 ) {
            UseParentConsole();
        }
    }

    return;
}



nsresult
NS_CreateNativeAppSupport( nsINativeAppSupport **aResult ) {
    nsNativeAppSupportWin *pNative = new nsNativeAppSupportWin;
    if (!pNative) return NS_ERROR_OUT_OF_MEMORY;

    
    pNative->CheckConsole();

    *aResult = pNative;
    NS_ADDREF( *aResult );

    return NS_OK;
}


#define MOZ_DDE_APPLICATION    "Mozilla"
#define MOZ_MUTEX_NAMESPACE    L"Local\\"
#define MOZ_STARTUP_MUTEX_NAME L"StartupMutex"
#define MOZ_DDE_START_TIMEOUT 30000
#define MOZ_DDE_STOP_TIMEOUT  15000
#define MOZ_DDE_EXEC_TIMEOUT  15000


const char * const topicNames[] = { "WWW_OpenURL",
                                    "WWW_Activate",
                                    "WWW_CancelProgress",
                                    "WWW_Version",
                                    "WWW_RegisterViewer",
                                    "WWW_UnRegisterViewer",
                                    "WWW_GetWindowInfo" };


int   nsNativeAppSupportWin::mConversations = 0;
HSZ   nsNativeAppSupportWin::mApplication   = 0;
HSZ   nsNativeAppSupportWin::mTopics[nsNativeAppSupportWin::topicCount] = { 0 };
DWORD nsNativeAppSupportWin::mInstance      = 0;
bool nsNativeAppSupportWin::mCanHandleRequests   = false;

char16_t nsNativeAppSupportWin::mMutexName[ 128 ] = { 0 };



struct MessageWindow {
    
    MessageWindow() {
        
        mHandle = ::FindWindowW( className(), 0 );
    }

    
    operator HWND() {
        return mHandle;
    }

    
    static const wchar_t *className() {
        static wchar_t classNameBuffer[128];
        static wchar_t *mClassName = 0;
        if ( !mClassName ) {
            ::_snwprintf(classNameBuffer,
                         128,   
                         L"%s%s",
                         NS_ConvertUTF8toUTF16(gAppData->remotingName).get(),
                         L"MessageWindow" );
            mClassName = classNameBuffer;
        }
        return mClassName;
    }

    
    NS_IMETHOD Create() {
        WNDCLASSW classStruct = { 0,                          
                                 &MessageWindow::WindowProc, 
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 className() };              

        
        NS_ENSURE_TRUE( ::RegisterClassW( &classStruct ), NS_ERROR_FAILURE );

        
        NS_ENSURE_TRUE( ( mHandle = ::CreateWindowW(className(),
                                                    0,          
                                                    WS_CAPTION, 
                                                    0,0,0,0,    
                                                    0,          
                                                    0,          
                                                    0,          
                                                    0 ) ),      
                        NS_ERROR_FAILURE );

#if MOZ_DEBUG_DDE
        printf( "Message window = 0x%08X\n", (int)mHandle );
#endif

        return NS_OK;
    }

    
    NS_IMETHOD Destroy() {
        nsresult retval = NS_OK;

        if ( mHandle ) {
            
            
            BOOL desRes = DestroyWindow( mHandle );
            if ( FALSE != desRes ) {
                mHandle = nullptr;
            }
            else {
                retval = NS_ERROR_FAILURE;
            }
        }

        return retval;
    }

    
    NS_IMETHOD SendRequest() {
        WCHAR *cmd = ::GetCommandLineW();
        WCHAR cwd[MAX_PATH];
        _wgetcwd(cwd, MAX_PATH);

        
        NS_ConvertUTF16toUTF8 utf8buffer(cmd);
        utf8buffer.Append('\0');
        AppendUTF16toUTF8(cwd, utf8buffer);
        utf8buffer.Append('\0');

        
        
        COPYDATASTRUCT cds = {
            1,
            utf8buffer.Length(),
            (void*) utf8buffer.get()
        };
        
        
        ::SetForegroundWindow( mHandle );
        ::SendMessage( mHandle, WM_COPYDATA, 0, (LPARAM)&cds );
        return NS_OK;
    }

    
    static LRESULT CALLBACK WindowProc( HWND msgWindow, UINT msg, WPARAM wp, LPARAM lp ) {
        if ( msg == WM_COPYDATA ) {
            if (!nsNativeAppSupportWin::mCanHandleRequests)
                return FALSE;

            
            COPYDATASTRUCT *cds = (COPYDATASTRUCT*)lp;
#if MOZ_DEBUG_DDE
            printf( "Incoming request: %s\n", (const char*)cds->lpData );
#endif
            nsCOMPtr<nsIFile> workingDir;

            if (1 >= cds->dwData) {
                char* wdpath = (char*) cds->lpData;
                
                
                while (*wdpath)
                    ++wdpath;

                ++wdpath;

#ifdef MOZ_DEBUG_DDE
                printf( "Working dir: %s\n", wdpath);
#endif

                NS_NewLocalFile(NS_ConvertUTF8toUTF16(wdpath),
                                false,
                                getter_AddRefs(workingDir));
            }
            (void)nsNativeAppSupportWin::HandleCommandLine((char*)cds->lpData, workingDir, nsICommandLine::STATE_REMOTE_AUTO);

            
            nsCOMPtr<nsIDOMWindow> win;
            GetMostRecentWindow( 0, getter_AddRefs( win ) );
            return win ? (LRESULT)hwndForDOMWindow( win ) : 0;
        }
        return DefWindowProc( msgWindow, msg, wp, lp );
    }

private:
    HWND mHandle;
}; 













NS_IMETHODIMP
nsNativeAppSupportWin::Start( bool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );
    NS_ENSURE_STATE( gAppData );

    if (getenv("MOZ_NO_REMOTE"))
    {
        *aResult = true;
        return NS_OK;
    }

    nsresult rv = NS_ERROR_FAILURE;
    *aResult = false;

    

    
    ::_snwprintf(reinterpret_cast<wchar_t*>(mMutexName),
                 sizeof mMutexName / sizeof(char16_t), L"%s%s%s",
                 MOZ_MUTEX_NAMESPACE,
                 NS_ConvertUTF8toUTF16(gAppData->name).get(),
                 MOZ_STARTUP_MUTEX_NAME );
    Mutex startupLock = Mutex( mMutexName );

    NS_ENSURE_TRUE( startupLock.Lock( MOZ_DDE_START_TIMEOUT ), NS_ERROR_FAILURE );

    
    MessageWindow msgWindow;
    if ( (HWND)msgWindow ) {
        
        rv = msgWindow.SendRequest();
    } else {
        
        rv = msgWindow.Create();
        if ( NS_SUCCEEDED( rv ) ) {
            
            this->StartDDE();
            
            *aResult = true;
        }
    }

    startupLock.Unlock();

    return rv;
}

bool
nsNativeAppSupportWin::InitTopicStrings() {
    for ( int i = 0; i < topicCount; i++ ) {
        if ( !( mTopics[ i ] = DdeCreateStringHandleA( mInstance, const_cast<char *>(topicNames[ i ]), CP_WINANSI ) ) ) {
            return false;
        }
    }
    return true;
}

int
nsNativeAppSupportWin::FindTopic( HSZ topic ) {
    for ( int i = 0; i < topicCount; i++ ) {
        if ( DdeCmpStringHandles( topic, mTopics[i] ) == 0 ) {
            return i;
        }
    }
    return -1;
}












NS_IMETHODIMP
nsNativeAppSupportWin::StartDDE() {
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    
    NS_ENSURE_TRUE( DMLERR_NO_ERROR == DdeInitialize( &mInstance,
                                                      nsNativeAppSupportWin::HandleDDENotification,
                                                      APPCLASS_STANDARD,
                                                      0 ),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( ( mApplication = DdeCreateStringHandleA( mInstance, (char*) gAppData->name, CP_WINANSI ) ) && InitTopicStrings(),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( DdeNameService( mInstance, mApplication, 0, DNS_REGISTER ), NS_ERROR_FAILURE );

#if MOZ_DEBUG_DDE
    printf( "DDE server started\n" );
#endif

    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportWin::Stop( bool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance, NS_ERROR_NOT_INITIALIZED );

    nsresult rv = NS_OK;
    *aResult = true;

    Mutex ddeLock( mMutexName );

    if ( ddeLock.Lock( MOZ_DDE_STOP_TIMEOUT ) ) {
        if ( mConversations == 0 ) {
            this->Quit();
        } else {
            *aResult = false;
        }

        ddeLock.Unlock();
    }
    else {
        
        
        *aResult = true;
    }

    return rv;
}

NS_IMETHODIMP
nsNativeAppSupportWin::Observe(nsISupports* aSubject, const char* aTopic,
                               const char16_t* aData)
{
    if (strcmp(aTopic, "quit-application") == 0) {
        Quit();
    } else {
        NS_ERROR("Unexpected observer topic.");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportWin::Quit() {
    
    
    
    
    Mutex mutexLock(mMutexName);
    NS_ENSURE_TRUE(mutexLock.Lock(MOZ_DDE_START_TIMEOUT), NS_ERROR_FAILURE);

    
    
    
    
    MessageWindow mw;
    mw.Destroy();

    if ( mInstance ) {
        
        DdeNameService( mInstance, mApplication, 0, DNS_UNREGISTER );
        
        if ( mApplication ) {
            DdeFreeStringHandle( mInstance, mApplication );
            mApplication = 0;
        }
        for ( int i = 0; i < topicCount; i++ ) {
            if ( mTopics[i] ) {
                DdeFreeStringHandle( mInstance, mTopics[i] );
                mTopics[i] = 0;
            }
        }
        DdeUninitialize( mInstance );
        mInstance = 0;
#if MOZ_DEBUG_DDE
    printf( "DDE server stopped\n" );
#endif
    }

    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportWin::Enable()
{
    mCanHandleRequests = true;

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->AddObserver(this, "quit-application", false);
    } else {
        NS_ERROR("No observer service?");
    }

    return NS_OK;
}

#if MOZ_DEBUG_DDE

#define XTYP_CASE(t) \
    case t: result = #t; break

static nsCString uTypeDesc( UINT uType ) {
    nsCString result;
    switch ( uType ) {
    XTYP_CASE(XTYP_ADVSTART);
    XTYP_CASE(XTYP_CONNECT);
    XTYP_CASE(XTYP_ADVREQ);
    XTYP_CASE(XTYP_REQUEST);
    XTYP_CASE(XTYP_WILDCONNECT);
    XTYP_CASE(XTYP_ADVDATA);
    XTYP_CASE(XTYP_EXECUTE);
    XTYP_CASE(XTYP_POKE);
    XTYP_CASE(XTYP_ADVSTOP);
    XTYP_CASE(XTYP_CONNECT_CONFIRM);
    XTYP_CASE(XTYP_DISCONNECT);
    XTYP_CASE(XTYP_ERROR);
    XTYP_CASE(XTYP_MONITOR);
    XTYP_CASE(XTYP_REGISTER);
    XTYP_CASE(XTYP_XACT_COMPLETE);
    XTYP_CASE(XTYP_UNREGISTER);
    default: result = "XTYP_?????";
    }
    return result;
}

static nsCString hszValue( DWORD instance, HSZ hsz ) {
    
    nsCString result("[");
    DWORD len = DdeQueryString( instance, hsz, nullptr, nullptr, CP_WINANSI );
    if ( len ) {
        char buffer[ 256 ];
        DdeQueryString( instance, hsz, buffer, sizeof buffer, CP_WINANSI );
        result += buffer;
    }
    result += "]";
    return result;
}
#else


static nsCString uTypeDesc( UINT ) {
    return nsCString( "?" );
}
static nsCString hszValue( DWORD, HSZ ) {
    return nsCString( "?" );
}
#endif



static void escapeQuotes( nsAString &aString ) {
    int32_t offset = -1;
    while( 1 ) {
       
       offset = aString.FindChar( '"', ++offset );
       if ( offset == kNotFound ) {
           
           break;
       } else {
           
           aString.Insert( char16_t('\\'), offset );
           
           offset++;
       }
    }
    return;
}

HDDEDATA CALLBACK
nsNativeAppSupportWin::HandleDDENotification( UINT uType,       
                                              UINT uFmt,        
                                              HCONV hconv,      
                                              HSZ hsz1,         
                                              HSZ hsz2,         
                                              HDDEDATA hdata,   
                                              ULONG_PTR dwData1,    
                                              ULONG_PTR dwData2 ) { 

    if (!mCanHandleRequests)
        return 0;


#if MOZ_DEBUG_DDE
    printf( "DDE: uType  =%s\n",      uTypeDesc( uType ).get() );
    printf( "     uFmt   =%u\n",      (unsigned)uFmt );
    printf( "     hconv  =%08x\n",    (int)hconv );
    printf( "     hsz1   =%08x:%s\n", (int)hsz1, hszValue( mInstance, hsz1 ).get() );
    printf( "     hsz2   =%08x:%s\n", (int)hsz2, hszValue( mInstance, hsz2 ).get() );
    printf( "     hdata  =%08x\n",    (int)hdata );
    printf( "     dwData1=%08x\n",    (int)dwData1 );
    printf( "     dwData2=%08x\n",    (int)dwData2 );
#endif

    HDDEDATA result = 0;
    if ( uType & XCLASS_BOOL ) {
        switch ( uType ) {
            case XTYP_CONNECT:
                
                if ( FindTopic( hsz1 ) != -1 ) {
                    
                    result = (HDDEDATA)1;
                }
                break;
            case XTYP_CONNECT_CONFIRM:
                
                result = (HDDEDATA)1;
                break;
        }
    } else if ( uType & XCLASS_DATA ) {
        if ( uType == XTYP_REQUEST ) {
            switch ( FindTopic( hsz1 ) ) {
                case topicOpenURL: {
                    

                    
                    nsAutoString url;
                    ParseDDEArg(hsz2, 0, url);

                    
                    
                    nsAutoString windowID;
                    ParseDDEArg(hsz2, 2, windowID);
                    
                    if ( windowID.IsEmpty() ) {
                        url.Insert(NS_LITERAL_STRING("mozilla -new-window "), 0);
                    }
                    else {
                        url.Insert(NS_LITERAL_STRING("mozilla -url "), 0);
                    }

#if MOZ_DEBUG_DDE
                    printf( "Handling dde XTYP_REQUEST request: [%s]...\n", NS_ConvertUTF16toUTF8(url).get() );
#endif
                    
                    HandleCommandLine(NS_ConvertUTF16toUTF8(url).get(), nullptr, nsICommandLine::STATE_REMOTE_EXPLICIT);

                    
                    result = CreateDDEData( 1 );
                    break;
                }
                case topicGetWindowInfo: {
                    
                    
                    
                    
                    

                    
                    
                    
                    do {
                        
                        nsCOMPtr<nsIDOMWindow> navWin;
                        GetMostRecentWindow( NS_LITERAL_STRING( "navigator:browser" ).get(),
                                             getter_AddRefs( navWin ) );
                        if ( !navWin ) {
                            
                            break;
                        }
                        
                        nsCOMPtr<nsIDOMWindow> content;
                        navWin->GetContent( getter_AddRefs( content ) );
                        if ( !content ) {
                            break;
                        }
                        
                        nsCOMPtr<nsPIDOMWindow> internalContent( do_QueryInterface( content ) );
                        if ( !internalContent ) {
                            break;
                        }
                        
                        nsCOMPtr<nsIDOMLocation> location;
                        internalContent->GetLocation( getter_AddRefs( location ) );
                        if ( !location ) {
                            break;
                        }
                        
                        nsAutoString url;
                        if ( NS_FAILED( location->GetHref( url ) ) ) {
                            break;
                        }
                        
                        escapeQuotes( url );

                        

                        
                        nsCOMPtr<nsIBaseWindow> baseWindow =
                          do_QueryInterface( internalContent->GetDocShell() );
                        if ( !baseWindow ) {
                            break;
                        }
                        
                        nsXPIDLString title;
                        if(!baseWindow) {
                            break;
                        }
                        baseWindow->GetTitle(getter_Copies(title));
                        
                        escapeQuotes( title );

                        
                        
                        nsAutoCString   outpt( NS_LITERAL_CSTRING("\"") );
                        
                        
                        nsAutoCString tmpNativeStr;
                        NS_CopyUnicodeToNative( url, tmpNativeStr );
                        outpt.Append( tmpNativeStr );
                        
                        
                        outpt.Append( NS_LITERAL_CSTRING("\",\"") );
                        
                        NS_CopyUnicodeToNative( title, tmpNativeStr );
                        outpt.Append( tmpNativeStr );
                        
                        outpt.Append( NS_LITERAL_CSTRING( "\",\"\"" ));

                        
                        
                        
                        
                        result = CreateDDEData( (LPBYTE)(const char*)outpt.get(),
                                                outpt.Length() + 1 );
#if MOZ_DEBUG_DDE
                        printf( "WWW_GetWindowInfo->%s\n", outpt.get() );
#endif
                    } while ( false );
                    break;
                }
                case topicActivate: {
                    
                    nsAutoString windowID;
                    ParseDDEArg(hsz2, 0, windowID);
                    
                    
                    if ( windowID.EqualsLiteral( "-1" ) ||
                         windowID.EqualsLiteral( "4294967295" ) ) {
                        
                        ActivateLastWindow();
                        
                        result = CreateDDEData( 1 );
                    }
                    break;
                }
                case topicVersion: {
                    
                    DWORD version = 1 << 16; 
                    result = CreateDDEData( version );
                    break;
                }
                case topicRegisterViewer: {
                    
                    result = CreateDDEData( false );
                    break;
                }
                case topicUnRegisterViewer: {
                    
                    result = CreateDDEData( false );
                    break;
                }
                default:
                    break;
            }
        } else if ( uType & XTYP_POKE ) {
            switch ( FindTopic( hsz1 ) ) {
                case topicCancelProgress: {
                    
                    result = (HDDEDATA)DDE_FACK;
                    break;
                }
                default:
                    break;
            }
        }
    } else if ( uType & XCLASS_FLAGS ) {
        if ( uType == XTYP_EXECUTE ) {
            
            DWORD bytes;
            LPBYTE request = DdeAccessData( hdata, &bytes );
#if MOZ_DEBUG_DDE
            printf( "Handling dde request: [%s]...\n", (char*)request );
#endif

            nsAutoString url;
            ParseDDEArg((const WCHAR*) request, 0, url);

            
            
            nsAutoString windowID;
            ParseDDEArg((const WCHAR*) request, 2, windowID);

            
            if ( windowID.IsEmpty() ) {
                url.Insert(NS_LITERAL_STRING("mozilla -new-window "), 0);
            }
            else {
                url.Insert(NS_LITERAL_STRING("mozilla -url "), 0);
            }
#if MOZ_DEBUG_DDE
            printf( "Handling dde XTYP_REQUEST request: [%s]...\n", NS_ConvertUTF16toUTF8(url).get() );
#endif
            
            HandleCommandLine(NS_ConvertUTF16toUTF8(url).get(), nullptr, nsICommandLine::STATE_REMOTE_EXPLICIT);

            
            DdeUnaccessData( hdata );
            result = (HDDEDATA)DDE_FACK;
        } else {
            result = (HDDEDATA)DDE_FNOTPROCESSED;
        }
    } else if ( uType & XCLASS_NOTIFICATION ) {
    }
#if MOZ_DEBUG_DDE
    printf( "DDE result=%d (0x%08X)\n", (int)result, (int)result );
#endif
    return result;
}







static int32_t advanceToEndOfQuotedArg( const WCHAR *p, int32_t offset, int32_t len ) {
    
    if ( p[++offset] == '"' ) {
        
        while ( offset < len && p[++offset] != '"' ) {
            
            
            if ( p[offset] == '\\' ) {
                offset++;
            }
        }
    }
    return offset;
}

void nsNativeAppSupportWin::ParseDDEArg( const WCHAR* args, int index, nsString& aString) {
    if ( args ) {
        nsDependentString temp(args);

        
        int32_t offset = -1;
        
        while( index-- ) {
            
            offset = advanceToEndOfQuotedArg( args, offset, temp.Length());
            
            offset = temp.FindChar( ',', offset );
            if ( offset == kNotFound ) {
                
                aString = args;
                return;
            }
        }
        
        
        
        
        
        
        
        
        int32_t end = advanceToEndOfQuotedArg( args, offset++, temp.Length() );
        
        end = temp.FindChar( ',', end );
        if ( end == kNotFound ) {
            
            end = temp.Length();
        }
        
        aString.Assign( args + offset, end - offset );
    }
    return;
}


void nsNativeAppSupportWin::ParseDDEArg( HSZ args, int index, nsString& aString) {
    DWORD argLen = DdeQueryStringW( mInstance, args, nullptr, 0, CP_WINUNICODE );
    
    if ( !argLen ) return;
    nsAutoString temp;
    
    temp.SetLength( argLen );
    
    DdeQueryString( mInstance, args, reinterpret_cast<wchar_t*>(temp.BeginWriting()), temp.Length(), CP_WINUNICODE );
    
    ParseDDEArg(temp.get(), index, aString);
    return;
}

HDDEDATA nsNativeAppSupportWin::CreateDDEData( DWORD value ) {
    return CreateDDEData( (LPBYTE)&value, sizeof value );
}

HDDEDATA nsNativeAppSupportWin::CreateDDEData( LPBYTE value, DWORD len ) {
    HDDEDATA result = DdeCreateDataHandle( mInstance,
                                           value,
                                           len,
                                           0,
                                           mApplication,
                                           CF_TEXT,
                                           0 );
    return result;
}

void nsNativeAppSupportWin::ActivateLastWindow() {
    nsCOMPtr<nsIDOMWindow> navWin;
    GetMostRecentWindow( MOZ_UTF16("navigator:browser"), getter_AddRefs( navWin ) );
    if ( navWin ) {
        
        activateWindow( navWin );
    } else {
        
        OpenBrowserWindow();
    }
}

void
nsNativeAppSupportWin::HandleCommandLine(const char* aCmdLineString,
                                         nsIFile* aWorkingDir,
                                         uint32_t aState)
{
    nsresult rv;

    int justCounting = 1;
    char **argv = 0;
    
    int init = 1;
    int between, quoted, bSlashCount;
    int argc;
    const char *p;
    nsAutoCString arg;

    nsCOMPtr<nsICommandLineRunner> cmdLine
        (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
    if (!cmdLine) {
        NS_ERROR("Couldn't create command line!");
        return;
    }

    
    
    
    
    while ( 1 ) {
        
        if ( init ) {
            p = aCmdLineString;
            between = 1;
            argc = quoted = bSlashCount = 0;

            init = 0;
        }
        if ( between ) {
            
            
            if (  *p != 0 && !isspace( *p ) ) {
                
                between = 0;
                arg = "";
                switch ( *p ) {
                    case '\\':
                        
                        bSlashCount = 1;
                        break;
                    case '"':
                        
                        quoted = 1;
                        break;
                    default:
                        
                        arg += *p;
                        break;
                }
            } else {
                
            }
        } else {
            
            
            if ( *p == 0 || ( !quoted && isspace( *p ) ) ) {
                
                
                while( bSlashCount ) {
                    arg += '\\';
                    bSlashCount--;
                }
                
                if ( !justCounting ) {
                    argv[argc] = new char[ arg.Length() + 1 ];
                    strcpy( argv[argc], arg.get() );
                }
                argc++;
                
                between = 1;
            } else {
                
                switch ( *p ) {
                    case '"':
                        
                        while ( bSlashCount > 1 ) {
                            
                            arg += '\\';
                            bSlashCount -= 2;
                        }
                        if ( bSlashCount ) {
                            
                            arg += '"';
                            bSlashCount = 0;
                        } else {
                            
                            if ( quoted ) {
                                
                                
                                if ( *(p+1) == '"' ) {
                                    
                                    
                                    
                                    bSlashCount = 1;
                                } else {
                                    quoted = 0;
                                }
                            } else {
                                quoted = 1;
                            }
                        }
                        break;
                    case '\\':
                        
                        bSlashCount++;
                        break;
                    default:
                        
                        while ( bSlashCount ) {
                            arg += '\\';
                            bSlashCount--;
                        }
                        
                        arg += *p;
                        break;
                }
            }
        }
        
        if ( *p ) {
            
            p++;
        } else {
            
            if ( justCounting ) {
                
                argv = new char*[ argc ];

                
                justCounting = 0;
                init = 1;
            } else {
                
                break;
            }
        }
    }

    rv = cmdLine->Init(argc, argv, aWorkingDir, aState);

    
    while ( argc ) {
        delete [] argv[ --argc ];
    }
    delete [] argv;

    if (NS_FAILED(rv)) {
        NS_ERROR("Error initializing command line.");
        return;
    }

    cmdLine->Run();
}

nsresult
nsNativeAppSupportWin::OpenWindow( const char*urlstr, const char *args ) {

  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsISupportsCString> sarg(do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID));
  if (sarg)
    sarg->SetData(nsDependentCString(args));

  if (wwatch && sarg) {
    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = wwatch->OpenWindow(0, urlstr, "_blank", "chrome,dialog=no,all",
                   sarg, getter_AddRefs(newWindow));
#if MOZ_DEBUG_DDE
  } else {
      printf("Get WindowWatcher (or create string) failed\n");
#endif
  }
  return rv;
}

HWND hwndForDOMWindow( nsISupports *window ) {
    nsCOMPtr<nsPIDOMWindow> pidomwindow( do_QueryInterface(window) );
    if ( !pidomwindow ) {
        return 0;
    }

    nsCOMPtr<nsIBaseWindow> ppBaseWindow =
        do_QueryInterface( pidomwindow->GetDocShell() );
    if ( !ppBaseWindow ) {
        return 0;
    }

    nsCOMPtr<nsIWidget> ppWidget;
    ppBaseWindow->GetMainWidget( getter_AddRefs( ppWidget ) );

    return (HWND)( ppWidget->GetNativeData( NS_NATIVE_WIDGET ) );
}

nsresult
nsNativeAppSupportWin::OpenBrowserWindow()
{
    nsresult rv = NS_OK;

    
    

    
    

    nsCOMPtr<nsIDOMWindow> navWin;
    GetMostRecentWindow( NS_LITERAL_STRING( "navigator:browser" ).get(), getter_AddRefs( navWin ) );

    
    
    do {
        
        if ( !navWin ) {
            
            break;
        }

        nsCOMPtr<nsIBrowserDOMWindow> bwin;
        { 
          nsCOMPtr<nsIWebNavigation> navNav( do_GetInterface( navWin ) );
          nsCOMPtr<nsIDocShellTreeItem> navItem( do_QueryInterface( navNav ) );
          if ( navItem ) {
            nsCOMPtr<nsIDocShellTreeItem> rootItem;
            navItem->GetRootTreeItem( getter_AddRefs( rootItem ) );
            nsCOMPtr<nsIDOMWindow> rootWin =
              rootItem ? rootItem->GetWindow() : nullptr;
            nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(rootWin));
            if ( chromeWin )
              chromeWin->GetBrowserDOMWindow( getter_AddRefs ( bwin ) );
          }
        }
        if ( bwin ) {
          nsCOMPtr<nsIURI> uri;
          NS_NewURI( getter_AddRefs( uri ), NS_LITERAL_CSTRING("about:blank"), 0, 0 );
          if ( uri ) {
            nsCOMPtr<nsIDOMWindow> container;
            rv = bwin->OpenURI( uri, 0,
                                nsIBrowserDOMWindow::OPEN_DEFAULTWINDOW,
                                nsIBrowserDOMWindow::OPEN_EXTERNAL,
                                getter_AddRefs( container ) );
            if ( NS_SUCCEEDED( rv ) )
              return NS_OK;
          }
        }

        NS_ERROR("failed to hand off external URL to extant window");
    } while ( false );

    

    char* argv[] = { 0 };
    nsCOMPtr<nsICommandLineRunner> cmdLine
        (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
    NS_ENSURE_TRUE(cmdLine, NS_ERROR_FAILURE);

    rv = cmdLine->Init(0, argv, nullptr, nsICommandLine::STATE_REMOTE_EXPLICIT);
    NS_ENSURE_SUCCESS(rv, rv);

    return cmdLine->Run();
}

