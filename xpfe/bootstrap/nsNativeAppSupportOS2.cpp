








































#ifdef MOZ_OS2_HIGH_MEMORY

#include <os2safe.h>
#endif

#define INCL_PM
#define INCL_GPI
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "nsStringSupport.h"

#include "nsNativeAppSupportBase.h"
#include "nsNativeAppSupportOS2.h"
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
#include "nsIAppShellService.h"
#include "nsIAppStartup.h"
#include "nsIProfileInternal.h"
#include "nsIXULWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPromptService.h"
#include "nsNetCID.h"
#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsIObserverService.h"
#include "nsXPFEComponentsCID.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"


#include "nsIDOMLocation.h"
#include "nsIJSContextStack.h"
#include "nsIWindowMediator.h"

#include "nsPaletteOS2.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include "prprf.h"


extern char **__argv;
extern int   *__pargc;





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




#define MOZ_DEBUG_DDE 0

#ifdef DEBUG_law
#undef MOZ_DEBUG_DDE
#define MOZ_DEBUG_DDE 1
#endif

class nsSplashScreenOS2 : public nsISplashScreen {
public:
    nsSplashScreenOS2();
    ~nsSplashScreenOS2();

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
    static nsSplashScreenOS2* GetPointer( HWND dlg );

    HWND mDlg;
    HBITMAP mBitmap;
    nsrefcnt mRefCnt;
    HDC hdcMemory;
    HPS hpsMemory;
    LONG mBitmapCX;
    LONG mBitmapCY;
}; 

MRESULT EXPENTRY DialogProc( HWND dlg, ULONG msg, MPARAM mp1, MPARAM mp2 );
void ThreadProc (void *splashScreen);


struct Mutex {
    Mutex( const char *name )
        : mName( name ),
          mHandle( 0 ),
          mState( 0xFFFF ) {
        
        mName.Insert("\\SEM32\\", 0);
        APIRET rc = DosCreateMutexSem(mName.get(), &mHandle, 0, FALSE);
        if (rc != NO_ERROR) {
#if MOZ_DEBUG_DDE
            printf( "CreateMutex error = 0x%08X\n", (int)rc );
#endif
        }
    }
    ~Mutex() {
        if ( mHandle ) {
            
            Unlock();


            APIRET rc = DosCloseMutexSem(mHandle);
            if (rc != NO_ERROR) {
#if MOZ_DEBUG_DDE
                printf( "CloseHandle error = 0x%08X\n", (int)rc );
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


















































































class nsNativeAppSupportOS2 : public nsNativeAppSupportBase
{
public:
    
    NS_IMETHOD Start( PRBool *aResult );
    NS_IMETHOD Stop( PRBool *aResult );
    NS_IMETHOD Quit();
    NS_IMETHOD StartServerMode();
    NS_IMETHOD OnLastWindowClosing();
    NS_IMETHOD SetIsServerMode( PRBool isServerMode );
    NS_IMETHOD EnsureProfile(nsICmdLineService* args);

    
    NS_IMETHOD StartDDE();

    
    
    
    
    
    PRBool CheckConsole();

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
    static char *mAppName;
    static PRBool mInitialWindowActive;
    static PRBool mForceProfileStartup;
    static char mMutexName[];
    static PRBool mUseDDE;
    friend struct MessageWindow;
}; 

nsSplashScreenOS2::nsSplashScreenOS2()
    : mDlg( 0 ), mBitmap( 0 ), mRefCnt( 0 ),
      hdcMemory( 0 ), hpsMemory( 0 ), mBitmapCX(0), mBitmapCY(0) {
}

nsSplashScreenOS2::~nsSplashScreenOS2() {
#if MOZ_DEBUG_DDE
    printf( "splash screen dtor called\n" );
#endif
    
    Hide();
}

NS_IMETHODIMP
nsSplashScreenOS2::Show() {
    
    _beginthread( ThreadProc, NULL, 16384, (void *)this );
    return NS_OK;
}

NS_IMETHODIMP
nsSplashScreenOS2::Hide() {
    if ( mDlg ) {
        
        WinPostMsg(mDlg, WM_CLOSE, 0, 0);
        mDlg = 0;
        GpiSetBitmap(hpsMemory, NULLHANDLE);
        if (mBitmap) {
            GpiDeleteBitmap(mBitmap);
            mBitmap = 0;
        }
        if (hdcMemory) {
            DevCloseDC(hdcMemory);
            hdcMemory = 0;
        }
        if (hpsMemory) {
           GpiDestroyPS(hpsMemory);
           hpsMemory = 0;
        }
    }
    return NS_OK;
}

HBITMAP LoadAndSetBitmapFromFile(HPS hps, PSZ pszFilename)
{
   FILE *fp = fopen(pszFilename, "rb");
   if (fp == NULL) {
      return NULLHANDLE;
   }
   fseek(fp, 0, SEEK_END );
   ULONG cbFile = ftell(fp);
   if (cbFile ==0) {
      fclose(fp);
      return NULLHANDLE;
   }
   fseek(fp, 0, SEEK_SET );
   PBYTE pBitmapData = (PBYTE)malloc(cbFile);
   fread((PVOID)pBitmapData, cbFile, 1, fp);
   fclose(fp);

   PBITMAPFILEHEADER2 pbfh2 = (PBITMAPFILEHEADER2)pBitmapData;
   PBITMAPINFOHEADER2 pbmp2 = NULL;

   switch (pbfh2->usType)
   {
      case BFT_BITMAPARRAY:
         




         pbfh2 = &(((PBITMAPARRAYFILEHEADER2) pBitmapData)->bfh2);
         pbmp2 = &pbfh2->bmp2;
         break;
      case BFT_BMAP:
         pbmp2 = &pbfh2->bmp2;
         break;
      case BFT_ICON:
      case BFT_POINTER:
      case BFT_COLORICON:
      case BFT_COLORPOINTER:
      default:
         break;
   }
    
   if (pbmp2 == NULL) {
      free(pBitmapData);
      return NULLHANDLE;
   }

   LONG lScans;
   if (pbmp2->cbFix == sizeof(BITMAPINFOHEADER))
      lScans = (LONG) ((PBITMAPINFOHEADER)pbmp2)->cy;
   else
      lScans = pbmp2->cy;

   HBITMAP hbmp = GpiCreateBitmap(hps, pbmp2, 0L, NULL, NULL);
   if (!hbmp) {
      free(pBitmapData);
      return NULLHANDLE;
   }

   if (GpiSetBitmap(hps, hbmp) == HBM_ERROR) {
      GpiDeleteBitmap(hbmp);
      free(pBitmapData);
      return NULLHANDLE;
   }

   LONG lScansSet = GpiSetBitmapBits(hps, 0L, lScans, pBitmapData + pbfh2->offBits,
                                     (PBITMAPINFO2) pbmp2);
   free(pBitmapData);

   if (lScansSet != lScans) {
      GpiSetBitmap(hps, NULLHANDLE);
      GpiDeleteBitmap(hbmp);
      return NULLHANDLE;
   }

   return hbmp;
}

void
nsSplashScreenOS2::LoadBitmap() {
    hdcMemory = DevOpenDC((HAB)0, OD_MEMORY, "*", 0L, NULL, 0L);
    SIZEL sizel = {0, 0};
    hpsMemory = GpiCreatePS((HAB)0, hdcMemory, &sizel,
                            PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC );

    
    PPIB ppib;
    PTIB ptib;
    char fileName[CCHMAXPATH];
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName( ppib->pib_hmte, CCHMAXPATH, fileName);
    int fileNameLen = strlen(fileName);
    if (fileNameLen >=3) {
        fileName[ fileNameLen - 3 ] = 0;
        strcat( fileName, "bmp" );
        
        mBitmap = LoadAndSetBitmapFromFile(hpsMemory, fileName);
    }
    if (!mBitmap) {
        mBitmap = GpiLoadBitmap(hpsMemory, NULL, IDB_SPLASH, 0L, 0L);
        GpiSetBitmap(hpsMemory, mBitmap);
    }
    BITMAPINFOHEADER bitmap;
    bitmap.cbFix = sizeof(BITMAPINFOHEADER);
    GpiQueryBitmapParameters(mBitmap, &bitmap);
    mBitmapCX = bitmap.cx;
    mBitmapCY = bitmap.cy;
}

MRESULT EXPENTRY DialogProc( HWND dlg, ULONG msg, MPARAM mp1, MPARAM mp2 ) {
    if ( msg == WM_INITDLG ) {
        
        nsSplashScreenOS2 *splashScreen = (nsSplashScreenOS2*)mp2;
        if ( mp2 ) {
            splashScreen->SetDialog( dlg );

            
            splashScreen->LoadBitmap();
        }

        


 
        HBITMAP hbitmap = splashScreen->mBitmap;
        if ( hbitmap ) {
            WinSetWindowPos( dlg,
                             HWND_TOP,
                             WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN )/2 - splashScreen->mBitmapCX/2,
                             WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN )/2 - splashScreen->mBitmapCY/2,
                             splashScreen->mBitmapCX,
                             splashScreen->mBitmapCY,
                             SWP_ACTIVATE | SWP_MOVE | SWP_SIZE );
            WinShowWindow( dlg, TRUE );
        }
        return (MRESULT)FALSE;
    }
    else if ( msg == WM_PAINT ) {
        nsSplashScreenOS2 *splashScreen = (nsSplashScreenOS2*)WinQueryWindowPtr( dlg, QWL_USER );
        HPS hps = WinBeginPaint (dlg, NULLHANDLE, NULL);
#if 0
        nsPaletteOS2::SelectGlobalPalette(hps, dlg);
#endif
        GpiErase (hps);
        POINTL aptl[8] = {{0, 0}, {splashScreen->mBitmapCX, splashScreen->mBitmapCY},
                          {0, 0}, {0, 0},
                          {0, 0}, {0, 0},
                          {0, 0}, {0, 0}};

        GpiBitBlt( hps, splashScreen->hpsMemory, 3L, aptl, ROP_SRCCOPY, 0L );
        WinEndPaint( hps );
        return (MRESULT)TRUE;
    }
#if 0
    else if ( msg == WM_REALIZEPALETTE ) {
        HPS hps = WinGetPS(dlg);
        nsPaletteOS2::SelectGlobalPalette(hps, dlg);
        WinReleasePS(hps);
        WinInvalidateRect( dlg, 0, TRUE);
        return (MRESULT)TRUE;
    }
#endif
    return WinDefDlgProc (dlg, msg, mp1, mp2);
}

void nsSplashScreenOS2::SetDialog( HWND dlg ) {
    
    mDlg = dlg;
    
    WinSetWindowPtr( mDlg, QWL_USER, this );
}

nsSplashScreenOS2 *nsSplashScreenOS2::GetPointer( HWND dlg ) {
    
    PVOID data = WinQueryWindowPtr( dlg, QWL_USER );
    return (nsSplashScreenOS2*)data;
}

void ThreadProc(void *splashScreen) {
    HAB hab = WinInitialize( 0 );
    HMQ hmq = WinCreateMsgQueue( hab, 0 );
    WinDlgBox( HWND_DESKTOP, HWND_DESKTOP, (PFNWP)DialogProc, NULLHANDLE, IDD_SPLASH, (MPARAM)splashScreen );
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

}

#define TURBOD "mozturbo.exe"

PRBool gAbortServer = PR_FALSE;

PRBool
nsNativeAppSupportOS2::CheckConsole() {
    CHAR pszAppPath[CCHMAXPATH];
    PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte, CCHMAXPATH, pszAppPath);
    *strrchr(pszAppPath, '\\') = '\0'; 

    for ( int i = 1; i < *__pargc; i++ ) {
        if ( strcmp( "-turbo", __argv[i] )  == 0 ||
             strcmp( "/turbo", __argv[i] )  == 0 ||
             strcmp( "-server", __argv[i] ) == 0 ||
             strcmp( "/server", __argv[i] ) == 0 ) {         

            struct stat st;
            CHAR pszTurboPath[CCHMAXPATH];

            strcpy(pszTurboPath, pszAppPath);
            strcat(pszTurboPath, "\\");
            strcat(pszTurboPath, TURBOD);
            int statrv = stat(pszTurboPath, &st);

            
            if (statrv == 0) {
              RESULTCODES rcodes;
              CHAR pszArgString[CCHMAXPATH];

              strcpy(pszArgString, pszTurboPath);
              strcat(pszArgString, " -l -p ");
              strcat(pszArgString, pszAppPath);
              pszArgString[strlen(pszTurboPath)] = '\0';
       
              DosExecPgm(NULL,0,EXEC_BACKGROUND,
                         pszArgString,
                         0, &rcodes,
                         pszTurboPath);
              return PR_FALSE; 
            } else {
              
              mServerMode = PR_TRUE;
              mShouldShowUI = PR_FALSE;
              __argv[i] = "-nosplash"; 
              
              break;
            }
        }
    }

    PRBool checkTurbo = PR_TRUE;
    for ( int j = 1; j < *__pargc; j++ ) {
        if (strcmp("-killAll", __argv[j]) == 0 || strcmp("/killAll", __argv[j]) == 0 ||
            strcmp("-kill", __argv[j]) == 0 || strcmp("/kill", __argv[j]) == 0) {

            struct stat st;
            CHAR pszTurboPath[CCHMAXPATH];

            strcpy(pszTurboPath, pszAppPath);
            strcat(pszTurboPath, "\\");
            strcat(pszTurboPath, TURBOD);
            int statrv = stat(pszTurboPath, &st);

            
            if (statrv == 0) {
              RESULTCODES rcodes;
              CHAR pszArgString[CCHMAXPATH];

              strcpy(pszArgString, pszTurboPath);
              strcat(pszArgString, " -u");
              pszArgString[strlen(pszTurboPath)] = '\0';
             
              DosExecPgm(NULL,0,EXEC_BACKGROUND,
                         pszArgString,
                         0, &rcodes,
                         pszTurboPath);
              return PR_FALSE; 
            } else {
              gAbortServer = PR_TRUE;
            }
            break;
        }

        if ( strcmp( "-silent", __argv[j] ) == 0 || strcmp( "/silent", __argv[j] ) == 0 ) {
            checkTurbo = PR_FALSE;
        }
    }

    
    
    if ( checkTurbo && !mServerMode ) {
#if 0
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
#endif
    }

    return PR_TRUE; 
}



nsresult
NS_CreateNativeAppSupport( nsINativeAppSupport **aResult ) {
    if ( aResult ) {
        nsNativeAppSupportOS2 *pNative = new nsNativeAppSupportOS2;
        if ( pNative ) {                                           
            
            
            
            if (pNative->CheckConsole() == PR_FALSE) {
               delete pNative;
               return NS_ERROR_FAILURE;
            }
            *aResult = pNative;                                    
            NS_ADDREF( *aResult );                                 
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
    
    
    
    
    BOOL doSplashScreen = TRUE;
    if ( aResult ) {
        *aResult = 0;
        CHAR pBuffer[3];
        PrfQueryProfileString( HINI_USERPROFILE, "PM_ControlPanel", "LogoDisplayTime", "1", pBuffer, 3);
        if (pBuffer[0] == '0') {
          doSplashScreen = FALSE;
        } 
        for ( int i = 1; i < *__pargc; i++ ) {
            if ( strcmp( "-quiet", __argv[i] ) == 0
                 ||
                 strcmp( "/quiet", __argv[i] ) == 0 ) {
                 doSplashScreen = FALSE;
            }
            if ( strcmp( "-splash", __argv[i] ) == 0
                 ||
                 strcmp( "/splash", __argv[i] ) == 0 ) {
                 doSplashScreen = TRUE;
            }
        }
        if (!doSplashScreen) {
          return NS_OK;
        }
        *aResult = new nsSplashScreenOS2;
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


int   nsNativeAppSupportOS2::mConversations = 0;
HSZ   nsNativeAppSupportOS2::mApplication   = 0;
HSZ   nsNativeAppSupportOS2::mTopics[nsNativeAppSupportOS2::topicCount] = { 0 };
DWORD nsNativeAppSupportOS2::mInstance      = 0;
PRBool nsNativeAppSupportOS2::mInitialWindowActive = PR_FALSE;
PRBool nsNativeAppSupportOS2::mForceProfileStartup = PR_FALSE;



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
                         nsNativeAppSupportOS2::mAppName,
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

    
    NS_IMETHOD SendRequest( const char *cmd ) {
        






        COPYDATASTRUCT *pcds;
        APIRET rc = NO_ERROR;
        PVOID pvData = NULL;
        ULONG ulSize = sizeof(COPYDATASTRUCT)+strlen(cmd)+1;
#ifdef MOZ_OS2_HIGH_MEMORY
        rc = DosAllocSharedMem( &pvData, NULL, ulSize,
                                PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE | OBJ_ANY);
	if( rc != NO_ERROR ) 
	{
          
          
          
          rc = DosAllocSharedMem( &pvData, NULL, ulSize,
                                  PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE);
	}
#else
        rc = DosAllocSharedMem( &pvData, NULL, ulSize,
                                PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GETTABLE);
#endif


        if( rc != NO_ERROR )
        {
           


           return NS_OK;
        }

        memset( pvData, '\0', ulSize );
        pcds = (COPYDATASTRUCT *)(pvData);
        pcds->dwData = 0;
        pcds->cbData = strlen(cmd)+1;
        


        pcds->lpData = &(pcds->chBuff);
        if( cmd )
        {
           strcpy( (char *)pcds->lpData, cmd );
        }
        WinSendMsg( mHandle, WM_COPYDATA, 0, (MPARAM)pcds );
        DosFreeMem( pvData );

        return NS_OK;
    }

    
    static MRESULT EXPENTRY WindowProc( HWND msgWindow, ULONG msg, MPARAM wp, 
                                        MPARAM lp )
    {
        MRESULT rc = (MRESULT)TRUE;

        if ( msg == WM_COPYDATA ) {
            
            COPYDATASTRUCT *cds = (COPYDATASTRUCT*)lp;
            DosGetSharedMem( (PVOID)cds, PAG_READ|PAG_WRITE );
#if MOZ_DEBUG_DDE
            printf( "Incoming request: %s\n", (const char*)cds->lpData );
#endif
            
            nsCOMPtr<nsIDOMWindow> win;
            (void)nsNativeAppSupportOS2::HandleRequest( (LPBYTE)cds->lpData, PR_FALSE, getter_AddRefs( win ) );
            return win ? (MRESULT)hwndForDOMWindow( win ) : 0;
 }

    


    else if ( msg == WM_CREATE ) {
        rc = (MRESULT)FALSE;
    }

    return rc;
}

private:
    HWND     mHandle;
    USHORT   mMsgWindowAtom;
}; 

static char nameBuffer[128] = { 0 };
char *nsNativeAppSupportOS2::mAppName = nameBuffer;
PRBool nsNativeAppSupportOS2::mUseDDE = PR_FALSE;













NS_IMETHODIMP
nsNativeAppSupportOS2::Start( PRBool *aResult ) {
    NS_ENSURE_ARG( aResult );
    NS_ENSURE_TRUE( mInstance == 0, NS_ERROR_NOT_INITIALIZED );

    nsresult rv = NS_ERROR_FAILURE;
    *aResult = PR_FALSE;

    
    
    
    for (int i = 1; i < *__pargc; i++ ) {
        if (stricmp("-dde", __argv[i]) == 0 ||
            stricmp("/dde", __argv[i]) == 0)
            mUseDDE = PR_TRUE;
        else
            if (stricmp("-console", __argv[i]) != 0 &&
                stricmp("/console", __argv[i]) != 0)
                continue;

        for (int j = i; j < *__pargc; j++)
            __argv[j] = __argv[j+1];

        (*__pargc)--;
        i--;
    }

    
    
    if (getenv("MOZ_NO_REMOTE")) {
        mUseDDE = PR_FALSE;
        *aResult = PR_TRUE;
        return NS_OK;
    }

    
    int retval;
    UINT id = ID_DDE_APPLICATION_NAME;
    retval = WinLoadString( NULLHANDLE, NULLHANDLE, id, sizeof(nameBuffer), nameBuffer );
    if ( retval == 0 ) {
        
        *aResult = PR_TRUE;
        return NS_OK;
    }

    
    PR_snprintf( mMutexName, sizeof mMutexName, "%s%s", nameBuffer, MOZ_STARTUP_MUTEX_NAME );
    Mutex startupLock = Mutex( mMutexName );

    NS_ENSURE_TRUE( startupLock.Lock( MOZ_DDE_START_TIMEOUT ), NS_ERROR_FAILURE );

    




    MQINFO mqinfo;
    HAB hab = NULLHANDLE;
    HMQ hmqCurrent = WinQueryQueueInfo( HMQ_CURRENT, &mqinfo, 
                                        sizeof( MQINFO ) );
    if( !hmqCurrent )
    {
        hab = WinInitialize( 0 );
        hmqCurrent = WinCreateMsgQueue( hab, 0 );
    }

    
    MessageWindow msgWindow;
    if ( msgWindow.getHWND() ) {
        
        char *cmd = GetCommandLine();
        rv = msgWindow.SendRequest( cmd );
    } else {
        
        if (!gAbortServer) {
            rv = msgWindow.Create();
            if ( NS_SUCCEEDED( rv ) ) {
                if (mUseDDE) {
                    
                    this->StartDDE();
                }
                
                *aResult = PR_TRUE;
            }
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

    
    NS_ENSURE_TRUE( ( mApplication = WinDdeCreateStringHandle( mAppName, CP_WINANSI ) ) && InitTopicStrings(),
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
nsNativeAppSupportOS2::Quit() {
    
    
    
    
    Mutex mutexLock(mMutexName);
    NS_ENSURE_TRUE(mutexLock.Lock(MOZ_DDE_START_TIMEOUT), NS_ERROR_FAILURE );

    
    
    
    
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

PRBool NS_CanRun()
{
      return PR_FALSE; 
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



static void escapeQuotes( nsString &aString ) {
    PRInt32 offset = -1;
    while( 1 ) {
       
       offset = FindCharInString( aString, '"', ++offset );
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
                    

                    
                    PRBool new_window = PR_FALSE;

                    
                    nsCAutoString url;
                    ParseDDEArg(hsz2, 0, url);
                    
                    
                    nsCAutoString windowID;
                    ParseDDEArg(hsz2, 2, windowID);
                    
                    if ( windowID.Equals( "0" ) ) {
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
                        
                        
                        outpt.Append( NS_LossyConvertUTF16toASCII( url ) );
                        
                        
                        outpt.Append( NS_LITERAL_CSTRING("\",\"") );
                        
                        outpt.Append( NS_LossyConvertUTF16toASCII( title ));
                        
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
            
            PRBool new_window = PR_FALSE;

            nsCAutoString url;
            ParseDDEArg((const char*) request, 0, url);

            
            
            nsCAutoString windowID;
            ParseDDEArg((const char*) request, 2, windowID);

            
            if ( windowID.Equals( "0" ) ) {
                new_window = PR_TRUE;
            }

            
            url.Insert( NS_STRINGIFY(MOZ_APP_NAME) " -url ", 0 );
#if MOZ_DEBUG_DDE
            printf( "Handling dde XTYP_REQUEST request: [%s]...\n", url.get() );
#endif
            
            nsCOMPtr<nsIDOMWindow> win;
            HandleRequest( LPBYTE( url.get() ), new_window, getter_AddRefs( win ) );

            

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
            
            offset = FindCharInString( temp, ',', offset );
            if ( offset == kNotFound ) {
                
                aString = args;
                return;
            }
        }
        
        
        
        
        
        
        
        
        PRInt32 end = advanceToEndOfQuotedArg( args, offset++, argLen );
        
        end = FindCharInString( temp, ',', end );
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
    
    char *temp = (char *) malloc(argLen + 1);
    if ( !temp ) return;
    
    WinDdeQueryString( args, temp, argLen + 1, CP_WINANSI );
    
    ParseDDEArg(temp, index, aString);
    free(temp);
}

void nsNativeAppSupportOS2::ActivateLastWindow() {
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







nsresult
nsNativeAppSupportOS2::HandleRequest( LPBYTE request, PRBool newWindow, nsIDOMWindow **aResult ) {

    
    

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
    if (NS_SUCCEEDED(rv) && (const char*)arg ) {
      
#if MOZ_DEBUG_DDE
      printf( "Launching chrome url [%s]...\n", (const char*)arg );
#endif
      rv = nativeApp->EnsureProfile(args);
      if (NS_SUCCEEDED(rv))
        rv = OpenWindow( arg.get(), EmptyString(), aResult );
      return rv;
    }

    
    

    rv = args->GetCmdLineValue( "-profilemanager", getter_Copies(arg));
    if ( NS_SUCCEEDED(rv) && (const char*)arg ) { 
      nsCOMPtr<nsIDOMWindowInternal> window;
      GetMostRecentWindow(0, getter_AddRefs(window));
      if (!window) { 
        mForceProfileStartup = PR_TRUE;
      }
    }

    
    rv = args->GetCmdLineValue( "-kill", getter_Copies(arg));
    if ( NS_SUCCEEDED(rv) && (const char*)arg ) {
      
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

    NS_ConvertUTF16toUTF8 url( defaultArgs );
    return OpenBrowserWindow(defaultArgs, newWindow, aResult);
}




nsresult
nsNativeAppSupportOS2::GetCmdLineArgs( LPBYTE request, nsICmdLineService **aResult ) {
    nsresult rv = NS_OK;

    int justCounting = 1;
    char **argv = 0;
    
    int between = 1, quoted = 0, bSlashCount = 0;
    int argc = 0;
    char *p = (char*)request;
    nsCAutoString arg;
    
    while ( 1 ) {
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
                p = (char*)request;
                between = 1;
                argc = quoted = bSlashCount = 0;
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
        printf( "Error creating command line service = 0x%08X (argc=%d, argv=0x%08X)\n", (int)rv, (int)argc, (int)argv );
#endif
    }

    
    while ( argc ) {
        delete [] argv[ --argc ];
    }
    delete [] argv;

    return rv;
}





nsresult
nsNativeAppSupportOS2::EnsureProfile(nsICmdLineService* args)
{
  nsresult rv;  

  nsCOMPtr<nsIProfileInternal> profileMgr(do_GetService(NS_PROFILE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIAppStartup> appStartup(do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
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
nsNativeAppSupportOS2::OpenWindow( const char *urlstr,
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
nsNativeAppSupportOS2::ReParent( nsISupports *window, HWND newParent ) {
    HWND hMainClient = hwndForDOMWindow( window );
    if ( !hMainClient ) {
        return NS_ERROR_FAILURE;
    }
    HWND hMainFrame = WinQueryWindow(hMainClient, QW_PARENT);

    
    WinSetParent( hMainFrame, newParent, FALSE );

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
nsNativeAppSupportOS2::OpenBrowserWindow( const nsAString& aArgs,
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







NS_IMETHODIMP
nsNativeAppSupportOS2::StartServerMode() {

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

    
    ReParent( newWindow, MessageWindow().getHWND() );

    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportOS2::SetIsServerMode( PRBool isServerMode ) {
    return nsNativeAppSupportBase::SetIsServerMode( isServerMode );
}

NS_IMETHODIMP
nsNativeAppSupportOS2::OnLastWindowClosing() {

    if ( !mServerMode )
        return NS_OK;

    
    
    if ( mInitialWindowActive ) {
        mInitialWindowActive = PR_FALSE;
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

        
        PPIB ppib;
        PTIB ptib;
        char filename[CCHMAXPATH];
        char buffer[CCHMAXPATH];
        DosGetInfoBlocks(&ptib, &ppib);
        DosQueryModuleName(ppib->pib_hmte, CCHMAXPATH, filename);
        strcpy(buffer, filename);
        
        strcat(buffer, " -turbo");

        
        RESULTCODES resultcodes;
        CHAR szLoadError[CCHMAXPATH];

        buffer[strlen(buffer)] = '\0';
        buffer[strlen(buffer)+1] = '\0';
        buffer[strlen(filename)] = '\0';

        DosExecPgm(szLoadError,
                         sizeof(szLoadError),
                         EXEC_ASYNCRESULT,
                         buffer,
                         NULL,
                         &resultcodes,
                         buffer);
#ifndef XP_OS2
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
#endif

        
        SetIsServerMode( PR_FALSE );
        appStartup->Quit(nsIAppStartup::eAttemptQuit);

        
    }
    return NS_OK;
}









PRBool     StartOS2App( int aArgc, char **aArgv)
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

      
      
      if (stricmp("turbo", arg) == 0 ||
        stricmp("server", arg)  == 0 ||
        stricmp("kill", arg)    == 0 ||
        stricmp("killall", arg) == 0) {
        rv = PR_TRUE;
        break;
      }
      else {
        if (stricmp("?", arg)  == 0 ||
          stricmp("h", arg)    == 0 ||
          stricmp("v", arg)    == 0 ||
          stricmp("help", arg) == 0 ||
          stricmp("version", arg) == 0 ||
          stricmp("console", arg) == 0)
          rv = PR_FALSE;
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

