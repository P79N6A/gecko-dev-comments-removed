






































#include "nsStringSupport.h"


#include "nsIStringBundle.h"

#include "nsNativeAppSupportBase.h"
#include "nsNativeAppSupportWin.h"
#include "nsICmdLineService.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICmdLineHandler.h"
#include "nsIDOMWindow.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsISupportsArray.h"
#include "nsIWindowWatcher.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIAppStartup.h"
#include "nsIProfileInternal.h"
#include "nsIXULWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIWindowsHooks.h"
#include "nsIPromptService.h"
#include "nsNetCID.h"
#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsIObserverService.h"
#include "nsXPCOM.h"
#include "nsXPFEComponentsCID.h"
#include "nsEmbedCID.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"

struct JSContext;


#include "nsIDOMLocation.h"
#include "nsIJSContextStack.h"
#include "nsIWindowMediator.h"

#include <windows.h>
#include <shellapi.h>
#include <ddeml.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#define TURBO_NAVIGATOR 1
#define TURBO_MAIL 2
#define TURBO_EDITOR 3
#define TURBO_ADDRESSBOOK 4
#define TURBO_DISABLE 5
#define TURBO_EXIT 6

#define MAPI_STARTUP_ARG       "/MAPIStartUp"

#ifndef LWA_ALPHA
#define LWA_ALPHA 2
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x80000
#endif

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

#define REG_SUCCEEDED(val) (val == ERROR_SUCCESS)

static HWND hwndForDOMWindow( nsISupports * );

static
nsresult
GetMostRecentWindow(const PRUnichar* aType, nsIDOMWindowInternal** aWindow) {
    nsresult rv;
    nsCOMPtr<nsIWindowMediator> med( do_GetService( NS_WINDOWMEDIATOR_CONTRACTID, &rv ) );
    if ( NS_FAILED( rv ) )
        return rv;

    if ( med )
        return med->GetMostRecentWindow( aType, aWindow );

    return NS_ERROR_FAILURE;
}

static char* GetACPString(const nsString& aStr)
{
    int acplen = aStr.Length() * 2 + 1;
    char * acp = new char[ acplen ];
    if( acp ) {
        int outlen = ::WideCharToMultiByte( CP_ACP, 0, aStr.get(), aStr.Length(),
                                            acp, acplen-1, NULL, NULL );
        acp[ outlen ] = '\0';  
    }
    return acp;
}

static
void
activateWindow( nsIDOMWindowInternal *win ) {
    
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

typedef BOOL (WINAPI *MOZ_SetLayeredWindowAttributesProc)(HWND, COLORREF, BYTE, DWORD);

class nsSplashScreenWin : public nsISplashScreen {
public:
    nsSplashScreenWin();
    ~nsSplashScreenWin();

    NS_IMETHOD Show();
    NS_IMETHOD Hide();

    
    NS_IMETHOD_(nsrefcnt) AddRef() {
        mRefCnt++;
        return mRefCnt;
    }
    NS_IMETHOD_(nsrefcnt) Release() {
        --mRefCnt;
        if ( !mRefCnt ) {
            delete this;
            return 0;
        }
        return mRefCnt;
    }
    NS_IMETHOD QueryInterface( const nsIID &iid, void**p ) {
        nsresult rv = NS_OK;
        if ( p ) {
            *p = 0;
            if ( iid.Equals( NS_GET_IID( nsISplashScreen ) ) ) {
                nsISplashScreen *result = this;
                *p = result;
                NS_ADDREF( result );
            } else if ( iid.Equals( NS_GET_IID( nsISupports ) ) ) {
                nsISupports *result = NS_STATIC_CAST( nsISupports*, this );
                *p = result;
                NS_ADDREF( result );
            } else {
                rv = NS_NOINTERFACE;
            }
        } else {
            rv = NS_ERROR_NULL_POINTER;
        }
        return rv;
    }

    void SetDialog( HWND dlg );
    void LoadBitmap();
    static nsSplashScreenWin* GetPointer( HWND dlg );

    static BOOL CALLBACK DialogProc( HWND dlg, UINT msg, WPARAM wp, LPARAM lp );
    static DWORD WINAPI ThreadProc( LPVOID );
    static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

    HWND mDlg;
    HBITMAP mBitmap;
    nsrefcnt mRefCnt;

    static int mOpacity;
    static MOZ_SetLayeredWindowAttributesProc mSetLayeredWindowAttributesProc;

}; 


struct Mutex {
    Mutex( const char *name )
        : mName( name ),
          mHandle( 0 ),
          mState( -1 ) {
        mHandle = CreateMutex( 0, FALSE, mName.get() );
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
    nsCString mName;
    HANDLE    mHandle;
    DWORD     mState;
};




















































































class nsNativeAppSupportWin : public nsNativeAppSupportBase {
public:
    
    NS_IMETHOD Start( PRBool *aResult );
    NS_IMETHOD Stop( PRBool *aResult );
    NS_IMETHOD Quit();
    NS_IMETHOD StartServerMode();
    NS_IMETHOD OnLastWindowClosing();
    NS_IMETHOD SetIsServerMode( PRBool isServerMode );
    NS_IMETHOD EnsureProfile(nsICmdLineService* args);

    
    NS_IMETHOD StartDDE();

    
    
    
    void CheckConsole();

private:
    static HDDEDATA CALLBACK HandleDDENotification( UINT     uType,
                                                    UINT     uFmt,
                                                    HCONV    hconv,
                                                    HSZ      hsz1,
                                                    HSZ      hsz2,
                                                    HDDEDATA hdata,
                                                    ULONG    dwData1,
                                                    ULONG    dwData2 );
    static nsresult HandleRequest( LPBYTE request, PRBool newWindow, nsIDOMWindow **aResult );
    static void ParseDDEArg( HSZ args, int index, nsCString& string);
    static void ParseDDEArg( const char* args, int index, nsCString& aString);
    static void ActivateLastWindow();
    static HDDEDATA CreateDDEData( DWORD value );
    static HDDEDATA CreateDDEData( LPBYTE value, DWORD len );
    static PRBool   InitTopicStrings();
    static int      FindTopic( HSZ topic );
    static nsresult GetCmdLineArgs( LPBYTE request, nsICmdLineService **aResult );
    static nsresult OpenWindow( const char *urlstr,
                                const nsAString& aArgs,
                                nsIDOMWindow **aResult );
    static nsresult OpenBrowserWindow( const nsAString& aArgs,
                                       PRBool newWindow,
                                       nsIDOMWindow **aResult );
    static nsresult ReParent( nsISupports *window, HWND newParent );
    static nsresult GetStartupURL(nsICmdLineService *args, nsCString& taskURL);
    static void     SetupSysTrayIcon();
    static void     RemoveSysTrayIcon();

    static UINT mTrayRestart;

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
    static NOTIFYICONDATA mIconData;
    static HMENU          mTrayIconMenu;

    static HSZ   mApplication, mTopics[ topicCount ];
    static DWORD mInstance;
    static char *mAppName;
    static PRBool mInitialWindowActive;
    static PRBool mForceProfileStartup;
    static PRBool mSupportingDDEExec;
    static char mMutexName[];
    friend struct MessageWindow;
}; 

nsSplashScreenWin::nsSplashScreenWin()
    : mDlg( 0 ), mBitmap( 0 ), mRefCnt( 0 ) {
}

nsSplashScreenWin::~nsSplashScreenWin() {
#if MOZ_DEBUG_DDE
    printf( "splash screen dtor called\n" );
#endif
    KillTimer(mDlg, 0);
    
    Hide();
}

NS_IMETHODIMP
nsSplashScreenWin::Show() {
    
    DWORD threadID = 0;
    HANDLE handle = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)ThreadProc, this, 0, &threadID );
    CloseHandle(handle);

    return NS_OK;
}

NS_IMETHODIMP
nsSplashScreenWin::Hide() {
    if ( mDlg ) {
        
        
        
        
        
        
        
        
        
        
        
        
        nsCOMPtr<nsIDOMWindowInternal> topLevel;
        GetMostRecentWindow(nsnull, getter_AddRefs( topLevel ) );
        HWND hWndTopLevel = topLevel ? hwndForDOMWindow(topLevel) : 0;
        
        ::PostMessage(mDlg, WM_CLOSE, (WPARAM)mBitmap, (LPARAM)hWndTopLevel);
        mBitmap = 0;
        mDlg = 0;
    }
    return NS_OK;
}

void
nsSplashScreenWin::LoadBitmap() {
    
    char fileName[ _MAX_PATH ];
    int fileNameLen = ::GetModuleFileName( NULL, fileName, sizeof fileName );
    if ( fileNameLen >= 3 ) {
        fileName[ fileNameLen - 3 ] = 0;
        strcat( fileName, "bmp" );
        
        HBITMAP bitmap = (HBITMAP)::LoadImage( NULL,
                                               fileName,
                                               IMAGE_BITMAP,
                                               0,
                                               0,
                                               LR_LOADFROMFILE );
        if ( bitmap ) {
            HWND bitmapControl = GetDlgItem( mDlg, IDB_SPLASH );
            if ( bitmapControl ) {
                HBITMAP old = (HBITMAP)SendMessage( bitmapControl,
                                                    STM_SETIMAGE,
                                                    IMAGE_BITMAP,
                                                    (LPARAM)bitmap );
                
                mBitmap = bitmap;
                
                if ( old ) {
                    BOOL ok = DeleteObject( old );
                }
            } else {
                
                DeleteObject( bitmap );
            }
        }
    }
}

int nsSplashScreenWin::mOpacity = 55;
MOZ_SetLayeredWindowAttributesProc nsSplashScreenWin::mSetLayeredWindowAttributesProc = 0;

VOID CALLBACK
nsSplashScreenWin::TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
    mOpacity += 20;
    if (mOpacity >= 255) { 
        mOpacity = 255;
        KillTimer(hwnd, 0);
    }
    
    mSetLayeredWindowAttributesProc(hwnd, 0, mOpacity, LWA_ALPHA);
}

BOOL CALLBACK
nsSplashScreenWin::DialogProc( HWND dlg, UINT msg, WPARAM wp, LPARAM lp ) {
    if ( msg == WM_INITDIALOG ) {
        
        nsSplashScreenWin *splashScreen = (nsSplashScreenWin*)lp;
        if ( lp ) {
            splashScreen->SetDialog( dlg );

            HMODULE user32lib = GetModuleHandle("user32.dll");
            mSetLayeredWindowAttributesProc = (MOZ_SetLayeredWindowAttributesProc) GetProcAddress(user32lib, "SetLayeredWindowAttributes");

            
            if (mSetLayeredWindowAttributesProc && !GetSystemMetrics(SM_REMOTESESSION)) {
                SetWindowLong(dlg, GWL_EXSTYLE,
                                 GetWindowLong(dlg, GWL_EXSTYLE) | WS_EX_LAYERED);
                mSetLayeredWindowAttributesProc(dlg, 0,
                                                  mOpacity, LWA_ALPHA);
                SetTimer(dlg, 0, 10, TimerProc);
            }

            
            splashScreen->LoadBitmap();
        }

        



        HWND bitmapControl = GetDlgItem( dlg, IDB_SPLASH );
        if ( bitmapControl ) {
            HBITMAP hbitmap = (HBITMAP)SendMessage( bitmapControl,
                                                    STM_GETIMAGE,
                                                    IMAGE_BITMAP,
                                                    0 );
            if ( hbitmap ) {
                BITMAP bitmap;
                if ( GetObject( hbitmap, sizeof bitmap, &bitmap ) ) {
                    SetWindowPos( dlg,
                                  NULL,
                                  GetSystemMetrics(SM_CXSCREEN)/2 - bitmap.bmWidth/2,
                                  GetSystemMetrics(SM_CYSCREEN)/2 - bitmap.bmHeight/2,
                                  bitmap.bmWidth,
                                  bitmap.bmHeight,
                                  SWP_NOZORDER );
                    ShowWindow( dlg, SW_SHOW );
                }
            }
        }
        return 1;
    } else if (msg == WM_CLOSE) {
        
        
        HWND topLevel = (HWND)lp;
        if (topLevel)
            ::SetForegroundWindow(topLevel);
        
        ::EndDialog(dlg, 0);
        
        HBITMAP bitmap = (HBITMAP)wp;
        if ( bitmap ) {
            ::DeleteObject( bitmap );
        }
    }
    return 0;
}

void nsSplashScreenWin::SetDialog( HWND dlg ) {
    
    mDlg = dlg;
    
    SetWindowLong( mDlg, DWL_USER, (LONG)(void*)this );
}

nsSplashScreenWin *nsSplashScreenWin::GetPointer( HWND dlg ) {
    
    LONG data = GetWindowLong( dlg, DWL_USER );
    return (nsSplashScreenWin*)(void*)data;
}

DWORD WINAPI nsSplashScreenWin::ThreadProc( LPVOID splashScreen ) {
    DialogBoxParam( GetModuleHandle( 0 ),
                    MAKEINTRESOURCE( IDD_SPLASH ),
                    HWND_DESKTOP,
                    (DLGPROC)DialogProc,
                    (LPARAM)splashScreen );
    return 0;
}

PRBool gAbortServer = PR_FALSE;

void
nsNativeAppSupportWin::CheckConsole() {
    for ( int i = 1; i < __argc; i++ ) {
        if ( strcmp( "-console", __argv[i] ) == 0
             ||
             strcmp( "/console", __argv[i] ) == 0 ) {
            
            
            BOOL rc = ::AllocConsole();
            if ( rc ) {
                
                

                
                int hCrt = ::_open_osfhandle( (long)GetStdHandle( STD_OUTPUT_HANDLE ),
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

                
                hCrt = ::_open_osfhandle( (long)::GetStdHandle( STD_ERROR_HANDLE ),
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
            
            break;
        } else if ( strcmp( "-turbo", __argv[i] ) == 0
                    ||
                    strcmp( "/turbo", __argv[i] ) == 0
                    ||
                    strcmp( "-server", __argv[i] ) == 0
                    ||
                    strcmp( "/server", __argv[i] ) == 0 ) {
            
            mServerMode = PR_TRUE;
            mShouldShowUI = PR_FALSE;
            __argv[i] = "-nosplash"; 
            
            break;
        }
    }

    PRBool checkTurbo = PR_TRUE;
    for ( int j = 1; j < __argc; j++ ) {
        if (strcmp("-killAll", __argv[j]) == 0 || strcmp("/killAll", __argv[j]) == 0 ||
            strcmp("-kill", __argv[j]) == 0 || strcmp("/kill", __argv[j]) == 0) {
            gAbortServer = PR_TRUE;
            break;
        }

        if ( strcmp( "-silent", __argv[j] ) == 0 || strcmp( "/silent", __argv[j] ) == 0 ) {
            checkTurbo = PR_FALSE;
        }
    }

    
    
    if ( checkTurbo && !mServerMode ) {
        HKEY key;
        LONG result = ::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &key );
        if ( result == ERROR_SUCCESS ) {
          BYTE regvalue[_MAX_PATH];
          DWORD type, len = sizeof(regvalue);
          result = ::RegQueryValueEx( key, NS_QUICKLAUNCH_RUN_KEY, NULL, &type, regvalue, &len);
          ::RegCloseKey( key );
          if ( result == ERROR_SUCCESS && len > 0 ) {
              
              char fileName[_MAX_PATH];
              int rv = ::GetModuleFileName( NULL, fileName, sizeof fileName );
              nsCAutoString regvalueholder;
              regvalueholder.Assign((char *) regvalue);
              if ((FindInString(regvalueholder, fileName, PR_TRUE) != kNotFound) &&
                  (FindInString(regvalueholder, "-turbo", PR_TRUE) != kNotFound) ) {
                  mServerMode = PR_TRUE;
                  mShouldShowUI = PR_TRUE;
              }
          }
        }
    }

    return;
}



nsresult
NS_CreateNativeAppSupport( nsINativeAppSupport **aResult ) {
    if ( aResult ) {
        nsNativeAppSupportWin *pNative = new nsNativeAppSupportWin;
        if ( pNative ) {
            *aResult = pNative;
            NS_ADDREF( *aResult );
            
            pNative->CheckConsole();
        } else {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        return NS_ERROR_NULL_POINTER;
    }

    return NS_OK;
}


nsresult
NS_CreateSplashScreen( nsISplashScreen **aResult ) {
    if ( aResult ) {
        *aResult = 0;
        for ( int i = 1; i < __argc; i++ ) {
            if ( strcmp( "-quiet", __argv[i] ) == 0
                 ||
                 strcmp( "/quiet", __argv[i] ) == 0 ) {
                
                return NS_OK;
            }
        }
        *aResult = new nsSplashScreenWin;
        if ( *aResult ) {
            NS_ADDREF( *aResult );
        } else {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        return NS_ERROR_NULL_POINTER;
    }

    return NS_OK;
}


#define MOZ_STARTUP_MUTEX_NAME "StartupMutex"
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
PRBool nsNativeAppSupportWin::mInitialWindowActive = PR_FALSE;
PRBool nsNativeAppSupportWin::mForceProfileStartup = PR_FALSE;
PRBool nsNativeAppSupportWin::mSupportingDDEExec   = PR_FALSE;

NOTIFYICONDATA nsNativeAppSupportWin::mIconData = { sizeof(NOTIFYICONDATA),
                                                    0,
                                                    1,
                                                    NIF_ICON | NIF_MESSAGE | NIF_TIP,
                                                    WM_USER,
                                                    0,
                                                    0 };
HMENU nsNativeAppSupportWin::mTrayIconMenu = 0;

char nsNativeAppSupportWin::mMutexName[ 128 ] = { 0 };



struct MessageWindow {
    
    MessageWindow() {
        
        mHandle = ::FindWindow( className(), 0 );
    }

    
    operator HWND() {
        return mHandle;
    }

    
    static const char *className() {
        static char classNameBuffer[128];
        static char *mClassName = 0;
        if ( !mClassName ) {
            ::_snprintf( classNameBuffer,
                         sizeof classNameBuffer,
                         "%s%s",
                         nsNativeAppSupportWin::mAppName,
                         "MessageWindow" );
            mClassName = classNameBuffer;
        }
        return mClassName;
    }

    
    NS_IMETHOD Create() {
        WNDCLASS classStruct = { 0,                          
                                 &MessageWindow::WindowProc, 
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 0,                          
                                 className() };              

        
        NS_ENSURE_TRUE( ::RegisterClass( &classStruct ), NS_ERROR_FAILURE );

        
        NS_ENSURE_TRUE( ( mHandle = ::CreateWindow( className(),
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
                mHandle = NULL;
            }
            else {
                retval = NS_ERROR_FAILURE;
            }
        }

        return retval;
    }

    
    NS_IMETHOD SendRequest( const char *cmd ) {
        COPYDATASTRUCT cds = { 0, ::strlen( cmd ) + 1, (void*)cmd };
        
        
        ::SetForegroundWindow( mHandle );
        ::SendMessage( mHandle, WM_COPYDATA, 0, (LPARAM)&cds );
        return NS_OK;
    }

    
    static long CALLBACK WindowProc( HWND msgWindow, UINT msg, WPARAM wp, LPARAM lp ) {
        if ( msg == WM_COPYDATA ) {
            
            COPYDATASTRUCT *cds = (COPYDATASTRUCT*)lp;
#if MOZ_DEBUG_DDE
            printf( "Incoming request: %s\n", (const char*)cds->lpData );
#endif
            
            nsCOMPtr<nsIDOMWindow> win;
            (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)cds->lpData, PR_FALSE, getter_AddRefs( win ) );
            return win ? (long)hwndForDOMWindow( win ) : 0;
#ifndef MOZ_PHOENIX
 } else if ( msg == WM_USER ) {
     if ( lp == WM_RBUTTONUP ) {
         
         nsCOMPtr<nsIDOMWindowInternal> win;
         GetMostRecentWindow( 0, getter_AddRefs( win ) );
         ::EnableMenuItem( nsNativeAppSupportWin::mTrayIconMenu, TURBO_EXIT, win ? MF_GRAYED : MF_ENABLED );
         POINT pt;
         GetCursorPos( &pt );

         SetForegroundWindow(msgWindow);
         int selectedItem = ::TrackPopupMenu( nsNativeAppSupportWin::mTrayIconMenu,
                                              TPM_NONOTIFY | TPM_RETURNCMD |
                                              TPM_RIGHTBUTTON,
                                              pt.x,
                                              pt.y,
                                              0,
                                              msgWindow,
                                              0 );

         nsCOMPtr<nsIDOMWindow> newWin;
         switch (selectedItem) {
         case TURBO_NAVIGATOR:
             (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)(NS_STRINGIFY(MOZ_APP_NAME) " -browser"), PR_TRUE, getter_AddRefs( newWin ) );
             break;
         case TURBO_MAIL:
             (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)(NS_STRINGIFY(MOZ_APP_NAME) " -mail"), PR_TRUE, getter_AddRefs( newWin ) );
              break;
         case TURBO_EDITOR:
             (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)(NS_STRINGIFY(MOZ_APP_NAME) " -editor"), PR_TRUE, getter_AddRefs( newWin ) );
             break;
         case TURBO_ADDRESSBOOK:
             (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)(NS_STRINGIFY(MOZ_APP_NAME) " -addressbook"), PR_TRUE, getter_AddRefs( newWin ) );
             break;
         case TURBO_EXIT:
             (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)(NS_STRINGIFY(MOZ_APP_NAME) " -kill"), PR_TRUE, getter_AddRefs( newWin ) );
             break;
         case TURBO_DISABLE:
             nsresult rv;
             nsCOMPtr<nsIStringBundleService> stringBundleService( do_GetService( NS_STRINGBUNDLE_CONTRACTID ) );
             nsCOMPtr<nsIStringBundle> turboMenuBundle;
             nsCOMPtr<nsIStringBundle> brandBundle;
             if ( stringBundleService ) {
                 stringBundleService->CreateBundle( "chrome://branding/locale/brand.properties", getter_AddRefs( brandBundle ) );
                 stringBundleService->CreateBundle( "chrome://navigator/locale/turboMenu.properties",
                                                    getter_AddRefs( turboMenuBundle ) );
             }
             nsXPIDLString dialogMsg;
             nsXPIDLString dialogTitle;
             nsXPIDLString brandName;
             if ( brandBundle && turboMenuBundle ) {
                 brandBundle->GetStringFromName( NS_LITERAL_STRING( "brandShortName" ).get(),
                                                 getter_Copies( brandName ) );
                 const PRUnichar *formatStrings[] = { brandName.get() };
                 turboMenuBundle->FormatStringFromName( NS_LITERAL_STRING( "DisableDlgMsg" ).get(), formatStrings,
                                                        1, getter_Copies( dialogMsg ) );
                 turboMenuBundle->FormatStringFromName( NS_LITERAL_STRING( "DisableDlgTitle" ).get(), formatStrings,
                                                        1, getter_Copies( dialogTitle ) );

             }
             if ( !dialogMsg.IsEmpty() && !dialogTitle.IsEmpty() && !brandName.IsEmpty() ) {
                 nsCOMPtr<nsIPromptService> dialog( do_GetService( NS_PROMPTSERVICE_CONTRACTID ) );
                 if ( dialog ) {
                     PRBool reallyDisable;
                     nsNativeAppSupportWin::mLastWindowIsConfirmation = PR_TRUE;
                     dialog->Confirm( nsnull, dialogTitle.get(), dialogMsg.get(), &reallyDisable );
                     if ( !reallyDisable ) {
                          break;
                     }
                 }

             }
             nsCOMPtr<nsIWindowsHooks> winHooksService ( do_GetService( NS_IWINDOWSHOOKS_CONTRACTID, &rv ) );
             if ( NS_SUCCEEDED( rv ) )
                 winHooksService->StartupRemoveOption("-turbo");

             nsCOMPtr<nsIAppStartup> appStartup
                 (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
             if ( NS_SUCCEEDED( rv ) ) {
                 nsCOMPtr<nsINativeAppSupport> native;
                 rv = appStartup->GetNativeAppSupport( getter_AddRefs( native ) );
                 if ( NS_SUCCEEDED( rv ) )
                     native->SetIsServerMode( PR_FALSE );
                 if ( !win )
                     appStartup->Quit(nsIAppStartup::eAttemptQuit);
             }
             break;
         }
         PostMessage(msgWindow, WM_NULL, 0, 0);
     } else if ( lp == WM_LBUTTONDBLCLK ) {
         
         
         
         nsCOMPtr<nsIDOMWindow> win;
         (void)nsNativeAppSupportWin::HandleRequest( (LPBYTE)NS_STRINGIFY(MOZ_APP_NAME), PR_TRUE, getter_AddRefs( win ) );
     }
     return TRUE;
#endif
  } else if ( msg == WM_QUERYENDSESSION ) {
    
    
    
    
    nsCOMPtr<nsICmdLineHandler>
        killAll( do_CreateInstance( "@mozilla.org/commandlinehandler/general-startup;1?type=killAll" ) );
    if ( killAll ) {
        nsXPIDLCString unused;
        
        
        
        
        nsresult rv = killAll->GetChromeUrlForTask( getter_Copies( unused ) );
        if ( rv == NS_ERROR_ABORT ) {
            
            return FALSE;
        } else {
            
            return TRUE;
        }
    }
  } else if ((nsNativeAppSupportWin::mTrayRestart) && (msg == nsNativeAppSupportWin::mTrayRestart)) {
     
     ::Shell_NotifyIcon( NIM_ADD, &nsNativeAppSupportWin::mIconData );
  }
  return DefWindowProc( msgWindow, msg, wp, lp );
}

private:
    HWND mHandle;
}; 

UINT nsNativeAppSupportWin::mTrayRestart = 0;
static char nameBuffer[128] = { 0 };
char *nsNativeAppSupportWin::mAppName = nameBuffer;













NS_IMETHODIMP
nsNativeAppSupportWin::Start( PRBool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    if (getenv("MOZ_NO_REMOTE"))
    {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    nsresult rv = NS_ERROR_FAILURE;
    *aResult = PR_FALSE;

    
    int retval;
    UINT id = ID_DDE_APPLICATION_NAME;
    retval = LoadString( (HINSTANCE) NULL, id, (LPTSTR) nameBuffer, sizeof(nameBuffer) );
    if ( retval == 0 ) {
        
        *aResult = PR_TRUE;
        return NS_OK;
    }

    
    ::_snprintf( mMutexName, sizeof mMutexName, "%s%s", nameBuffer, MOZ_STARTUP_MUTEX_NAME );
    Mutex startupLock = Mutex( mMutexName );

    NS_ENSURE_TRUE( startupLock.Lock( MOZ_DDE_START_TIMEOUT ), NS_ERROR_FAILURE );

    
    MessageWindow msgWindow;
    if ( (HWND)msgWindow ) {
        
        LPTSTR cmd = ::GetCommandLine();
        rv = msgWindow.SendRequest( cmd );
    } else {
        
        if (!gAbortServer) {
            rv = msgWindow.Create();
            if ( NS_SUCCEEDED( rv ) ) {
                
                this->StartDDE();
                
                *aResult = PR_TRUE;
            }
        }
    }

    startupLock.Unlock();

    return rv;
}

PRBool
nsNativeAppSupportWin::InitTopicStrings() {
    for ( int i = 0; i < topicCount; i++ ) {
        if ( !( mTopics[ i ] = DdeCreateStringHandle( mInstance, NS_CONST_CAST(char *,topicNames[ i ]), CP_WINANSI ) ) ) {
            return PR_FALSE;
        }
    }
    return PR_TRUE;
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


static PRBool handlingHTTP() {
    PRBool result = PR_FALSE; 
    
    nsCOMPtr<nsIWindowsHooks> winhooks( do_GetService( NS_IWINDOWSHOOKS_CONTRACTID ) );
    if ( winhooks ) {
        nsCOMPtr<nsIWindowsHooksSettings> settings;
        nsresult rv = winhooks->GetSettings( getter_AddRefs( settings ) );
        if ( NS_SUCCEEDED( rv ) ) {
            settings->GetIsHandlingHTTP( &result );
            if ( result ) {
                
                
                
                
                
                
                
                
                

                
                settings->SetIsHandlingHTTPS( PR_FALSE );
#ifndef MOZ_PHOENIX
                settings->SetIsHandlingFTP( PR_FALSE );
                settings->SetIsHandlingCHROME( PR_FALSE );
                settings->SetIsHandlingGOPHER( PR_FALSE );
#endif
                
                settings->SetIsHandlingHTML( PR_FALSE );
                settings->SetIsHandlingXHTML( PR_FALSE );
#ifndef MOZ_PHOENIX
                settings->SetIsHandlingJPEG( PR_FALSE );
                settings->SetIsHandlingGIF( PR_FALSE );
                settings->SetIsHandlingPNG( PR_FALSE );
                settings->SetIsHandlingBMP( PR_FALSE );
                settings->SetIsHandlingICO( PR_FALSE );
                settings->SetIsHandlingXML( PR_FALSE );
                settings->SetIsHandlingXUL( PR_FALSE );
#endif
                
                settings->GetRegistryMatches( &result );
            }
        }
    }
    return result;
}


static DWORD deleteKey( HKEY baseKey, const char *keyName ) {
    
    DWORD rc;
    if ( keyName && ::strlen(keyName) ) {
        
        HKEY key;
        rc = ::RegOpenKeyEx( baseKey,
                             keyName,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | DELETE,
                             &key );
        
        while ( rc == ERROR_SUCCESS ) {
            char subkeyName[_MAX_PATH];
            DWORD len = sizeof subkeyName;
            
            
            
            rc = ::RegEnumKeyEx( key,
                                 0,
                                 subkeyName,
                                 &len,
                                 0,
                                 0,
                                 0,
                                 0 );
            if ( rc == ERROR_NO_MORE_ITEMS ) {
                
                rc = ::RegDeleteKey( baseKey, keyName );
                break;
            } else if ( rc == ERROR_SUCCESS ) {
                
                rc = deleteKey( key, subkeyName );
            }
        }
        
        ::RegCloseKey( key );
    } else {
        rc = ERROR_BADKEY;
    }
    return rc;
}












NS_IMETHODIMP
nsNativeAppSupportWin::StartDDE() {
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    
    NS_ENSURE_TRUE( DMLERR_NO_ERROR == DdeInitialize( &mInstance,
                                                      nsNativeAppSupportWin::HandleDDENotification,
                                                      APPCLASS_STANDARD,
                                                      0 ),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( ( mApplication = DdeCreateStringHandle( mInstance, mAppName, CP_WINANSI ) ) && InitTopicStrings(),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( DdeNameService( mInstance, mApplication, 0, DNS_REGISTER ), NS_ERROR_FAILURE );

#if MOZ_DEBUG_DDE
    printf( "DDE server started\n" );
#endif

    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportWin::Stop( PRBool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance, NS_ERROR_NOT_INITIALIZED );

    nsresult rv = NS_OK;
    *aResult = PR_TRUE;

    Mutex ddeLock( mMutexName );

    if ( ddeLock.Lock( MOZ_DDE_STOP_TIMEOUT ) ) {
        if ( mConversations == 0 ) {
            this->Quit();
        } else {
            *aResult = PR_FALSE;
        }

        ddeLock.Unlock();
    }
    else {
        
        
        *aResult = PR_TRUE;
    }

    return rv;
}


NS_IMETHODIMP
nsNativeAppSupportWin::Quit() {
    
    
    
    
    Mutex mutexLock(mMutexName);
    NS_ENSURE_TRUE(mutexLock.Lock(MOZ_DDE_START_TIMEOUT), NS_ERROR_FAILURE );

    
    
    
    
    MessageWindow mw;
    mw.Destroy();

    if ( mInstance ) {
        
        if ( mSupportingDDEExec && handlingHTTP() ) {
            mSupportingDDEExec = PR_FALSE;
#if MOZ_DEBUG_DDE
            printf( "Deleting ddexec subkey on exit\n" );
#endif
            deleteKey( HKEY_CLASSES_ROOT, "http\\shell\\open\\ddeexec" );
        }

        
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
    }

    return NS_OK;
}

PRBool NS_CanRun()
{
      return PR_TRUE;
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
    DWORD len = DdeQueryString( instance, hsz, NULL, NULL, CP_WINANSI );
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



static void escapeQuotes( nsString &aString ) {
    PRInt32 offset = -1;
    while( 1 ) {
       
       offset = FindCharInString(aString, '"', ++offset );
       if ( offset == kNotFound ) {
           
           break;
       } else {
           
           aString.Insert( PRUnichar('\\'), offset );
           
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
                                              ULONG dwData1,    
                                              ULONG dwData2 ) { 

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
                    

                    
                    PRBool new_window = PR_FALSE;

                    
                    nsCAutoString url;
                    ParseDDEArg(hsz2, 0, url);
                    
                    
                    nsCAutoString windowID;
                    ParseDDEArg(hsz2, 2, windowID);
                    
                    if ( strcmp(windowID.get(), "0" ) == 0 ) {
                        new_window = PR_TRUE;
                    }

                    
                    url.Insert( NS_STRINGIFY(MOZ_APP_NAME) " -url ", 0 );
#if MOZ_DEBUG_DDE
                    printf( "Handling dde XTYP_REQUEST request: [%s]...\n", url.get() );
#endif
                    
                    nsCOMPtr<nsIDOMWindow> win;
                    HandleRequest( LPBYTE( url.get() ), new_window, getter_AddRefs( win ) );
                    
                    result = CreateDDEData( 1 );
                    break;
                }
                case topicGetWindowInfo: {
                    
                    
                    
                    
                    

                    
                    
                    
                    do {
                        
                        nsCOMPtr<nsIDOMWindowInternal> navWin;
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
                        
                        nsCOMPtr<nsIDOMWindowInternal> internalContent( do_QueryInterface( content ) );
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

                        
                        nsCOMPtr<nsPIDOMWindow> scrWin( do_QueryInterface( internalContent ) );
                        if ( !scrWin ) {
                            break;
                        }
                        
                        nsCOMPtr<nsIBaseWindow> baseWindow =
                            do_QueryInterface( scrWin->GetDocShell() );
                        if ( !baseWindow ) {
                            break;
                        }
                        
                        nsXPIDLString title;
                        if(!baseWindow) {
                            break;
                        }
                        baseWindow->GetTitle(getter_Copies(title));
                        
                        escapeQuotes( title );

                        
                        
                        nsCAutoString   outpt( NS_LITERAL_CSTRING("\"") );
                        
                        
                        nsCAutoString tmpNativeStr;
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
                    } while ( PR_FALSE );
                    break;
                }
                case topicActivate: {
                    
                    nsCAutoString windowID;
                    ParseDDEArg(hsz2, 0, windowID);
                    
                    
                    const char *wid = windowID.get();
                    if ( strcmp(wid, "-1" ) == 0 ||
                         strcmp(wid, "4294967295" ) == 0 ) {
                        
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
                    
                    result = CreateDDEData( PR_FALSE );
                    break;
                }
                case topicUnRegisterViewer: {
                    
                    result = CreateDDEData( PR_FALSE );
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
            
            PRBool new_window = PR_FALSE;

            nsCAutoString url;
            ParseDDEArg((const char*) request, 0, url);

            
            
            nsCAutoString windowID;
            ParseDDEArg((const char*) request, 2, windowID);

            
            if ( strcmp(windowID.get(), "0" ) == 0 ) {
                new_window = PR_TRUE;
            }

            
            url.Insert( NS_STRINGIFY(MOZ_APP_NAME) " -url ", 0 );
#if MOZ_DEBUG_DDE
            printf( "Handling dde XTYP_REQUEST request: [%s]...\n", url.get() );
#endif
            
            nsCOMPtr<nsIDOMWindow> win;
            HandleRequest( LPBYTE( url.get() ), new_window, getter_AddRefs( win ) );

            
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







static PRInt32 advanceToEndOfQuotedArg( const char *p, PRInt32 offset, PRInt32 len ) {
    
    if ( p[++offset] == '"' ) {
        
        while ( offset < len && p[++offset] != '"' ) {
            
            
            if ( p[offset] == '\\' ) {
                offset++;
            }
        }
    }
    return offset;
}

void nsNativeAppSupportWin::ParseDDEArg( const char* args, int index, nsCString& aString) {
    if ( args ) {
        int argLen = strlen(args);
        nsDependentCString temp(args, argLen);

        
        PRInt32 offset = -1;
        
        while( index-- ) {
            
            offset = advanceToEndOfQuotedArg( args, offset, argLen);
            
            offset = FindCharInString(temp, ',', offset );
            if ( offset == kNotFound ) {
                
                aString = args;
                return;
            }
        }
        
        
        
        
        
        
        
        
        PRInt32 end = advanceToEndOfQuotedArg( args, offset++, argLen );
        
        end = FindCharInString(temp, ',', end );
        if ( end == kNotFound ) {
            
            end = argLen;
        }
        
        aString.Assign( args + offset, end - offset );
    }
    return;
}


void nsNativeAppSupportWin::ParseDDEArg( HSZ args, int index, nsCString& aString) {
    DWORD argLen = DdeQueryString( mInstance, args, NULL, NULL, CP_WINANSI );
    
    if ( !argLen ) return;
    
    char *temp = (char *) malloc(argLen + 1);
    if ( !temp ) return;
    
    DdeQueryString( mInstance, args, temp, argLen + 1, CP_WINANSI );
    
    ParseDDEArg(temp, index, aString);
    free(temp);
}

void nsNativeAppSupportWin::ActivateLastWindow() {
    nsCOMPtr<nsIDOMWindowInternal> navWin;
    GetMostRecentWindow( NS_LITERAL_STRING("navigator:browser").get(), getter_AddRefs( navWin ) );
    if ( navWin ) {
        
        activateWindow( navWin );
    } else {
        
        nsCOMPtr<nsIDOMWindow> newWin;
        OpenBrowserWindow( NS_LITERAL_STRING( "about:blank" ),
                           PR_TRUE, getter_AddRefs( newWin ) );
    }
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







nsresult
nsNativeAppSupportWin::HandleRequest( LPBYTE request, PRBool newWindow, nsIDOMWindow **aResult ) {

    
    

    if (mInitialWindowActive) {
      return NS_ERROR_FAILURE;
    }

    

    nsCOMPtr<nsICmdLineService> args;
    nsresult rv;

    rv = GetCmdLineArgs( request, getter_AddRefs( args ) );
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIAppStartup> appStartup (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsINativeAppSupport> nativeApp;
    rv = appStartup->GetNativeAppSupport(getter_AddRefs( nativeApp ));
    if (NS_FAILED(rv)) return rv;

    
    nsXPIDLCString arg;
    rv = args->GetURLToLoad(getter_Copies(arg));
    if (NS_FAILED(rv)) return rv;

    if (!arg.IsEmpty() ) {
      
#if MOZ_DEBUG_DDE
      printf( "Launching browser on url [%s]...\n", arg.get() );
#endif
      rv = nativeApp->EnsureProfile(args);
      if (NS_SUCCEEDED(rv)) {
        nsAutoString tmpArg;
        NS_CopyNativeToUnicode( arg, tmpArg );
        rv = OpenBrowserWindow( tmpArg, newWindow, aResult );
      }
      return rv;
    }


    
    rv = args->GetCmdLineValue("-chrome", getter_Copies(arg));
    if (NS_SUCCEEDED(rv) && !arg.IsEmpty() ) {
      
#if MOZ_DEBUG_DDE
      printf( "Launching chrome url [%s]...\n", arg.get() );
#endif
      rv = nativeApp->EnsureProfile(args);
      if (NS_SUCCEEDED(rv))
        rv = OpenWindow( arg.get(), EmptyString(), aResult );
      return rv;
    }

    
    

    rv = args->GetCmdLineValue( "-profilemanager", getter_Copies(arg));
    if ( NS_SUCCEEDED(rv) && !arg.IsEmpty() ) { 
      nsCOMPtr<nsIDOMWindowInternal> window;
      GetMostRecentWindow(0, getter_AddRefs(window));
      if (!window) { 
        mForceProfileStartup = PR_TRUE;
      }
    }

    
    rv = args->GetCmdLineValue( "-kill", getter_Copies(arg));
    if ( NS_SUCCEEDED(rv) && !arg.IsEmpty() ) {
      
      nsCOMPtr<nsIAppStartup> appStartup
        (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr<nsINativeAppSupport> native;
      rv = appStartup->GetNativeAppSupport( getter_AddRefs( native ));
      if (NS_SUCCEEDED(rv)) {
        native->SetIsServerMode( PR_FALSE );

        
        rv = appStartup->Quit(nsIAppStartup::eConsiderQuit);
      }

      return rv;
    }

    
    
    rv = args->GetCmdLineValue(MAPI_STARTUP_ARG, getter_Copies(arg));
    if (NS_SUCCEEDED(rv) && !arg.IsEmpty()) {
      return nativeApp->EnsureProfile(args);
    }

    

    
    rv = nativeApp->EnsureProfile(args);
    if (NS_FAILED(rv)) return rv;

    
    PRBool windowOpened = PR_FALSE;

    
    rv = DoCommandLines( args, &windowOpened );

    
    
    if (rv == NS_ERROR_NOT_AVAILABLE || rv == NS_ERROR_ABORT || windowOpened) {
      return NS_OK;
    }

    
#if MOZ_DEBUG_DDE
    printf( "Unknown request [%s]\n", (char*) request );
#endif
    
    const char * const contractID =
      "@mozilla.org/commandlinehandler/general-startup;1?type=browser";
    nsCOMPtr<nsICmdLineHandler> handler = do_GetService(contractID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString defaultArgs;
    rv = handler->GetDefaultArgs(getter_Copies(defaultArgs));
    if (NS_FAILED(rv) || defaultArgs.IsEmpty()) return rv;

    
    if (FindCharInString(defaultArgs, '\n') != kNotFound)
        newWindow = PR_TRUE;

    return OpenBrowserWindow(defaultArgs, newWindow, aResult);
}




nsresult
nsNativeAppSupportWin::GetCmdLineArgs( LPBYTE request, nsICmdLineService **aResult ) {
    nsresult rv = NS_OK;

    int justCounting = 1;
    char **argv = 0;
    
    int init = 1;
    int between, quoted, bSlashCount;
    int argc;
    char *p;
    nsCAutoString arg;
    
    while ( 1 ) {
        
        if ( init ) {
            p = (char*)request;
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
                    if (!argv[argc]) {
                        rv = NS_ERROR_OUT_OF_MEMORY;
                        break;
                    }
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
                if (!argv) {
                    rv = NS_ERROR_OUT_OF_MEMORY;
                    break;
                }
                
                justCounting = 0;
                init = 1;
            } else {
                
                break;
            }
        }
    }

    nsCOMPtr<nsIComponentManager> compMgr;
    NS_GetComponentManager(getter_AddRefs(compMgr));
    
    rv = compMgr->CreateInstanceByContractID(
                    NS_COMMANDLINESERVICE_CONTRACTID,
                    nsnull, NS_GET_IID(nsICmdLineService),
                    (void**) aResult);

    if ( NS_FAILED( rv ) || NS_FAILED( ( rv = (*aResult)->Initialize( argc, argv ) ) ) ) {
#if MOZ_DEBUG_DDE
        printf( "Error creating command line service = 0x%08X (argc=%d, argv=0x%08X)\n", (int)rv, (int)argc, (void*)argv );
#endif
    }

    
    while ( argc ) {
        delete [] argv[ --argc ];
    }
    delete [] argv;

    return rv;
}





nsresult
nsNativeAppSupportWin::EnsureProfile(nsICmdLineService* args)
{
  static PRBool firstTime = PR_TRUE;
  if ( firstTime ) {
    firstTime = PR_FALSE;
    
    nsCOMPtr<nsIPrefBranch> prefService( do_GetService( NS_PREFSERVICE_CONTRACTID ) );
    PRBool supportDDEExec = PR_FALSE;
    if ( prefService ) {
        prefService->GetBoolPref( "advanced.system.supportDDEExec", &supportDDEExec );
    }
    if ( supportDDEExec && handlingHTTP() ) {
#if MOZ_DEBUG_DDE
printf( "Setting ddexec subkey entries\n" );
#endif

      DWORD dwDisp;
      HKEY hKey;
      DWORD rc;

      rc = ::RegCreateKeyEx( HKEY_CLASSES_ROOT,
                             "http\\shell\\open\\ddeexec", 0,
                             NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                             &hKey, &dwDisp );

      if ( REG_SUCCEEDED( rc ) ) {
        
        const BYTE ddeexec[] = "\"%1\",,-1,0,,,,";
        ::RegSetValueEx( hKey, "", 0, REG_SZ, ddeexec, sizeof ddeexec );
        ::RegCloseKey( hKey );
      }

      
      rc = ::RegCreateKeyEx( HKEY_CLASSES_ROOT,
                             "http\\shell\\open\\ddeexec\\application", 0,
                             NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                             &hKey, &dwDisp );

      if ( REG_SUCCEEDED( rc ) ) {
        ::RegSetValueEx( hKey, "", 0, REG_SZ, (const BYTE *) mAppName,
                         ::strlen( mAppName ) + 1 );
        ::RegCloseKey( hKey );
      }

      rc = ::RegCreateKeyEx( HKEY_CLASSES_ROOT,
                             "http\\shell\\open\\ddeexec\\topic", 0,
                             NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                             &hKey, &dwDisp );

      if ( REG_SUCCEEDED( rc ) ) {
        const BYTE topic[] = "WWW_OpenURL";
        ::RegSetValueEx( hKey, "", 0, REG_SZ, topic, sizeof topic );
        ::RegCloseKey( hKey );
      }

      
      mSupportingDDEExec = PR_TRUE;
    }
  }

  nsresult rv;

  nsCOMPtr<nsIProfileInternal> profileMgr(do_GetService(NS_PROFILE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIAppStartup> appStartup (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  
  
  
  
  
  PRBool haveProfile;
  rv = profileMgr->IsCurrentProfileAvailable(&haveProfile);
  if (!mForceProfileStartup && NS_SUCCEEDED(rv) && haveProfile)
      return NS_OK;

  
  PRBool doingProfileStartup;
  rv = profileMgr->GetIsStartingUp(&doingProfileStartup);
  if (NS_FAILED(rv) || doingProfileStartup) return NS_ERROR_FAILURE;

  
  PRBool canInteract = PR_TRUE;
  nsXPIDLCString arg;
  if (NS_SUCCEEDED(args->GetCmdLineValue("-silent", getter_Copies(arg)))) {
    if (!arg.IsEmpty()) {
      canInteract = PR_FALSE;
    }
  }
  rv = appStartup->DoProfileStartup(args, canInteract);

  mForceProfileStartup = PR_FALSE;

  return rv;
}

nsresult
nsNativeAppSupportWin::OpenWindow( const char *urlstr,
                                   const nsAString& aArgs,
                                   nsIDOMWindow **aResult )
{

  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsISupportsString>
    sarg(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  if (sarg)
    sarg->SetData(aArgs);

  if (wwatch && sarg) {
    rv = wwatch->OpenWindow(0, urlstr, "_blank", "chrome,dialog=no,all",
                   sarg, aResult);
#if MOZ_DEBUG_DDE
  } else {
      printf("Get WindowWatcher (or create string) failed\n");
#endif
  }
  return rv;
}

static char procPropertyName[] = "MozillaProcProperty";


static LRESULT CALLBACK focusFilterProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    if ( uMsg == WM_SETFOCUS ) {
        
        return 0;
    } else {
        
        HANDLE oldProc = ::GetProp( hwnd, procPropertyName );
        if ( oldProc ) {
            return ::CallWindowProc( (WNDPROC)oldProc, hwnd, uMsg, wParam, lParam );
        } else {
            
            return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
        }
    }
}

HWND hwndForDOMWindow( nsISupports *window ) {
    nsCOMPtr<nsPIDOMWindow> win( do_QueryInterface(window) );
    if ( !win ) {
        return 0;
    }

    nsCOMPtr<nsIBaseWindow> ppBaseWindow =
        do_QueryInterface( win->GetDocShell() );
    if ( !ppBaseWindow ) {
        return 0;
    }

    nsCOMPtr<nsIWidget> ppWidget;
    ppBaseWindow->GetMainWidget( getter_AddRefs( ppWidget ) );

    return (HWND)( ppWidget->GetNativeData( NS_NATIVE_WIDGET ) );
}

nsresult
nsNativeAppSupportWin::ReParent( nsISupports *window, HWND newParent ) {
    HWND hMainFrame = hwndForDOMWindow( window );
    if ( !hMainFrame ) {
        return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    LONG oldProc = 0;
    if ( newParent ) {
        
        oldProc = ::SetWindowLong( hMainFrame,
                                   GWL_WNDPROC,
                                   (LONG)(WNDPROC)focusFilterProc );

        
        
        ::SetProp( hMainFrame, procPropertyName, (HANDLE)oldProc );
    }

    
    ::SetParent( hMainFrame, newParent );

    
    if ( newParent ) {
        ::SetWindowLong( hMainFrame, GWL_WNDPROC, oldProc );
        ::RemoveProp( hMainFrame, procPropertyName );
    }

    return NS_OK;
}

static const char sJSStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";

class SafeJSContext {
public:
  SafeJSContext();
  ~SafeJSContext();

  nsresult   Push();
  JSContext *get() { return mContext; }

protected:
  nsCOMPtr<nsIThreadJSContextStack>  mService;
  JSContext                         *mContext;
};

SafeJSContext::SafeJSContext() : mContext(nsnull) {
}

SafeJSContext::~SafeJSContext() {
  JSContext *cx;
  nsresult   rv;

  if(mContext) {
    rv = mService->Pop(&cx);
    NS_ASSERTION(NS_SUCCEEDED(rv) && cx == mContext, "JSContext push/pop mismatch");
  }
}

nsresult SafeJSContext::Push() {
  if (mContext) 
    return NS_ERROR_FAILURE;

  mService = do_GetService(sJSStackContractID);
  if(mService) {
    JSContext *cx;
    if (NS_SUCCEEDED(mService->GetSafeJSContext(&cx)) &&
        cx &&
        NS_SUCCEEDED(mService->Push(cx))) {
      
      mContext = cx;
    }
  }
  return mContext ? NS_OK : NS_ERROR_FAILURE;
}


nsresult
nsNativeAppSupportWin::OpenBrowserWindow( const nsAString& aArgs,
                                          PRBool newWindow,
                                          nsIDOMWindow **aResult )
{
    nsresult rv = NS_OK;
    
    

    
    nsCOMPtr<nsIDOMWindowInternal> navWin;
    GetMostRecentWindow( NS_LITERAL_STRING( "navigator:browser" ).get(), getter_AddRefs( navWin ) );

    
    
    do {
        
        if ( newWindow ) {
            break;
        }
        if ( !navWin ) {
            
            break;
        }
        nsCOMPtr<nsIDOMChromeWindow> chromeWin( do_QueryInterface( navWin ) );
        if ( !chromeWin ) {
            break;
        }
        nsCOMPtr<nsIBrowserDOMWindow> bwin;
        chromeWin->GetBrowserDOMWindow( getter_AddRefs( bwin ) );
        if ( !bwin ) {
            break;
        }
        nsCOMPtr<nsIURIFixup> fixup( do_GetService( NS_URIFIXUP_CONTRACTID ) );
        if ( !fixup ) {
            break;
        }
        nsCOMPtr<nsIURI> uri;
        rv = fixup->CreateFixupURI( NS_ConvertUTF16toUTF8( aArgs ),
                                    nsIURIFixup::FIXUP_FLAG_NONE,
                                    getter_AddRefs( uri ) );
        if ( NS_FAILED(rv) || !uri ) {
            break;
        }
        return bwin->OpenURI( uri, nsnull, nsIBrowserDOMWindow::OPEN_DEFAULTWINDOW, nsIBrowserDOMWindow::OPEN_EXTERNAL, aResult );
    } while ( PR_FALSE );

    nsCOMPtr<nsICmdLineHandler> handler(do_GetService("@mozilla.org/commandlinehandler/general-startup;1?type=browser", &rv));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString chromeUrlForTask;
    rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
    if (NS_FAILED(rv)) return rv;

    
    return OpenWindow( chromeUrlForTask.get(), aArgs, aResult );
}

void AppendMenuItem( HMENU& menu, PRInt32 aIdentifier, const nsString& aText ) {
  char* ACPText = GetACPString( aText );
  if ( ACPText ) {
    ::AppendMenu( menu, MF_STRING, aIdentifier, ACPText );
    delete [] ACPText;
  }
}



void
nsNativeAppSupportWin::SetupSysTrayIcon() {
    
    mIconData.hWnd  = (HWND)MessageWindow();

    
    mIconData.hIcon =  (HICON)::LoadImage( ::GetModuleHandle( NULL ),
                                           IDI_APPLICATION,
                                           IMAGE_ICON,
                                           ::GetSystemMetrics( SM_CXSMICON ),
                                           ::GetSystemMetrics( SM_CYSMICON ),
                                           NULL );

    
    mIconData.szTip[0] = 0;
    nsCOMPtr<nsIStringBundleService> svc( do_GetService( NS_STRINGBUNDLE_CONTRACTID ) );
    if ( svc ) {
        nsCOMPtr<nsIStringBundle> brandBundle;
        nsXPIDLString tooltip;
        svc->CreateBundle( "chrome://branding/locale/brand.properties", getter_AddRefs( brandBundle ) );
        if ( brandBundle ) {
            brandBundle->GetStringFromName( NS_LITERAL_STRING( "brandShortName" ).get(),
                                            getter_Copies( tooltip ) );
            ::strncpy( mIconData.szTip,
                       NS_LossyConvertUTF16toASCII(tooltip).get(),
                       sizeof mIconData.szTip - 1 );
        }
        
        nsCOMPtr<nsIStringBundle> turboBundle;
        nsCOMPtr<nsIStringBundle> mailBundle;
        svc->CreateBundle( "chrome://navigator/locale/turboMenu.properties",
                           getter_AddRefs( turboBundle ) );
        nsresult rv = svc->CreateBundle( "chrome://messenger/locale/mailTurboMenu.properties",
                                         getter_AddRefs( mailBundle ) );
        PRBool isMail = NS_SUCCEEDED(rv) && mailBundle;
        nsAutoString exitText;
        nsAutoString disableText;
        nsAutoString navigatorText;
        nsAutoString editorText;
        nsAutoString mailText;
        nsAutoString addressbookText;
        nsXPIDLString text;
        if ( turboBundle ) {
            if ( brandBundle ) {
                const PRUnichar* formatStrings[] = { tooltip.get() };
                turboBundle->FormatStringFromName( NS_LITERAL_STRING( "Exit" ).get(), formatStrings, 1,
                                                   getter_Copies( text ) );
                exitText = text;
            }
            turboBundle->GetStringFromName( NS_LITERAL_STRING( "Disable" ).get(),
                                            getter_Copies( text ) );
            disableText = text;
            turboBundle->GetStringFromName( NS_LITERAL_STRING( "Navigator" ).get(),
                                            getter_Copies( text ) );
            navigatorText = text;
            turboBundle->GetStringFromName( NS_LITERAL_STRING( "Editor" ).get(),
                                            getter_Copies( text ) );
            editorText = text;
        }
        if (isMail) {
            mailBundle->GetStringFromName( NS_LITERAL_STRING( "MailNews" ).get(),
                                           getter_Copies( text ) );
            mailText = text;
            mailBundle->GetStringFromName( NS_LITERAL_STRING( "Addressbook" ).get(),
                                           getter_Copies( text ) );
            addressbookText = text;
        }

        if ( exitText.IsEmpty() ) {
            exitText.Assign( NS_LITERAL_STRING( "E&xit " ) );
            exitText.Append( NS_LITERAL_STRING( NS_STRINGIFY(MOZ_APP_DISPLAYNAME) ) );
        }

        if ( disableText.IsEmpty() )
            disableText.Assign( NS_LITERAL_STRING("&Disable Quick Launch") );

        if ( navigatorText.IsEmpty() )
            navigatorText.Assign( NS_LITERAL_STRING("&Navigator") );

        if ( editorText.IsEmpty() )
            editorText.Assign( NS_LITERAL_STRING("&Composer") );

        if ( isMail ) {
            if ( mailText.IsEmpty() )
              mailText.Assign( NS_LITERAL_STRING("&Mail && Newsgroups") );
            if ( addressbookText.IsEmpty() )
              addressbookText.Assign( NS_LITERAL_STRING("&Address Book") );
        }
        
        mTrayIconMenu = ::CreatePopupMenu();
        ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_NAVIGATOR, navigatorText.get() );
        if ( ::GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) {
            AppendMenuItem( mTrayIconMenu, TURBO_NAVIGATOR, navigatorText );
            if ( isMail )
                AppendMenuItem( mTrayIconMenu, TURBO_MAIL, mailText );
            AppendMenuItem( mTrayIconMenu, TURBO_EDITOR, editorText );
            if ( isMail )
                AppendMenuItem( mTrayIconMenu, TURBO_ADDRESSBOOK, addressbookText );
            ::AppendMenu( mTrayIconMenu, MF_SEPARATOR, NULL, NULL );
            AppendMenuItem( mTrayIconMenu, TURBO_DISABLE, disableText );
            AppendMenuItem( mTrayIconMenu, TURBO_EXIT, exitText );
        }
        else {
            if (isMail)
                ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_MAIL, mailText.get() );
            ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_EDITOR, editorText.get() );
            if (isMail)
                ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_ADDRESSBOOK, addressbookText.get() );
            ::AppendMenuW( mTrayIconMenu, MF_SEPARATOR, NULL, NULL );
            ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_DISABLE, disableText.get() );
            ::AppendMenuW( mTrayIconMenu, MF_STRING, TURBO_EXIT, exitText.get() );
        }
    }

    

    


    mTrayRestart = ::RegisterWindowMessage(TEXT("TaskbarCreated"));
    ::Shell_NotifyIcon( NIM_ADD, &mIconData );
}


void
nsNativeAppSupportWin::RemoveSysTrayIcon() {
    
    mTrayRestart = 0;
    ::Shell_NotifyIcon( NIM_DELETE, &mIconData );
    
    ::DestroyMenu( mTrayIconMenu );
}









NS_IMETHODIMP
nsNativeAppSupportWin::StartServerMode() {

    
    SetupSysTrayIcon();

    if (mShouldShowUI) {
        
        
        return NS_OK;
    } else {
        
        
        
        nsCOMPtr<nsIDOMWindowInternal> win;
        GetMostRecentWindow( 0, getter_AddRefs( win ) );
        if ( win ) {
            
            return NS_OK;
        }
    }

    
    

    
    nsCOMPtr<nsIWindowWatcher>   ww(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    nsCOMPtr<nsISupportsString> arg1(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
    nsCOMPtr<nsISupportsString> arg2(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
    if ( !ww || !arg1 || !arg2 ) {
        return NS_OK;
    }

    
    nsCOMPtr<nsISupportsArray> argArray = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
    if ( !argArray ) {
        return NS_OK;
    }

    
    
    arg1->SetData( NS_LITERAL_STRING( "about:blank" ) );
    arg2->SetData( NS_LITERAL_STRING( "turbo=yes" ) );

    
    if ( NS_FAILED( argArray->AppendElement( arg1 ) ) ||
        NS_FAILED( argArray->AppendElement( arg2 ) ) ) {
        return NS_OK;
    }

    
    nsCOMPtr<nsIDOMWindow> newWindow;
    ww->OpenWindow( 0,
        "chrome://navigator/content",
        "_blank",
        "chrome,dialog=no,toolbar=no",
        argArray,
        getter_AddRefs( newWindow ) );

    if ( !newWindow ) {
        return NS_OK;
    }
    mInitialWindowActive = PR_TRUE;

    
    ReParent( newWindow, (HWND)MessageWindow() );

    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportWin::SetIsServerMode( PRBool isServerMode ) {
    
    if ( mServerMode && !isServerMode ) {
        RemoveSysTrayIcon();
    }
    else if ( !mServerMode && isServerMode) {
        SetupSysTrayIcon();
    }
    return nsNativeAppSupportBase::SetIsServerMode( isServerMode );
}

NS_IMETHODIMP
nsNativeAppSupportWin::OnLastWindowClosing() {

    if ( !mServerMode )
        return NS_OK;

    
    
    if ( mInitialWindowActive ) {
        mInitialWindowActive = PR_FALSE;
        mShouldShowUI = PR_TRUE;
        return NS_OK;
    }

    
    
    if ( mLastWindowIsConfirmation ) {
        mLastWindowIsConfirmation = PR_FALSE;
        return NS_OK;
    }


    nsresult rv;

    
    
    
    PRBool singleProfileOnly = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefService( do_GetService( NS_PREFSERVICE_CONTRACTID, &rv ) );
    if ( NS_SUCCEEDED( rv ) ) {
        prefService->GetBoolPref( "browser.turbo.singleProfileOnly", &singleProfileOnly );
    }
    if ( singleProfileOnly ) {
        nsCOMPtr<nsIProfile> profileMgr( do_GetService( NS_PROFILE_CONTRACTID, &rv ) );
        if ( NS_SUCCEEDED( rv ) ) {
            PRInt32 profileCount = 0;
            if ( NS_SUCCEEDED( profileMgr->GetProfileCount( &profileCount ) ) &&
                 profileCount > 1 ) {
                
                SetIsServerMode( PR_FALSE );
                nsCOMPtr<nsIAppStartup> appStartup
                    (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
                if ( NS_SUCCEEDED( rv ) ) {
                    appStartup->Quit(nsIAppStartup::eAttemptQuit);
                }
                return NS_OK;
            }
        }
    }

    if ( !mShownTurboDialog ) {
        PRBool showDialog = PR_TRUE;
        if ( NS_SUCCEEDED( rv ) )
            prefService->GetBoolPref( "browser.turbo.showDialog", &showDialog );

        if ( showDialog ) {
          



          nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
          if ( wwatch ) {
              nsCOMPtr<nsIDOMWindow> newWindow;
              mShownTurboDialog = PR_TRUE;
              mLastWindowIsConfirmation = PR_TRUE;
              rv = wwatch->OpenWindow(0, "chrome://navigator/content/turboDialog.xul",
                                      "_blank", "chrome,modal,titlebar,centerscreen,dialog",
                                      0, getter_AddRefs(newWindow));
          }
        }
    }

    nsCOMPtr<nsIAppStartup> appStartup
        (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
    if ( NS_SUCCEEDED( rv ) ) {
        
        
        
        

        
        Mutex mutexLock = Mutex(mMutexName);
        NS_ENSURE_TRUE(mutexLock.Lock(MOZ_DDE_START_TIMEOUT), NS_ERROR_FAILURE );

        
        MessageWindow mw;
        mw.Destroy();

        
        char buffer[ _MAX_PATH ];
        
        ::GetModuleFileName( 0, buffer, sizeof buffer );
        
        ::GetShortPathName( buffer, buffer, sizeof buffer );
        nsCAutoString cmdLine( buffer );
        
        cmdLine.Append( " -turbo" );

        
        STARTUPINFO startupInfo;
        ::GetStartupInfo( &startupInfo );
        PROCESS_INFORMATION processInfo;
        DWORD rc = ::CreateProcess( 0,
                              (LPTSTR)cmdLine.get(),
                              0,
                              0,
                              0,
                              0,
                              0,
                              0,
                              &startupInfo,
                              &processInfo );

        
        SetIsServerMode( PR_FALSE );
        appStartup->Quit(nsIAppStartup::eAttemptQuit);

        
    }
    return NS_OK;
}
