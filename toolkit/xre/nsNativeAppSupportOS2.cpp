







































#ifdef MOZ_OS2_HIGH_MEMORY

#include <os2safe.h>
#endif

#define INCL_PM
#define INCL_GPI
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "nsNativeAppSupportBase.h"
#include "nsNativeAppSupportOS2.h"
#include "nsAppRunner.h"
#include "nsXULAppAPI.h"
#include "nsString.h"
#include "nsIBrowserDOMWindow.h"
#include "nsICommandLineRunner.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsISupportsArray.h"
#include "nsIWindowWatcher.h"
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
#include "nsIJSContextStack.h"
#include "nsIWebNavigation.h"
#include "nsIWindowMediator.h"

#include <stdlib.h>
#include "prprf.h"

#define kMailtoUrlScheme "mailto:"





typedef struct _COPYDATASTRUCT
{
   ULONG   dwData;
   ULONG   cbData;
   PVOID   lpData;
   CHAR    chBuff;
}COPYDATASTRUCT, *PCOPYDATASTRUCT;
#define WM_COPYDATA             (WM_USER + 60)

char szCommandLine[2*CCHMAXPATH];

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

static
void
activateWindow( nsIDOMWindowInternal *win ) {
    
    HWND hwnd = hwndForDOMWindow( win );
    if ( hwnd ) {

        


        LONG id = WinQueryWindowUShort( hwnd, QWS_ID );
        if( id == FID_CLIENT )
        {
           hwnd = WinQueryWindow( hwnd, QW_PARENT );
        }

        
        
        WinSetWindowPos( hwnd, 0L, 0L, 0L, 0L, 0L, 
                         SWP_SHOW | SWP_RESTORE | SWP_ACTIVATE );
    } else {
        
        win->Focus();
    }
}


#ifdef DEBUG_law
#undef MOZ_DEBUG_DDE
#define MOZ_DEBUG_DDE 1
#endif


struct Mutex {
    Mutex( const char *name )
        : mName( name ),
          mHandle( 0 ),
          mState( 0xFFFF ) {
        
        mName.Insert("\\SEM32\\", 0);
        APIRET rc = DosCreateMutexSem(mName.get(), &mHandle, 0, FALSE);
        if (rc != NO_ERROR) {
#if MOZ_DEBUG_DDE
            printf("CreateMutex error = 0x%08X\n", (int)rc);
#endif
        }
    }
    ~Mutex() {
        if ( mHandle ) {
            
            Unlock();

            APIRET rc = DosCloseMutexSem(mHandle);
            if (rc != NO_ERROR) {
#if MOZ_DEBUG_DDE
                printf("CloseHandle error = 0x%08X\n", (int)rc);
#endif
            }
        }
    }
    BOOL Lock( DWORD timeout ) {
        if ( mHandle ) {
#if MOZ_DEBUG_DDE
            printf( "Waiting (%d msec) for DDE mutex...\n", (int)timeout );
#endif
            mState = DosRequestMutexSem( mHandle, timeout );
#if MOZ_DEBUG_DDE
            printf( "...wait complete, result = 0x%08X\n", (int)mState );
#endif
            return (mState == NO_ERROR);
        } else {
            return FALSE;
        }
    }
    void Unlock() {
        if ( mHandle && mState == NO_ERROR ) {
#if MOZ_DEBUG_DDE
            printf( "Releasing DDE mutex\n" );
#endif
            DosReleaseMutexSem( mHandle );
            mState = 0xFFFF;
        }
    }
private:
    nsCString mName;
    HMTX      mHandle;
    DWORD     mState;
};


















































































class nsNativeAppSupportOS2 : public nsNativeAppSupportBase,
                              public nsIObserver
{
public:
    NS_DECL_NSIOBSERVER
    NS_DECL_ISUPPORTS_INHERITED

    
    NS_IMETHOD Start( PRBool *aResult );
    NS_IMETHOD Stop( PRBool *aResult );
    NS_IMETHOD Quit();
    NS_IMETHOD Enable();

    
    NS_IMETHOD StartDDE();

    
    
    
    void CheckConsole();

private:
    static HDDEDATA APIENTRY HandleDDENotification( ULONG    idInst,
                                                    USHORT   uType,
                                                    USHORT   uFmt,
                                                    HCONV    hconv,
                                                    HSZ      hsz1,
                                                    HSZ      hsz2,
                                                    HDDEDATA hdata,
                                                    ULONG    dwData1,
                                                    ULONG    dwData2 );
    static void HandleCommandLine(const char* aCmdLineString, nsIFile* aWorkingDir, PRUint32 aState);
    static void ParseDDEArg( HSZ args, int index, nsCString& string);
    static void ParseDDEArg( const char* args, int index, nsCString& aString);
    static void ActivateLastWindow();
    static HDDEDATA CreateDDEData( DWORD value );
    static HDDEDATA CreateDDEData( LPBYTE value, DWORD len );
    static PRBool   InitTopicStrings();
    static int      FindTopic( HSZ topic );
    static nsresult OpenWindow( const char *urlstr, const char *args );
    static nsresult OpenBrowserWindow();

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
    static PRBool mCanHandleRequests;
    static char mMutexName[];
    static PRBool mUseDDE;
    friend struct MessageWindow;
}; 

NS_INTERFACE_MAP_BEGIN(nsNativeAppSupportOS2)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END_INHERITING(nsNativeAppSupportBase)

NS_IMPL_ADDREF_INHERITED(nsNativeAppSupportOS2, nsNativeAppSupportBase)
NS_IMPL_RELEASE_INHERITED(nsNativeAppSupportOS2, nsNativeAppSupportBase)

void
nsNativeAppSupportOS2::CheckConsole() {
    return;
}



nsresult
NS_CreateNativeAppSupport( nsINativeAppSupport **aResult ) {
    nsNativeAppSupportOS2 *pNative = new nsNativeAppSupportOS2;
    if (!pNative) return NS_ERROR_OUT_OF_MEMORY;

    
    pNative->CheckConsole();

    *aResult = pNative;
    NS_ADDREF( *aResult );

    return NS_OK;
}


#define MOZ_DDE_APPLICATION    "Mozilla"
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


int   nsNativeAppSupportOS2::mConversations = 0;
HSZ   nsNativeAppSupportOS2::mApplication   = 0;
HSZ   nsNativeAppSupportOS2::mTopics[nsNativeAppSupportOS2::topicCount] = { 0 };
DWORD nsNativeAppSupportOS2::mInstance      = 0;
PRBool nsNativeAppSupportOS2::mCanHandleRequests   = PR_FALSE;


int DdeCmpStringHandles( HSZ hsz1, HSZ hsz2 )
{
  char chhsz1[CCHMAXPATH], chhsz2[CCHMAXPATH];
  int rc = -1;

  




  WinDdeQueryString( hsz1, chhsz1, sizeof( chhsz1 ), 0 );
  WinDdeQueryString( hsz2, chhsz2, sizeof( chhsz2 ),0 );

  rc = stricmp( chhsz1, chhsz2 );

  return(rc);
}


char *GetCommandLine()
{
   





   PTIB pTIB = NULL;
   PPIB pPIB = NULL;
   APIRET rc = NO_ERROR;
   char *pchParam = NULL;

   rc = DosGetInfoBlocks( &pTIB, &pPIB );
   if( rc == NO_ERROR )
   {
      INT iLen = 0;
      char *pchTemp = NULL;
      pchParam = pPIB->pib_pchcmd;
      strcpy( szCommandLine, pchParam );
      iLen = strlen( pchParam );

      


      pchTemp = &(pchParam[iLen+1]);

      



      if( *pchTemp )
      {
         szCommandLine[iLen] = ' ';
         iLen++;
         if( *pchTemp == ' ' )
         {
            pchTemp++;
         }
         strcpy( &(szCommandLine[iLen]), pchTemp );
      }

   }

   return( szCommandLine );
}

typedef struct _DDEMLFN
{
   PFN   *fn;
   ULONG ord; 
} DDEMLFN, *PDDEMLFN;

DDEMLFN ddemlfnTable[] = 
{
   { (PFN *)&WinDdeAbandonTransaction   ,100 },
   { (PFN *)&WinDdeAccessData           ,101 },
   { (PFN *)&WinDdeAddData              ,102 },
   { (PFN *)&WinDdeSubmitTransaction    ,103 },
   { (PFN *)&WinDdeCompareStringHandles ,104 },
   { (PFN *)&WinDdeConnect              ,105 },
   { (PFN *)&WinDdeConnectList          ,106 },
   { (PFN *)&WinDdeCreateDataHandle     ,107 },
   { (PFN *)&WinDdeCreateStringHandle   ,108 },
   { (PFN *)&WinDdeDisconnect           ,109 },
   { (PFN *)&WinDdeDisconnectList       ,110 },
   { (PFN *)&WinDdeEnableCallback       ,111 },
   { (PFN *)&WinDdeFreeDataHandle       ,112 },
   { (PFN *)&WinDdeFreeStringHandle     ,113 },
   { (PFN *)&WinDdeGetData              ,114 },
   { (PFN *)&WinDdeInitialize           ,116 },
   { (PFN *)&WinDdeKeepStringHandle     ,117 },
   { (PFN *)&WinDdeNameService          ,118 },
   { (PFN *)&WinDdePostAdvise           ,119 },
   { (PFN *)&WinDdeQueryConvInfo        ,120 },
   { (PFN *)&WinDdeQueryNextServer      ,121 },
   { (PFN *)&WinDdeQueryString          ,122 },
   { (PFN *)&WinDdeReconnect            ,123 },
   { (PFN *)&WinDdeSetUserHandle        ,124 },
   { (PFN *)&WinDdeUninitialize         ,126 },
   { (PFN *)NULL                           ,  0 }
};





BOOL SetupOS2ddeml()
{
    BOOL bRC = FALSE;
    HMODULE hmodDDEML = NULLHANDLE;
    APIRET rc = NO_ERROR;

    rc = DosLoadModule( NULL, 0, "PMDDEML", &hmodDDEML );
    if( rc == NO_ERROR )
    {
       int i;
       
       bRC = TRUE;
       for( i=0; ddemlfnTable[i].ord != 0; i++ )
       {
          rc = DosQueryProcAddr( hmodDDEML, ddemlfnTable[i].ord, NULL,
                                 ddemlfnTable[i].fn );
          if( rc != NO_ERROR )
          {
             
             bRC = FALSE;
             break;
          }
       }
    } 

    return( bRC );
}

char nsNativeAppSupportOS2::mMutexName[ 128 ] = { 0 };



struct MessageWindow {
    
    MessageWindow() {
        
        HATOMTBL  hatomtbl;
        HENUM     henum;
        HWND      hwndNext;
        char      classname[CCHMAXPATH];

        mHandle = NULLHANDLE;

        hatomtbl = WinQuerySystemAtomTable();
        mMsgWindowAtom = WinFindAtom(hatomtbl, className());
        if (mMsgWindowAtom == 0)
        {
          
          
          mMsgWindowAtom = WinAddAtom(hatomtbl, className());
        } else {
          
          
          
          henum = WinBeginEnumWindows(HWND_OBJECT);
          while ((hwndNext = WinGetNextWindow(henum)) != NULLHANDLE)
          {
            if (WinQueryWindowUShort(hwndNext, QWS_ID) == (USHORT)mMsgWindowAtom)
            {
              WinQueryClassName(hwndNext, CCHMAXPATH, classname);
              if (strcmp(classname, className()) == 0)
              {
                mHandle = hwndNext;
                break;
              }
            }
          }
        }
    }

    HWND getHWND() {
        return mHandle;
    }

    
    static const char *className() {
        static char classNameBuffer[128];
        static char *mClassName = 0;
        if ( !mClassName ) { 
            sprintf( classNameBuffer,
                         "%s%s",
                         gAppData->name,
                         "MessageWindow" );
            mClassName = classNameBuffer;
        }
        return mClassName;
    }

    
    NS_IMETHOD Create() {

        
        NS_ENSURE_TRUE( WinRegisterClass( 0, className(), 
                                          (PFNWP)&MessageWindow::WindowProc, 
                                          0L, 0 ), 
                        NS_ERROR_FAILURE );

        



        const char * pszClassName = className();
        mHandle = WinCreateWindow( HWND_OBJECT,
                                   pszClassName,
                                   NULL,        
                                   WS_DISABLED, 
                                   0,0,     
                                   0,0,       
                                   HWND_DESKTOP,
                                   HWND_BOTTOM,  
                                   (USHORT)mMsgWindowAtom, 
                                   NULL,        
                                   NULL );      

#if MOZ_DEBUG_DDE
        printf( "Message window = 0x%08X\n", (int)mHandle );
#endif

        return NS_OK;
    }

    
    NS_IMETHOD Destroy() {
        nsresult retval = NS_OK;

        if ( mHandle ) {
           HATOMTBL hatomtbl = WinQuerySystemAtomTable();
           WinDeleteAtom(hatomtbl, mMsgWindowAtom);
           
            
            
            BOOL desRes = WinDestroyWindow( mHandle );
            if ( FALSE != desRes ) {
                mHandle = NULL;
            }
            else {
                retval = NS_ERROR_FAILURE;
            }
        }

        return retval;
    }

    
    

    NS_IMETHOD SendRequest( const char *cmd )
    {
    






        COPYDATASTRUCT *pcds;
        PVOID pvData = NULL;

        if (!cmd)
            return NS_ERROR_FAILURE;

        ULONG ulSize = sizeof(COPYDATASTRUCT)+strlen(cmd)+1+CCHMAXPATH;
#ifdef MOZ_OS2_HIGH_MEMORY
        APIRET rc = DosAllocSharedMem( &pvData, NULL, ulSize,
                                       PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE | OBJ_ANY);
        if (rc != NO_ERROR) { 
            
            
            
            rc = DosAllocSharedMem( &pvData, NULL, ulSize,
                                    PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE);
            if (rc != NO_ERROR) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
#else
        if (DosAllocSharedMem( &pvData, NULL, ulSize,
                               PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE | OBJ_ANY))
            return NS_ERROR_OUT_OF_MEMORY;
#endif

        
        
        
        
        pcds = (COPYDATASTRUCT *)(pvData);
        pcds->dwData = 1;

        char * ptr = &(pcds->chBuff);
        pcds->lpData = ptr;
        strcpy( ptr, cmd);
        pcds->cbData = strlen( ptr) + 1;
        ptr += pcds->cbData;

        if (DosQueryPathInfo( ".", FIL_QUERYFULLNAME, ptr, CCHMAXPATH)) {
            ptr[0] = '.';
            ptr[1] = '\0';
        }
        pcds->cbData += strlen( ptr) + 1;

        WinSendMsg( mHandle, WM_COPYDATA, 0, (MPARAM)pcds );
        DosFreeMem( pvData );

        return NS_OK;
    }

    
    static
    MRESULT EXPENTRY WindowProc( HWND msgWindow, ULONG msg, MPARAM wp, MPARAM lp )
    {
        
        if ( msg == WM_CREATE )
            return (MRESULT)FALSE;

        if ( msg != WM_COPYDATA ) 
            return (MRESULT)TRUE;

        
        COPYDATASTRUCT *cds = (COPYDATASTRUCT*)lp;
        DosGetSharedMem( (PVOID)cds, PAG_READ|PAG_WRITE );

        nsCOMPtr<nsILocalFile> workingDir;

        
        
        if (cds->dwData >= 1) {
            char* wdpath = strchr( (char*)cds->lpData, 0) + 1;
            NS_NewNativeLocalFile(nsDependentCString(wdpath),
                                  PR_FALSE, getter_AddRefs(workingDir));
        }

        nsNativeAppSupportOS2::HandleCommandLine((char*)cds->lpData,
                                workingDir, nsICommandLine::STATE_REMOTE_AUTO);

        return (MRESULT)TRUE;
    }

private:
    HWND mHandle;
    USHORT   mMsgWindowAtom;
}; 

PRBool nsNativeAppSupportOS2::mUseDDE = PR_FALSE;













NS_IMETHODIMP
nsNativeAppSupportOS2::Start( PRBool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    nsresult rv = NS_ERROR_FAILURE;
    *aResult = PR_FALSE;

    
    
    
    for (int i = 1; i < gArgc; i++) {
        if (stricmp("-dde", gArgv[i]) == 0 ||
            stricmp("/dde", gArgv[i]) == 0)
            mUseDDE = PR_TRUE;
        else
            if (stricmp("-console", gArgv[i]) != 0 &&
                stricmp("/console", gArgv[i]) != 0)
                continue;

        for (int j = i; j < gArgc; j++)
            gArgv[j] = gArgv[j+1];
        gArgc--;
        i--;
    }

    
    
    if (getenv("MOZ_NO_REMOTE")) {
        mUseDDE = PR_FALSE;
        *aResult = PR_TRUE;
        return NS_OK;
    }

    

    
    PR_snprintf( mMutexName, sizeof mMutexName, "%s%s", gAppData->name, MOZ_STARTUP_MUTEX_NAME );
    Mutex startupLock = Mutex( mMutexName );

    NS_ENSURE_TRUE( startupLock.Lock( MOZ_DDE_START_TIMEOUT ), NS_ERROR_FAILURE );

    




    MQINFO mqinfo;
    HAB hab;
    HMQ hmqCurrent = WinQueryQueueInfo( HMQ_CURRENT, &mqinfo, 
                                        sizeof( MQINFO ) );
    if( !hmqCurrent ) {
        hab = WinInitialize( 0 );
        hmqCurrent = WinCreateMsgQueue( hab, 0 );
    }

    
    MessageWindow msgWindow;
    if ( msgWindow.getHWND() ) {
        
        char *cmd = GetCommandLine();
        rv = msgWindow.SendRequest( cmd );
    } else {
        
        rv = msgWindow.Create();
        if ( NS_SUCCEEDED( rv ) ) {
            
            if (mUseDDE)
                this->StartDDE();
            
            *aResult = PR_TRUE;
        }
    }

    startupLock.Unlock();

    if( *aResult == PR_FALSE )
    {
        


        if (hmqCurrent)
           WinDestroyMsgQueue(hmqCurrent);
        if (hab)
           WinTerminate(hab);
    }

    return rv;
}

PRBool
nsNativeAppSupportOS2::InitTopicStrings() {
    for ( int i = 0; i < topicCount; i++ ) {
        if ( !( mTopics[ i ] = WinDdeCreateStringHandle( (PSZ)topicNames[ i ], CP_WINANSI ) ) ) {
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}

int
nsNativeAppSupportOS2::FindTopic( HSZ topic ) {
    for ( int i = 0; i < topicCount; i++ ) {
        if ( DdeCmpStringHandles( topic, mTopics[i] ) == 0 ) {
            return i;
        }
    }
    return -1;
}











NS_IMETHODIMP
nsNativeAppSupportOS2::StartDDE() {
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    
    BOOL bDDEML = SetupOS2ddeml();

    
    
    if (!bDDEML) {
       mUseDDE = PR_FALSE;
       return NS_OK;
    }

    
    NS_ENSURE_TRUE( DDEERR_NO_ERROR == WinDdeInitialize( &mInstance,
                                                         nsNativeAppSupportOS2::HandleDDENotification,
                                                         APPCLASS_STANDARD,
                                                         0 ),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( ( mApplication = WinDdeCreateStringHandle( (char*) gAppData->name, CP_WINANSI ) ) && InitTopicStrings(),
                    NS_ERROR_FAILURE );

    
    NS_ENSURE_TRUE( WinDdeNameService( mInstance, mApplication, 0, DNS_REGISTER ), NS_ERROR_FAILURE );

#if MOZ_DEBUG_DDE
    printf( "DDE server started\n" );
#endif

    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportOS2::Stop( PRBool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance, NS_ERROR_NOT_INITIALIZED );

    nsresult rv = NS_OK;
    *aResult = PR_TRUE;

    if (!mUseDDE) {
       return rv;
    }

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
nsNativeAppSupportOS2::Observe(nsISupports* aSubject, const char* aTopic,
                               const PRUnichar* aData)
{
    if (strcmp(aTopic, "quit-application") == 0) {
        Quit();
    } else {
        NS_ERROR("Unexpected observer topic.");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportOS2::Quit() {
    
    
    
    
    Mutex mutexLock(mMutexName);
    NS_ENSURE_TRUE(mutexLock.Lock(MOZ_DDE_START_TIMEOUT), NS_ERROR_FAILURE);

    
    
    
    
    MessageWindow mw;
    mw.Destroy();

    if ( mInstance ) {
        
        WinDdeNameService( mInstance, mApplication, 0, DNS_UNREGISTER );
        
        if ( mApplication ) {
            WinDdeFreeStringHandle( mApplication );
            mApplication = 0;
        }
        for ( int i = 0; i < topicCount; i++ ) {
            if ( mTopics[i] ) {
                WinDdeFreeStringHandle( mTopics[i] );
                mTopics[i] = 0;
            }
        }
        WinDdeUninitialize( mInstance );
        mInstance = 0;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportOS2::Enable()
{
    mCanHandleRequests = PR_TRUE;
    nsCOMPtr<nsIObserverService> obs
        (do_GetService("@mozilla.org/observer-service;1"));
    if (obs) {
        obs->AddObserver(this, "quit-application", PR_FALSE);
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
    DWORD len = WinDdeQueryString( hsz, NULL, NULL, CP_WINANSI );
    if ( len ) {
        char buffer[ 256 ];
        WinDdeQueryString( hsz, buffer, sizeof buffer, CP_WINANSI );
        result += buffer;
    }
    result += "]";
    return result;
}
#endif



static void escapeQuotes( nsAString &aString ) {
    PRInt32 offset = -1;
    while( 1 ) {
       
       offset = aString.FindChar( '"', ++offset );
       if ( offset == kNotFound ) {
           
           break;
       } else {
           
           aString.Insert( PRUnichar('\\'), offset );
           
           offset++;
       }
    }
    return;
}


HDDEDATA APIENTRY
nsNativeAppSupportOS2::HandleDDENotification( ULONG idInst,     
                                              USHORT uType,     
                                              USHORT uFmt,      
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
                    

                    
                    nsCAutoString url;
                    ParseDDEArg(hsz2, 0, url);

                    
                    
                    nsCAutoString windowID;
                    ParseDDEArg(hsz2, 2, windowID);
                    
                    
                    
                    if (windowID.Equals( "0" ) || windowID.Equals( "" ))
                        url.Insert("mozilla -new-window ", 0);
                    else
                        url.Insert("mozilla -url ", 0);

#if MOZ_DEBUG_DDE
                    printf( "Handling dde XTYP_REQUEST request: [%s]...\n", url.get() );
#endif
                    
                    HandleCommandLine(url.get(), nsnull, nsICommandLine::STATE_REMOTE_EXPLICIT);
                    
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

                        
                        
                        nsCAutoString   outpt( NS_LITERAL_CSTRING("\"") );
                        
                        
                        outpt.Append( NS_LossyConvertUTF16toASCII( url ) );
                        
                        
                        outpt.Append( NS_LITERAL_CSTRING("\",\"") );
                        
                        outpt.Append( NS_LossyConvertUTF16toASCII( title.get() ));
                        
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
                    
                    
                    if ( windowID.Equals( "-1" ) ||
                         windowID.Equals( "4294967295" ) ) {
                        
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
            LPBYTE request = (LPBYTE)WinDdeAccessData( hdata, &bytes );
#if MOZ_DEBUG_DDE
            printf( "Handling dde request: [%s]...\n", (char*)request );
#endif
            nsCAutoString url;
            ParseDDEArg((const char*) request, 0, url);

            
            
            nsCAutoString windowID;
            ParseDDEArg((const char*) request, 2, windowID);

            
            
            
            if (windowID.Equals( "0" ) || windowID.Equals( "" ))
                url.Insert("mozilla -new-window ", 0);
            else
                url.Insert("mozilla -url ", 0);

#if MOZ_DEBUG_DDE
            printf( "Handling dde XTYP_REQUEST request: [%s]...\n", url.get() );
#endif
            
            HandleCommandLine(url.get(), nsnull, nsICommandLine::STATE_REMOTE_EXPLICIT);

            

            result = (HDDEDATA)DDE_FACK;
        } else {
            result = (HDDEDATA)DDE_NOTPROCESSED;
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

void nsNativeAppSupportOS2::ParseDDEArg( const char* args, int index, nsCString& aString) {
    if ( args ) {
        int argLen = strlen(args);
        nsDependentCString temp(args, argLen);

        
        PRInt32 offset = -1;
        
        while( index-- ) {
            
            offset = advanceToEndOfQuotedArg( args, offset, argLen);
            
            offset = temp.FindChar( ',', offset );
            if ( offset == kNotFound ) {
                
                aString = args;
                return;
            }
        }
        
        
        
        
        
        
        
        
        PRInt32 end = advanceToEndOfQuotedArg( args, offset++, argLen );
        
        end = temp.FindChar( ',', end );
        if ( end == kNotFound ) {
            
            end = argLen;
        }
        
        aString.Assign( args + offset, end - offset );
    }
    return;
}


void nsNativeAppSupportOS2::ParseDDEArg( HSZ args, int index, nsCString& aString) {
    DWORD argLen = WinDdeQueryString( args, NULL, NULL, CP_WINANSI );
    
    if ( !argLen ) return;
    nsCAutoString temp;
    
    temp.SetLength( argLen );
    
    WinDdeQueryString( args, temp.BeginWriting(), temp.Length(), CP_WINANSI );
    
    ParseDDEArg(temp.get(), index, aString);
    return;
}

void nsNativeAppSupportOS2::ActivateLastWindow() {
    nsCOMPtr<nsIDOMWindowInternal> navWin;
    GetMostRecentWindow( NS_LITERAL_STRING("navigator:browser").get(), getter_AddRefs( navWin ) );
    if ( navWin )
        
        activateWindow( navWin );
    else
        
        OpenBrowserWindow();
}

HDDEDATA nsNativeAppSupportOS2::CreateDDEData( DWORD value ) {
    return CreateDDEData( (LPBYTE)&value, sizeof value );
}

HDDEDATA nsNativeAppSupportOS2::CreateDDEData( LPBYTE value, DWORD len ) {
    HDDEDATA result = WinDdeCreateDataHandle( value,
                                              len,
                                              0,
                                              mApplication,
                                              CF_TEXT,
                                              0 );
    return result;
}


void
nsNativeAppSupportOS2::HandleCommandLine(const char* aCmdLineString,
                                         nsIFile* aWorkingDir,
                                         PRUint32 aState)
{
    nsresult rv;

    int justCounting = 1;
    char **argv = 0;
    
    int init = 1;
    int between, quoted, bSlashCount;
    int argc;
    const char *p;
    nsCAutoString arg;

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
            
            

            
            
            
            nsDependentCString mailtoUrlScheme (kMailtoUrlScheme);

            if ( *p == 0 || ( !quoted && isspace( *p ) && !StringBeginsWith(arg, mailtoUrlScheme, nsCaseInsensitiveCStringComparator()) ) ) {
                
                
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

    
    
    PRBool found;
    cmdLine->HandleFlag(NS_LITERAL_STRING("console"), PR_FALSE, &found);
    cmdLine->HandleFlag(NS_LITERAL_STRING("dde"), PR_FALSE, &found);

    
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
nsNativeAppSupportOS2::OpenWindow( const char*urlstr, const char *args ) {

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
nsNativeAppSupportOS2::OpenBrowserWindow()
{
    nsresult rv = NS_OK;

    
    

    
    

    nsCOMPtr<nsIDOMWindowInternal> navWin;
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
            nsCOMPtr<nsIDOMWindow> rootWin( do_GetInterface( rootItem ) );
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

        NS_ERROR("failed to hand off external URL to extant window\n");
    } while ( PR_FALSE );

    

    char* argv[] = { 0 };
    nsCOMPtr<nsICommandLineRunner> cmdLine
        (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
    NS_ENSURE_TRUE(cmdLine, NS_ERROR_FAILURE);

    rv = cmdLine->Init(0, argv, nsnull, nsICommandLine::STATE_REMOTE_EXPLICIT);
    NS_ENSURE_SUCCESS(rv, rv);

    return cmdLine->Run();
}









PRBool     StartOS2App(int aArgc, char **aArgv)
{
  PRBool    rv = PR_TRUE;
  PPIB      ppib;
  PTIB      ptib;

  DosGetInfoBlocks(&ptib, &ppib);

  
  
  if (ppib->pib_ultype != SSF_TYPE_PM)
    ppib->pib_ultype = SSF_TYPE_PM;
  else {
    for (int i = 1; i < aArgc; i++ ) {
      char *arg = aArgv[i];
      if (*arg != '-' && *arg != '/')
        continue;
      arg++;
      if (stricmp("?", arg) == 0 ||
        stricmp("h", arg) == 0 ||
        stricmp("v", arg) == 0 ||
        stricmp("help", arg) == 0 ||
        stricmp("version", arg) == 0 ||
        stricmp("console", arg) == 0) {
        rv = PR_FALSE;
        break;
      }
    }
  }

  
  
  if (rv) {
    ULONG    ulMaxFH = 0;
    LONG     ulReqCount = 0;

    DosSetRelMaxFH(&ulReqCount, &ulMaxFH);
    if (ulMaxFH < 256)
      DosSetMaxFH(256);

    return rv;
  }

  
  char        szErrObj[64] = "";
  STARTDATA   x;

  memset(&x, 0, sizeof(x));
  x.Length = sizeof(x);
  (const char* const)(x.PgmTitle) = gAppData->name;
  x.InheritOpt = SSF_INHERTOPT_PARENT;
  x.SessionType = SSF_TYPE_WINDOWABLEVIO;
  x.PgmControl = SSF_CONTROL_NOAUTOCLOSE;
  x.ObjectBuffer = szErrObj;
  x.ObjectBuffLen = sizeof(szErrObj);

  
  
  char * ptr = ppib->pib_pchcmd - 2;
  while (*ptr)
    ptr--;
  x.PgmName = ptr + 1;
  x.PgmInputs = strchr(ppib->pib_pchcmd, 0) + 1;

  
  
  
  
  ULONG ulSession;
  PID   pid;
  ULONG rc = DosStartSession(&x, &ulSession, &pid);
  if (rc && rc != ERROR_SMG_START_IN_BACKGROUND)
    rv = PR_TRUE;

  return rv;
}

