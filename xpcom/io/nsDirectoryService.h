





































#ifndef nsDirectoryService_h___
#define nsDirectoryService_h___

#include "nsIDirectoryService.h"
#include "nsHashtable.h"
#include "nsILocalFile.h"
#include "nsISupportsArray.h"
#include "nsIAtom.h"

#define NS_XPCOM_INIT_CURRENT_PROCESS_DIR       "MozBinD"   // Can be used to set NS_XPCOM_CURRENT_PROCESS_DIR
                                                            
#define NS_DIRECTORY_SERVICE_CID  {0xf00152d0,0xb40b,0x11d3,{0x8c, 0x9c, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

class nsDirectoryService : public nsIDirectoryService,
                           public nsIProperties,
                           public nsIDirectoryServiceProvider2
{
  public:

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIPROPERTIES  

  NS_DECL_NSIDIRECTORYSERVICE

  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

  nsDirectoryService();
   ~nsDirectoryService();

  static nsresult RealInit();
  void RegisterCategoryProviders();

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  static nsDirectoryService* gService;

private:
    nsresult GetCurrentProcessDirectory(nsILocalFile** aFile);
    
    static PRBool PR_CALLBACK ReleaseValues(nsHashKey* key, void* data, void* closure);
    nsSupportsHashtable mHashtable;
    nsCOMPtr<nsISupportsArray> mProviders;

public:
    static nsIAtom *sCurrentProcess;
    static nsIAtom *sComponentRegistry;
    static nsIAtom *sComponentDirectory;
    static nsIAtom *sXPTIRegistry;
    static nsIAtom *sGRE_Directory;
    static nsIAtom *sGRE_ComponentDirectory;
    static nsIAtom *sOS_DriveDirectory;
    static nsIAtom *sOS_TemporaryDirectory;
    static nsIAtom *sOS_CurrentProcessDirectory;
    static nsIAtom *sOS_CurrentWorkingDirectory;
    static nsIAtom *sOS_DesktopDirectory;
    static nsIAtom *sOS_HomeDirectory;
#if defined (XP_MACOSX)
    static nsIAtom *sDirectory;
    static nsIAtom *sTrashDirectory;
    static nsIAtom *sStartupDirectory;
    static nsIAtom *sShutdownDirectory;
    static nsIAtom *sAppleMenuDirectory;
    static nsIAtom *sControlPanelDirectory;
    static nsIAtom *sExtensionDirectory;
    static nsIAtom *sFontsDirectory;
    static nsIAtom *sPreferencesDirectory;
    static nsIAtom *sDocumentsDirectory;
    static nsIAtom *sInternetSearchDirectory;
    static nsIAtom *sUserLibDirectory;
    static nsIAtom *sDefaultDownloadDirectory;
    static nsIAtom *sUserDesktopDirectory;
    static nsIAtom *sLocalDesktopDirectory;
    static nsIAtom *sUserApplicationsDirectory;
    static nsIAtom *sLocalApplicationsDirectory;
    static nsIAtom *sUserDocumentsDirectory;
    static nsIAtom *sLocalDocumentsDirectory;
    static nsIAtom *sUserInternetPlugInDirectory;
    static nsIAtom *sLocalInternetPlugInDirectory;
    static nsIAtom *sUserFrameworksDirectory;
    static nsIAtom *sLocalFrameworksDirectory;
    static nsIAtom *sUserPreferencesDirectory;
    static nsIAtom *sLocalPreferencesDirectory;
    static nsIAtom *sPictureDocumentsDirectory;
    static nsIAtom *sMovieDocumentsDirectory;
    static nsIAtom *sMusicDocumentsDirectory;
    static nsIAtom *sInternetSitesDirectory;
#elif defined (XP_WIN) 
    static nsIAtom *sSystemDirectory;
    static nsIAtom *sWindowsDirectory;
    static nsIAtom *sWindowsProgramFiles;
    static nsIAtom *sDesktop;
    static nsIAtom *sPrograms;
    static nsIAtom *sControls;
    static nsIAtom *sPrinters;
    static nsIAtom *sPersonal;
    static nsIAtom *sFavorites;
    static nsIAtom *sStartup;
    static nsIAtom *sRecent;
    static nsIAtom *sSendto;
    static nsIAtom *sBitbucket;
    static nsIAtom *sStartmenu;
    static nsIAtom *sDesktopdirectory;
    static nsIAtom *sDrives;
    static nsIAtom *sNetwork;
    static nsIAtom *sNethood;
    static nsIAtom *sFonts;
    static nsIAtom *sTemplates;
    static nsIAtom *sCommon_Startmenu;
    static nsIAtom *sCommon_Programs;
    static nsIAtom *sCommon_Startup;
    static nsIAtom *sCommon_Desktopdirectory;
    static nsIAtom *sAppdata;
    static nsIAtom *sLocalAppdata;
    static nsIAtom *sPrinthood;
    static nsIAtom *sWinCookiesDirectory;
    static nsIAtom *sDefaultDownloadDirectory;
#elif defined (XP_UNIX)
    static nsIAtom *sLocalDirectory;
    static nsIAtom *sLibDirectory;
#elif defined (XP_OS2)
    static nsIAtom *sSystemDirectory;
    static nsIAtom *sOS2Directory;
#elif defined (XP_BEOS)
    static nsIAtom *sSettingsDirectory;
    static nsIAtom *sSystemDirectory;
#endif


};


#endif

