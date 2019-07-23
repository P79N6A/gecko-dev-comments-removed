










































#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsNativeCharsetUtils.h"
#include "nsStringEnumerator.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsHashtable.h"
#include "nsIFileChannel.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsThreadUtils.h"

#include "nsNetUtil.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "prmem.h"
#include "plstr.h"
#include "prprf.h"
#include "nsCRT.h"

#include "VerReg.h"

#include "nsInstall.h"
#include "nsInstallFolder.h"
#include "nsInstallVersion.h"
#include "nsInstallFile.h"
#include "nsInstallExecute.h"
#include "nsInstallPatch.h"
#include "nsInstallUninstall.h"
#include "nsInstallResources.h"
#include "nsXPIProxy.h"
#include "nsRegisterItem.h"
#include "nsNetUtil.h"
#include "ScheduledTasks.h"
#include "nsIPersistentProperties2.h"

#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#ifdef _WINDOWS
#include "nsWinReg.h"
#include "nsWinProfile.h"
#endif

#include "nsInstallFileOpEnums.h"
#include "nsInstallFileOpItem.h"

#ifdef XP_MACOSX
#include <Gestalt.h>
#include "nsAppleSingleDecoder.h"
#include "nsILocalFileMac.h"
#endif

#include "nsILocalFile.h"
#include "nsIURL.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <sys/utsname.h>
#endif

#if defined(XP_WIN)
#include <windows.h>
#endif

#if defined(XP_OS2)
#define INCL_DOSMISC
#include <os2.h>
#endif

#define kInstallLocaleProperties "chrome://global/locale/commonDialogs.properties"







static void
NS_SoftwareUpdateRequestAutoReg()
{
  nsresult rv;
  nsCOMPtr<nsIFile> file;

  if (nsSoftwareUpdate::GetProgramDirectory())
    
    nsSoftwareUpdate::GetProgramDirectory()->Clone(getter_AddRefs(file));
  else
    NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                           getter_AddRefs(file));

  if (!file) {
    NS_WARNING("Getting NS_XPCOM_CURRENT_PROCESS_DIR failed");
    return;
  }

  file->AppendNative(nsDependentCString(".autoreg"));
#ifdef DEBUG_timeless
  PRBool condition;
  if (NS_SUCCEEDED(file->IsDirectory(&condition)) && condition) {
    
    return;
  }
#endif

  
  
  

  file->Remove(PR_FALSE);
  rv = file->Create(nsIFile::NORMAL_FILE_TYPE, 0666);

  if (NS_FAILED(rv)) {
    NS_WARNING("creating file failed");
    return;
  }
}



nsInstallInfo::nsInstallInfo(PRUint32           aInstallType,
                             nsIFile*           aFile,
                             const PRUnichar*   aURL,
                             const PRUnichar*   aArgs,
                             nsIPrincipal*      aPrincipal,
                             PRUint32           flags,
                             nsIXPIListener*    aListener)
: mPrincipal(aPrincipal),
  mError(0),
  mType(aInstallType),
  mFlags(flags),
  mURL(aURL),
  mArgs(aArgs),
  mFile(aFile),
  mListener(aListener)
{
    MOZ_COUNT_CTOR(nsInstallInfo);

    nsresult rv;

    

    nsCOMPtr<nsIThread> thread = do_GetMainThread();

    NS_WITH_ALWAYS_PROXIED_SERVICE(CHROMEREG_IFACE, cr,
                                   NS_CHROMEREGISTRY_CONTRACTID,
                                   thread, &rv);
    if (NS_SUCCEEDED(rv)) {
      mChromeRegistry = cr;

      nsCAutoString spec;
      rv = NS_GetURLSpecFromFile(aFile, spec);
      if (NS_SUCCEEDED(rv)) {
        spec.Insert(NS_LITERAL_CSTRING("jar:"), 0);
        spec.AppendLiteral("!/");
#ifdef MOZ_XUL_APP
        NS_NewURI(getter_AddRefs(mFileJARURL), spec);
#else
        mFileJARSpec.Assign(spec);
#endif
      }
    }

#ifdef MOZ_XUL_APP
    NS_WITH_ALWAYS_PROXIED_SERVICE(nsIExtensionManager, em,
                                   "@mozilla.org/extensions/manager;1",
                                   thread, &rv);
    if (NS_SUCCEEDED(rv))
      mExtensionManager = em;

    nsCOMPtr<nsIFile> manifest;
    rv = NS_GetSpecialDirectory(NS_APP_CHROME_DIR, getter_AddRefs(manifest));
    if (NS_SUCCEEDED(rv))
      NS_NewFileURI(getter_AddRefs(mManifestURL), manifest);
#endif
}


nsInstallInfo::~nsInstallInfo()
{
  MOZ_COUNT_DTOR(nsInstallInfo);
}

static NS_DEFINE_IID(kSoftwareUpdateCID,  NS_SoftwareUpdate_CID);


nsInstall::nsInstall(nsIZipReader * theJARFile)
{
    MOZ_COUNT_CTOR(nsInstall);

    mScriptObject           = nsnull;           
    mVersionInfo            = nsnull;           
    mInstalledFiles         = nsnull;           


    mPatchList              = nsnull;
    mUninstallPackage       = PR_FALSE;
    mRegisterPackage        = PR_FALSE;
    mFinalStatus            = SUCCESS;
    mStartInstallCompleted  = PR_FALSE;
    mJarFileLocation        = nsnull;
    
    mPackageFolder          = nsnull;

    
    mJarFileData = theJARFile;

    nsISoftwareUpdate *su;
    nsresult rv = CallGetService(kSoftwareUpdateCID, &su);

    if (NS_SUCCEEDED(rv))
    {
        su->GetMasterListener( getter_AddRefs(mListener) );
    }

    su->Release();

    nsCOMPtr<nsIThread> thread = do_GetMainThread();

    
    mStringBundle = nsnull;
    NS_WITH_PROXIED_SERVICE( nsIStringBundleService,
                             service,
                             NS_STRINGBUNDLE_CONTRACTID,
                             thread,
                             &rv );

    if (NS_SUCCEEDED(rv) && service)
    {
        rv = service->CreateBundle( XPINSTALL_BUNDLE_URL,
                                    getter_AddRefs(mStringBundle) );
    }
}

nsInstall::~nsInstall()
{
    if (mVersionInfo != nsnull)
        delete mVersionInfo;

    if (mPackageFolder)
        delete mPackageFolder;

    MOZ_COUNT_DTOR(nsInstall);
}

PRInt32
nsInstall::SetScriptObject(void *aScriptObject)
{
  mScriptObject = (JSObject*) aScriptObject;
  return NS_OK;
}

#ifdef _WINDOWS
PRInt32
nsInstall::SaveWinRegPrototype(void *aScriptObject)
{
  mWinRegObject = (JSObject*) aScriptObject;
  return NS_OK;
}
PRInt32
nsInstall::SaveWinProfilePrototype(void *aScriptObject)
{
  mWinProfileObject = (JSObject*) aScriptObject;
  return NS_OK;
}

JSObject*
nsInstall::RetrieveWinRegPrototype()
{
  return mWinRegObject;
}

JSObject*
nsInstall::RetrieveWinProfilePrototype()
{
  return mWinProfileObject;
}
#endif


PRInt32
nsInstall::GetInstallPlatform(nsCString& aPlatform)
{
  if (mInstallPlatform.IsEmpty())
  {
    
    

    
#if defined(XP_WIN)
    mInstallPlatform = "Windows";
#elif defined(XP_MACOSX)
    mInstallPlatform = "Macintosh";
#elif defined (XP_UNIX)
    mInstallPlatform = "X11";
#elif defined(XP_BEOS)
    mInstallPlatform = "BeOS";
#elif defined(XP_OS2)
    mInstallPlatform = "OS/2";
#endif

    mInstallPlatform += "; ";

    
#if defined(XP_WIN)
    OSVERSIONINFO info = { sizeof(OSVERSIONINFO) };
    if (GetVersionEx(&info)) {
        if ( info.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
            if (info.dwMajorVersion      == 3) {
                mInstallPlatform += "WinNT3.51";
            }
            else if (info.dwMajorVersion == 4) {
                mInstallPlatform += "WinNT4.0";
            }
            else if (info.dwMajorVersion == 5) {
                mInstallPlatform += "Windows NT 5.0";
            }
            else {
                mInstallPlatform += "WinNT";
            }
        } else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (info.dwMinorVersion == 90)
                mInstallPlatform += "Win 9x 4.90";
            else if (info.dwMinorVersion > 0)
                mInstallPlatform += "Win98";
            else
                mInstallPlatform += "Win95";
        }
    }
#elif defined (XP_UNIX) || defined (XP_BEOS)
    struct utsname name;

    int ret = uname(&name);
    if (ret >= 0) {
       mInstallPlatform +=  (char*)name.sysname;
       mInstallPlatform += ' ';
       mInstallPlatform += (char*)name.release;
       mInstallPlatform += ' ';
       mInstallPlatform += (char*)name.machine;
    }
#elif defined(XP_MACOSX)
    mInstallPlatform += "PPC";
#elif defined(XP_OS2)
    ULONG os2ver = 0;
    DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR,
                    &os2ver, sizeof(os2ver));
    if (os2ver == 11)
        mInstallPlatform += "2.11";
    else if (os2ver == 30)
        mInstallPlatform += "Warp 3";
    else if (os2ver == 40)
        mInstallPlatform += "Warp 4";
    else if (os2ver == 45)
        mInstallPlatform += "Warp 4.5";
    else
        mInstallPlatform += "Warp ???";
#endif
  }

  aPlatform = mInstallPlatform;
  return NS_OK;
}


void
nsInstall::InternalAbort(PRInt32 errcode)
{
    mFinalStatus = errcode;

    nsInstallObject* ie;
    if (mInstalledFiles != nsnull)
    {
        
        
        for (PRInt32 i = mInstalledFiles->Count()-1; i >= 0; i--)
        {
            ie = (nsInstallObject *)mInstalledFiles->ElementAt(i);
            if (ie)
                ie->Abort();
        }
    }

    CleanUp();
}

PRInt32
nsInstall::AbortInstall(PRInt32 aErrorNumber)
{
    InternalAbort(aErrorNumber);
    return NS_OK;
}

PRInt32
nsInstall::AddDirectory(const nsString& aRegName,
                        const nsString& aVersion,
                        const nsString& aJarSource,
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir,
                        PRInt32 aMode,
                        PRInt32* aReturn)
{
    nsInstallFile* ie = nsnull;
    PRInt32 result;

    if ( aJarSource.IsEmpty() || aFolder == nsnull )
    {
        *aReturn = SaveError(nsInstall::INVALID_ARGUMENTS);
        return NS_OK;
    }

    result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    
    nsString qualifiedRegName;
    result = GetQualifiedRegName( aRegName.IsEmpty() ? aJarSource : aRegName,
                                  qualifiedRegName);

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString qualifiedVersion = aVersion;
    if (qualifiedVersion.IsEmpty())
    {
        
        *aReturn = mVersionInfo->ToString(qualifiedVersion);

        if (NS_FAILED(*aReturn))
        {
            SaveError( nsInstall::UNEXPECTED_ERROR );
            return NS_OK;
        }
    }

    nsAutoString subdirectory(aSubdir);
    if (!subdirectory.IsEmpty())
        subdirectory.AppendLiteral("/");

    nsAutoString prefix(aJarSource + NS_LITERAL_STRING("/"));
    const PRInt32 prefix_length = prefix.Length();
    NS_LossyConvertUTF16toASCII pattern(prefix + NS_LITERAL_STRING("*"));

    nsCOMPtr<nsIUTF8StringEnumerator> jarEnum;
    nsresult rv = mJarFileData->FindEntries(pattern.get(),
                                            getter_AddRefs(jarEnum) );
    if (NS_FAILED(rv) || !jarEnum)
    {
        *aReturn = SaveError( nsInstall::EXTRACTION_FAILED );
        return NS_OK;
    }

    PRInt32 count = 0;
    PRBool bMore = PR_FALSE;
    while (NS_SUCCEEDED(jarEnum->HasMore(&bMore)) && bMore)
    {
        nsCAutoString name;
        rv = jarEnum->GetNext(name);
        if (NS_FAILED(rv))
        {
            result = nsInstall::EXTRACTION_FAILED;
            break;
        }

        if ( name.Last() == '/' )
        {
            
            continue;
        }
        const PRInt32 namelen = name.Length();
        NS_ASSERTION( prefix_length <= namelen, "Match must be longer than pattern!" );
        NS_ConvertASCIItoUTF16 fileName(Substring(name,
                                                  prefix_length,
                                                  namelen - prefix_length));
        nsAutoString newJarSource(prefix + fileName);
        nsAutoString newSubDir(subdirectory + fileName);

        ie = new nsInstallFile( this,
                                qualifiedRegName,
                                qualifiedVersion,
                                newJarSource,
                                aFolder,
                                newSubDir,
                                aMode,
                                (count == 0), 
                                &result);

        if (ie == nsnull)
        {
            result = nsInstall::OUT_OF_MEMORY;
        }
        else if (result != nsInstall::SUCCESS)
        {
            delete ie;
        }
        else
        {
            result = ScheduleForInstall( ie );
            count++;
        }
    }

    if (result == nsInstall::SUCCESS && count == 0) 
        result = nsInstall::DOES_NOT_EXIST;

    *aReturn = SaveError( result );
    return NS_OK;
}

PRInt32
nsInstall::AddDirectory(const nsString& aRegName,
                        const nsString& aVersion,
                        const nsString& aJarSource,
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir,
                        PRInt32* aReturn)
{
    return AddDirectory(aRegName,
                        aVersion,
                        aJarSource,
                        aFolder,
                        aSubdir,
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32
nsInstall::AddDirectory(const nsString& aRegName,
                        const nsString& aJarSource,
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir,
                        PRInt32* aReturn)
{
    return AddDirectory(aRegName,
                        EmptyString(),
                        aJarSource,
                        aFolder,
                        aSubdir,
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32
nsInstall::AddDirectory(const nsString& aJarSource,
                        PRInt32* aReturn)
{
    if(mPackageFolder == nsnull)
    {
        *aReturn = SaveError( nsInstall::PACKAGE_FOLDER_NOT_SET );
        return NS_OK;
    }

    return AddDirectory(EmptyString(),
                        EmptyString(),
                        aJarSource,
                        mPackageFolder,
                        EmptyString(),
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32
nsInstall::AddSubcomponent(const nsString& aRegName,
                           const nsString& aVersion,
                           const nsString& aJarSource,
                           nsInstallFolder *aFolder,
                           const nsString& aTargetName,
                           PRInt32 aMode,
                           PRInt32* aReturn)
{
    nsInstallFile*  ie;
    nsString        qualifiedRegName;
    nsString        qualifiedVersion = aVersion;
    nsString        tempTargetName   = aTargetName;

    PRInt32         errcode = nsInstall::SUCCESS;


    if(aJarSource.IsEmpty() || aFolder == nsnull )
    {
        *aReturn = SaveError( nsInstall::INVALID_ARGUMENTS );
        return NS_OK;
    }

    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    if( aTargetName.IsEmpty() )
    {
        PRInt32 pos = aJarSource.RFindChar('/');

        if ( pos == kNotFound )
            tempTargetName = aJarSource;
        else
            aJarSource.Right(tempTargetName, aJarSource.Length() - (pos+1));
    }

    if (qualifiedVersion.IsEmpty())
        qualifiedVersion.AssignLiteral("0.0.0.0");

    
    result = GetQualifiedRegName( aRegName.IsEmpty() ? aJarSource : aRegName,
                                  qualifiedRegName);

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }


    ie = new nsInstallFile( this,
                            qualifiedRegName,
                            qualifiedVersion,
                            aJarSource,
                            aFolder,
                            tempTargetName,
                            aMode,
                            PR_TRUE,
                            &errcode );

    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (errcode == nsInstall::SUCCESS)
    {
        errcode = ScheduleForInstall( ie );
    }
    else
    {
        delete ie;
    }

    *aReturn = SaveError( errcode );
    return NS_OK;
}

PRInt32
nsInstall::AddSubcomponent(const nsString& aRegName,
                           const nsString& aVersion,
                           const nsString& aJarSource,
                           nsInstallFolder* aFolder,
                           const nsString& aTargetName,
                           PRInt32* aReturn)
{
    return AddSubcomponent(aRegName,
                           aVersion,
                           aJarSource,
                           aFolder,
                           aTargetName,
                           INSTALL_NO_COMPARE,
                           aReturn);
}

PRInt32
nsInstall::AddSubcomponent(const nsString& aRegName,
                           const nsString& aJarSource,
                           nsInstallFolder* aFolder,
                           const nsString& aTargetName,
                           PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString version;
    *aReturn = mVersionInfo->ToString(version);

    if (NS_FAILED(*aReturn))
    {
        SaveError( nsInstall::UNEXPECTED_ERROR );
        return NS_OK;
    }

    return AddSubcomponent(aRegName,
                           version,
                           aJarSource,
                           aFolder,
                           aTargetName,
                           INSTALL_NO_COMPARE,
                           aReturn);
}

PRInt32
nsInstall::AddSubcomponent(const nsString& aJarSource,
                           PRInt32* aReturn)
{
    if(mPackageFolder == nsnull)
    {
        *aReturn = SaveError( nsInstall::PACKAGE_FOLDER_NOT_SET );
        return NS_OK;
    }
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString version;
    *aReturn = mVersionInfo->ToString(version);

    if (NS_FAILED(*aReturn))
    {
        SaveError( nsInstall::UNEXPECTED_ERROR );
        return NS_OK;
    }

    return AddSubcomponent(EmptyString(),
                           version,
                           aJarSource,
                           mPackageFolder,
                           EmptyString(),
                           INSTALL_NO_COMPARE,
                           aReturn);
}


PRInt32
nsInstall::DiskSpaceAvailable(const nsString& aFolder, PRInt64* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        double d = SaveError( result );
        LL_L2D(d, *aReturn);
        return NS_OK;
    }
    nsCOMPtr<nsILocalFile> folder;
    NS_NewLocalFile(aFolder, PR_TRUE, getter_AddRefs(folder));

    result = folder->GetDiskSpaceAvailable(aReturn);
    return NS_OK;
}

PRInt32
nsInstall::Execute(const nsString& aJarSource, const nsString& aArgs, PRBool aBlocking, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsInstallExecute* ie = new nsInstallExecute(this, aJarSource, aArgs, aBlocking, &result);

    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS)
    {
        result = ScheduleForInstall( ie );
    }

    *aReturn = SaveError(result);
    return NS_OK;
}


PRInt32
nsInstall::FinalizeInstall(PRInt32* aReturn)
{
    PRInt32 result = SUCCESS;
    PRBool  rebootNeeded = PR_FALSE;

    *aReturn = SanityCheck();

    if (*aReturn != nsInstall::SUCCESS)
    {
        SaveError( *aReturn );
        mFinalStatus = *aReturn;
        return NS_OK;
    }


    if ( mInstalledFiles->Count() > 0 )
    {
        if ( mUninstallPackage )
        {
            VR_UninstallCreateNode( const_cast<char *>(NS_ConvertUTF16toUTF8(mRegistryPackageName).get()),
                                    const_cast<char *>(NS_ConvertUTF16toUTF8(mUIName).get()));
        }

        
        if (mVersionInfo)
        {
            nsString versionString;
            nsCString path;

            mVersionInfo->ToString(versionString);
            nsCAutoString versionCString;
            versionCString.AssignWithConversion(versionString);

            if (mPackageFolder)
                mPackageFolder->GetDirectoryPath(path);

            VR_Install( const_cast<char *>(NS_ConvertUTF16toUTF8(mRegistryPackageName).get()),
                        const_cast<char *>(path.get()),
                        const_cast<char *>(versionCString.get()),
                        PR_TRUE );
        }

        nsInstallObject* ie = nsnull;

        for (PRInt32 i=0; i < mInstalledFiles->Count(); i++)
        {
            ie = (nsInstallObject*)mInstalledFiles->ElementAt(i);
            NS_ASSERTION(ie, "NULL object in install queue!");
            if (ie == NULL)
                continue;

            if (mListener)
            {
                char *objString = ie->toString();
                if (objString)
                {
                    mListener->OnFinalizeProgress(
                                    NS_ConvertASCIItoUTF16(objString).get(),
                                    (i+1), mInstalledFiles->Count());
                    delete [] objString;
                }
            }

            result = ie->Complete();

            if (result != nsInstall::SUCCESS)
            {
                if ( result == REBOOT_NEEDED )
                {
                    rebootNeeded = PR_TRUE;
                    result = SUCCESS;
                }
                else
                {
                    InternalAbort( result );
                    break;
                }
            }
        }

        if ( result == SUCCESS )
        {
            if ( rebootNeeded )
                *aReturn = SaveError( REBOOT_NEEDED );

            if ( nsSoftwareUpdate::mNeedCleanup )
            {
                
                
                
                
                nsPIXPIProxy* proxy = GetUIThreadProxy();
                if (proxy)
                    proxy->NotifyRestartNeeded();
            }

            
            
            NS_SoftwareUpdateRequestAutoReg();
        }
        else
            *aReturn = SaveError( result );

        mFinalStatus = *aReturn;
    }
    else
    {
        
        

        mFinalStatus = *aReturn;
    }

    CleanUp();

    return NS_OK;
}

#ifdef XP_MACOSX
#define GESTALT_CHAR_CODE(x)          (((unsigned long) ((x[0]) & 0x000000FF)) << 24) \
                                    | (((unsigned long) ((x[1]) & 0x000000FF)) << 16) \
                                    | (((unsigned long) ((x[2]) & 0x000000FF)) << 8)  \
                                    | (((unsigned long) ((x[3]) & 0x000000FF)))
#endif

PRInt32
nsInstall::Gestalt(const nsString& aSelector, PRInt32* aReturn)
{
    *aReturn = nsnull;

    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
#ifdef XP_MACOSX

    long    response = 0;
    char    selectorChars[4];
    int     i;
    OSErr   err = noErr;
    OSType  selector;

    if (aSelector.IsEmpty())
    {
        return NS_OK;
    }

    for (i=0; i<4; i++)
        selectorChars[i] = aSelector.CharAt(i);
    selector = GESTALT_CHAR_CODE(selectorChars);

    err = ::Gestalt(selector, &response);

    if (err != noErr)
        *aReturn = err;
    else
        *aReturn = response;

#endif 
    return NS_OK;
}

PRInt32
nsInstall::GetComponentFolder(const nsString& aComponentName, const nsString& aSubdirectory, nsInstallFolder** aNewFolder)
{
    long        err;
    char        dir[MAXREGPATHLEN];
    nsresult    res = NS_OK;

    if(!aNewFolder)
        return INVALID_ARGUMENTS;

    *aNewFolder = nsnull;


    nsString tempString;

    if ( GetQualifiedPackageName(aComponentName, tempString) != SUCCESS )
    {
        return NS_OK;
    }

    NS_ConvertUTF16toUTF8 componentCString(tempString);

    if((err = VR_GetDefaultDirectory( const_cast<char *>(componentCString.get()), sizeof(dir), dir )) != REGERR_OK)
    {
        
        
        if((err = VR_GetPath( const_cast<char *>(componentCString.get()), sizeof(dir), dir )) != REGERR_OK)
        {
          
          *dir = '\0';
        }
    }

    nsCOMPtr<nsILocalFile> componentDir;
    nsCOMPtr<nsIFile> componentIFile;
    if(*dir != '\0')
        NS_NewNativeLocalFile( nsDependentCString(dir), PR_FALSE, getter_AddRefs(componentDir) );

    if ( componentDir )
    {
        PRBool isFile;

        res = componentDir->IsFile(&isFile);
        if (NS_SUCCEEDED(res) && isFile)
            componentDir->GetParent(getter_AddRefs(componentIFile));
        else
            componentIFile = do_QueryInterface(componentDir);

        nsInstallFolder * folder = new nsInstallFolder();
        if (!folder)
            return NS_ERROR_OUT_OF_MEMORY;

        res = folder->Init(componentIFile, aSubdirectory);
        if (NS_FAILED(res))
        {
            delete folder;
        }
        else
        {
            *aNewFolder = folder;
        }
    }

    return res;
}

PRInt32
nsInstall::GetComponentFolder(const nsString& aComponentName, nsInstallFolder** aNewFolder)
{
    return GetComponentFolder(aComponentName, EmptyString(), aNewFolder);
}

PRInt32
nsInstall::GetFolder(const nsString& targetFolder, const nsString& aSubdirectory, nsInstallFolder** aNewFolder)
{
    
    if (!aNewFolder)
        return INVALID_ARGUMENTS;

    * aNewFolder = nsnull;

    nsInstallFolder* folder = new nsInstallFolder();
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(targetFolder, aSubdirectory);

    if (NS_FAILED(res))
    {
        delete folder;
        return res;
    }
    *aNewFolder = folder;
    return NS_OK;
}

PRInt32
nsInstall::GetFolder(const nsString& targetFolder, nsInstallFolder** aNewFolder)
{
    
    return GetFolder(targetFolder, EmptyString(), aNewFolder);
}

PRInt32
nsInstall::GetFolder( nsInstallFolder& aTargetFolderObj, const nsString& aSubdirectory, nsInstallFolder** aNewFolder )
{
    
    if (!aNewFolder)
        return INVALID_ARGUMENTS;

    * aNewFolder = nsnull;

    nsInstallFolder* folder = new nsInstallFolder();
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(aTargetFolderObj, aSubdirectory);

    if (NS_FAILED(res))
    {
        delete folder;
        return res;
    }
    *aNewFolder = folder;
    return NS_OK;
}




PRInt32
nsInstall::GetLastError(PRInt32* aReturn)
{
    *aReturn = mLastError;
    return NS_OK;
}

PRInt32
nsInstall::GetWinProfile(const nsString& aFolder, const nsString& aFile, JSContext* jscontext, JSClass* WinProfileClass, jsval* aReturn)
{
    *aReturn = JSVAL_NULL;

    if (SanityCheck() != nsInstall::SUCCESS)
    {
        return NS_OK;
    }

#ifdef _WINDOWS
    JSObject*     winProfileObject;
    nsWinProfile* nativeWinProfileObject = new nsWinProfile(this, aFolder, aFile);

    if (nativeWinProfileObject == nsnull)
        return NS_OK;

    JSObject*     winProfilePrototype    = this->RetrieveWinProfilePrototype();

    winProfileObject = JS_NewObject(jscontext, WinProfileClass, winProfilePrototype, NULL);
    if(winProfileObject == NULL)
        return NS_OK;

    JS_SetPrivate(jscontext, winProfileObject, nativeWinProfileObject);

    *aReturn = OBJECT_TO_JSVAL(winProfileObject);
#endif 

    return NS_OK;
}

PRInt32
nsInstall::GetWinRegistry(JSContext* jscontext, JSClass* WinRegClass, jsval* aReturn)
{
    *aReturn = JSVAL_NULL;

    if (SanityCheck() != nsInstall::SUCCESS)
    {
        return NS_OK;
    }

#ifdef _WINDOWS
    JSObject* winRegObject;
    nsWinReg* nativeWinRegObject = new nsWinReg(this);

    if (nativeWinRegObject == nsnull)
        return NS_OK;

    JSObject* winRegPrototype    = this->RetrieveWinRegPrototype();

    winRegObject = JS_NewObject(jscontext, WinRegClass, winRegPrototype, NULL);
    if(winRegObject == NULL)
        return NS_OK;

    JS_SetPrivate(jscontext, winRegObject, nativeWinRegObject);

    *aReturn = OBJECT_TO_JSVAL(winRegObject);
#endif 

    return NS_OK;
}

PRInt32
nsInstall::LoadResources(JSContext* cx, const nsString& aBaseName, jsval* aReturn)
{
    *aReturn = JSVAL_NULL;
 
    if (SanityCheck() != nsInstall::SUCCESS)
    {
        return NS_OK;
    }
    nsresult ret;
    nsCOMPtr<nsIFile> resFile;
    nsIURI *url = nsnull;
    nsIStringBundleService* service = nsnull;
    nsIStringBundle* bundle = nsnull;
    nsCOMPtr<nsISimpleEnumerator> propEnum;
    jsval v = JSVAL_NULL;

    
    JS_GetProperty( cx, JS_GetGlobalObject( cx ), "Object", &v );
    if (!v)
    {
        return NS_ERROR_NULL_POINTER;
    }
    JSClass *objclass = JS_GetClass( cx, JSVAL_TO_OBJECT(v) );
    JSObject *res = JS_NewObject( cx, objclass, JSVAL_TO_OBJECT(v), 0 );

    
    
    PRInt32 err = ExtractFileFromJar(aBaseName, nsnull, getter_AddRefs(resFile));
    if ( (!resFile) || (err != nsInstall::SUCCESS)  )
    {
        SaveError( err );
        return NS_OK;
    }

    
    ret = CallGetService(NS_STRINGBUNDLE_CONTRACTID, &service);
    if (NS_FAILED(ret))
        goto cleanup;

    
#if 1
    {
      nsCAutoString spec;
      ret = NS_GetURLSpecFromFile(resFile, spec);
      if (NS_FAILED(ret)) {
        NS_WARNING("cannot get url spec\n");
        NS_RELEASE(service);
        return ret;
      }
      ret = service->CreateBundle(spec.get(), &bundle);
    }
#else
    ret = service->CreateBundle(url, &bundle);
#endif
    if (NS_FAILED(ret))
        goto cleanup;
    ret = bundle->GetSimpleEnumeration(getter_AddRefs(propEnum));
    if (NS_FAILED(ret))
        goto cleanup;

    
    
    PRBool hasMore;
    while (NS_SUCCEEDED(propEnum->HasMoreElements(&hasMore)) && hasMore)
    {
        nsCOMPtr<nsISupports> nextProp;
        ret = propEnum->GetNext(getter_AddRefs(nextProp));
        if (NS_FAILED(ret))
            goto cleanup;

        nsCOMPtr<nsIPropertyElement> propElem =
            do_QueryInterface(nextProp);
        if (!propElem)
            continue;

        nsAutoString pVal;
        nsCAutoString pKey;
        ret = propElem->GetKey(pKey);
        if (NS_FAILED(ret))
            goto cleanup;
        ret = propElem->GetValue(pVal);
        if (NS_FAILED(ret))
            goto cleanup;

        if (!pKey.IsEmpty() && !pVal.IsEmpty())
        {
            JSString* propValJSStr = JS_NewUCStringCopyZ(cx, reinterpret_cast<const jschar*>(pVal.get()));
            jsval propValJSVal = STRING_TO_JSVAL(propValJSStr);
            NS_ConvertUTF8toUTF16 UCKey(pKey);
            JS_SetUCProperty(cx, res, (jschar*)UCKey.get(), UCKey.Length(), &propValJSVal);
        }
    }

    *aReturn = OBJECT_TO_JSVAL(res);
    ret = nsInstall::SUCCESS;

cleanup:
    SaveError( ret );

    
    NS_IF_RELEASE( service );

    
    NS_IF_RELEASE( url );
    NS_IF_RELEASE( bundle );

    return NS_OK;
}

PRInt32
nsInstall::Patch(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString qualifiedRegName;

    *aReturn = GetQualifiedRegName( aRegName, qualifiedRegName);

    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }

    if (!mPatchList)
    {
        mPatchList = new nsHashtable();
        if (mPatchList == nsnull)
        {
            *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
            return NS_OK;
        }
    }

    nsInstallPatch* ip = new nsInstallPatch( this,
                                             qualifiedRegName,
                                             aVersion,
                                             aJarSource,
                                             aFolder,
                                             aTargetName,
                                             &result);

    if (ip == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS)
    {
        result = ScheduleForInstall( ip );
    }

    *aReturn = SaveError(result);
    return NS_OK;
}

PRInt32
nsInstall::Patch(const nsString& aRegName, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn)
{
    return Patch(aRegName, EmptyString(), aJarSource, aFolder, aTargetName, aReturn);
}

PRInt32
nsInstall::RegisterChrome(nsIFile* chrome, PRUint32 chromeType, const char* path)
{
    PRInt32 result = SanityCheck();
    if (result != SUCCESS)
        return SaveError( result );

    if (!chrome || !chromeType)
        return SaveError( INVALID_ARGUMENTS );

    nsRegisterItem* ri = new nsRegisterItem(this, chrome, chromeType, path);
    if (ri == nsnull)
        return SaveError(OUT_OF_MEMORY);
    else
        return SaveError(ScheduleForInstall( ri ));
}

nsPIXPIProxy* nsInstall::GetUIThreadProxy()
{
    if (!mUIThreadProxy)
    {
        nsresult rv;
        nsCOMPtr<nsPIXPIProxy> tmp(do_QueryInterface(new nsXPIProxy()));
        rv = NS_GetProxyForObject( NS_PROXY_TO_MAIN_THREAD,
                                   NS_GET_IID(nsPIXPIProxy), tmp,
                                   NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                   getter_AddRefs(mUIThreadProxy) );
    }

    return mUIThreadProxy;
}

PRInt32
nsInstall::RefreshPlugins(PRBool aReloadPages)
{
    nsPIXPIProxy* proxy = GetUIThreadProxy();
    if (!proxy)
        return UNEXPECTED_ERROR;

    return proxy->RefreshPlugins(aReloadPages);
}


PRInt32
nsInstall::ResetError(PRInt32 aError)
{
    mLastError = aError;
    return SUCCESS;
}

PRInt32
nsInstall::SetPackageFolder(nsInstallFolder& aFolder)
{
    if (mPackageFolder)
        delete mPackageFolder;

    nsInstallFolder* folder = new nsInstallFolder();
    if (folder == nsnull)
    {
        return OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(aFolder, EmptyString());

    if (NS_FAILED(res))
    {
        delete folder;
        return UNEXPECTED_ERROR;
    }
    mPackageFolder = folder;
    return SUCCESS;
}


PRInt32
nsInstall::StartInstall(const nsString& aUserPackageName, const nsString& aRegistryPackageName, const nsString& aVersion, PRInt32* aReturn)
{
    if ( aUserPackageName.IsEmpty() )
    {
        
        *aReturn = SaveError(INVALID_ARGUMENTS);
        return NS_OK;
    }

    char szRegPackagePath[MAXREGPATHLEN];

    *szRegPackagePath = '0';
    *aReturn   = nsInstall::SUCCESS;

    ResetError(nsInstall::SUCCESS);

    mUserCancelled = PR_FALSE;

    mUIName = aUserPackageName;

    *aReturn = GetQualifiedPackageName( aRegistryPackageName, mRegistryPackageName );

    if (*aReturn != nsInstall::SUCCESS)
    {
        SaveError( *aReturn );
        return NS_OK;
    }

    
    if (mVersionInfo != nsnull)
        delete mVersionInfo;

    mVersionInfo    = new nsInstallVersion();
    if (mVersionInfo == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }
    mVersionInfo->Init(aVersion);

    
    mInstalledFiles = new nsVoidArray();

    if (mInstalledFiles == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    
    if (mPackageFolder != nsnull)
        delete mPackageFolder;

    mPackageFolder = nsnull;
    if(REGERR_OK == VR_GetDefaultDirectory(
                        const_cast<char *>(NS_ConvertUTF16toUTF8(mRegistryPackageName).get()),
                        sizeof(szRegPackagePath), szRegPackagePath))
    {
        
        mPackageFolder = new nsInstallFolder();
        nsCOMPtr<nsILocalFile> packageDir;
        NS_NewNativeLocalFile(
                            nsDependentCString(szRegPackagePath), 
                            PR_FALSE, getter_AddRefs(packageDir) );

        if (mPackageFolder && packageDir)
        {
            if (NS_FAILED( mPackageFolder->Init(packageDir, EmptyString()) ))
            {
                delete mPackageFolder;
                mPackageFolder = nsnull;
            }
        }
    }

    
    
    
    
    mStartInstallCompleted = PR_TRUE;
    mFinalStatus = MALFORMED_INSTALL;
    if (mListener)
        mListener->OnPackageNameSet(mInstallURL.get(), mUIName.get(), aVersion.get());

    return NS_OK;
}


PRInt32
nsInstall::Uninstall(const nsString& aRegistryPackageName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString qualifiedPackageName;

    *aReturn = GetQualifiedPackageName( aRegistryPackageName, qualifiedPackageName );

    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }

    nsInstallUninstall *ie = new nsInstallUninstall( this,
                                                     qualifiedPackageName,
                                                     &result );

    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS)
    {
        result = ScheduleForInstall( ie );
    }
    else
    {
        delete ie;
    }

    *aReturn = SaveError(result);

    return NS_OK;
}




void
nsInstall::AddPatch(nsHashKey *aKey, nsIFile* fileName)
{
    if (mPatchList != nsnull)
    {
        mPatchList->Put(aKey, fileName);
    }
}

void
nsInstall::GetPatch(nsHashKey *aKey, nsIFile** fileName)
{
    if (!fileName)
        return;
    else
        *fileName = nsnull;

    if (mPatchList)
    {
        NS_IF_ADDREF(*fileName = (nsIFile*) mPatchList->Get(aKey));
    }
}

PRInt32
nsInstall::FileOpDirCreate(nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_CREATE, localFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirGetParent(nsInstallFolder& aTarget, nsInstallFolder** theParentFolder)
{
  nsCOMPtr<nsIFile> parent;
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  nsresult rv = localFile->GetParent(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv) && parent)
  {
    nsInstallFolder* folder = new nsInstallFolder();
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
      folder->Init(parent, EmptyString());
      *theParentFolder = folder;
  }
  else
      theParentFolder = nsnull;

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirRemove(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_REMOVE, localFile, aFlags, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirRename(nsInstallFolder& aSrc, nsString& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aSrc.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_RENAME, localFile, aTarget, PR_FALSE, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileCopy(nsInstallFolder& aSrc, nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localSrcFile = aSrc.GetFileSpec();
  if (localSrcFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsCOMPtr<nsIFile>localTargetFile = aTarget.GetFileSpec();
  if (localTargetFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_COPY, localSrcFile, localTargetFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileDelete(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_DELETE, localFile, aFlags, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileExecute(nsInstallFolder& aTarget, nsString& aParams, PRBool aBlocking, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_EXECUTE, localFile, aParams, aBlocking, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileExists(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->Exists(aReturn);
  return NS_OK;
}

#ifdef XP_WIN
#include <winver.h>
#endif

PRInt32
nsInstall::FileOpFileGetNativeVersion(nsInstallFolder& aTarget, nsString* aReturn)
{
  PRInt32           rv = NS_OK;

#ifdef XP_WIN
  PRBool            flagExists;
  nsCOMPtr<nsILocalFile> localTarget(do_QueryInterface(aTarget.GetFileSpec()));
  UINT              uLen;
  UINT              dwLen;
  DWORD             dwHandle;
  LPVOID            lpData;
  LPVOID            lpBuffer;
  VS_FIXEDFILEINFO  *lpBuffer2;
  DWORD             dwMajor   = 0;
  DWORD             dwMinor   = 0;
  DWORD             dwRelease = 0;
  DWORD             dwBuild   = 0;
  nsCAutoString     nativeTargetPath;
  char              *nativeVersionString = nsnull;

  if(localTarget == nsnull)
    return(rv);

  flagExists = PR_FALSE;
  localTarget->Exists(&flagExists);
  if(flagExists)
  {
    localTarget->GetNativePath(nativeTargetPath);
    uLen   = 0;
    

    dwLen  = GetFileVersionInfoSize((char *)nativeTargetPath.get(), &dwHandle);
    lpData = (LPVOID)PR_Malloc(sizeof(long)*dwLen);
    if(!lpData)
      return(nsInstall::OUT_OF_MEMORY);

    

    if(GetFileVersionInfo((char *)nativeTargetPath.get(), dwHandle, dwLen, lpData) != 0)
    {
      if(VerQueryValue(lpData, "\\", &lpBuffer, &uLen) != 0)
      {
        lpBuffer2 = (VS_FIXEDFILEINFO *)lpBuffer;
        dwMajor   = HIWORD(lpBuffer2->dwFileVersionMS);
        dwMinor   = LOWORD(lpBuffer2->dwFileVersionMS);
        dwRelease = HIWORD(lpBuffer2->dwFileVersionLS);
        dwBuild   = LOWORD(lpBuffer2->dwFileVersionLS);
      }
    }

    nativeVersionString = PR_smprintf("%d.%d.%d.%d", dwMajor, dwMinor, dwRelease, dwBuild);
    if(!nativeVersionString)
      rv = nsInstall::OUT_OF_MEMORY;
    else
    {
      aReturn->AssignASCII(nativeVersionString);
      PR_smprintf_free(nativeVersionString);
    }

    PR_FREEIF(lpData);
  }
#endif

  return(rv);
}

PRInt32
nsInstall::FileOpFileGetDiskSpaceAvailable(nsInstallFolder& aTarget, PRInt64* aReturn)
{
  nsresult rv;
  nsCOMPtr<nsIFile> file = aTarget.GetFileSpec();
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);

  localFile->GetDiskSpaceAvailable(aReturn);  
  return NS_OK;
}


PRInt32
nsInstall::FileOpFileGetModDate(nsInstallFolder& aTarget, double* aReturn)
{
    * aReturn = 0;

    nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

    if (localFile)
    {
        double newStamp;
        PRInt64 lastModDate = LL_ZERO;
        localFile->GetLastModifiedTime(&lastModDate);

        LL_L2D(newStamp, lastModDate);

        *aReturn = newStamp;
    }

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileGetSize(nsInstallFolder& aTarget, PRInt64* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->GetFileSize(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileIsDirectory(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->IsDirectory(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileIsWritable(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->IsWritable(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileIsFile(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->IsFile(aReturn);
  return NS_OK;
}


PRInt32
nsInstall::FileOpFileModDateChanged(nsInstallFolder& aTarget, double aOldStamp, PRBool* aReturn)
{
    *aReturn = PR_TRUE;

    nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
    if (localFile)
    {
        double newStamp;
        PRInt64 lastModDate = LL_ZERO;
        localFile->GetLastModifiedTime(&lastModDate);

        LL_L2D(newStamp, lastModDate);

        *aReturn = !(newStamp == aOldStamp);
    }
    return NS_OK;
}

PRInt32
nsInstall::FileOpFileMove(nsInstallFolder& aSrc, nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localSrcFile = aSrc.GetFileSpec();
  if (localSrcFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }
  nsCOMPtr<nsIFile> localTargetFile = aTarget.GetFileSpec();
  if (localTargetFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_MOVE, localSrcFile, localTargetFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileRename(nsInstallFolder& aSrc, nsString& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aSrc.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_RENAME, localFile, aTarget, PR_FALSE, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

#ifdef _WINDOWS
#include <winbase.h>
#endif


PRInt32
nsInstall::FileOpFileWindowsGetShortName(nsInstallFolder& aTarget, nsString& aShortPathName)
{
#ifdef _WINDOWS

  PRInt32             err;
  PRBool              flagExists;
  nsString            tmpNsString;
  nsCAutoString       nativeTargetPath;
  nsAutoString        unicodePath;
  char                nativeShortPathName[MAX_PATH];
  nsCOMPtr<nsIFile>   localTarget(aTarget.GetFileSpec());

  if(localTarget == nsnull)
    return NS_OK;

  localTarget->Exists(&flagExists);
  if(flagExists)
  {
    memset(nativeShortPathName, 0, MAX_PATH);
    localTarget->GetNativePath(nativeTargetPath);

    err = GetShortPathName(nativeTargetPath.get(), nativeShortPathName, MAX_PATH);
    if((err > 0) && (*nativeShortPathName == '\0'))
    {
      
      
      
      char *nativeShortPathNameTmp = new char[err + 1];
      if(nativeShortPathNameTmp == nsnull)
        return NS_OK;

      err = GetShortPathName(nativeTargetPath.get(), nativeShortPathNameTmp, err + 1);
      
      
      
      
      
      if(err != 0)
      {
        
        NS_CopyNativeToUnicode(nsDependentCString(nativeShortPathNameTmp), unicodePath);
      }

      if(nativeShortPathNameTmp)
        delete [] nativeShortPathNameTmp;
    }
    else if(err != 0)
    {
      
      NS_CopyNativeToUnicode(nsDependentCString(nativeShortPathName), unicodePath);
    }
  }

  if (!unicodePath.IsEmpty())
    aShortPathName = unicodePath;

#endif

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileWindowsShortcut(nsIFile* aTarget, nsIFile* aShortcutPath, nsString& aDescription, nsIFile* aWorkingPath, nsString& aParams, nsIFile* aIcon, PRInt32 aIconId, PRInt32* aReturn)
{

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_WIN_SHORTCUT, aTarget, aShortcutPath, aDescription, aWorkingPath, aParams, aIcon, aIconId, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileMacAlias(nsIFile *aSourceFile, nsIFile *aAliasFile, PRInt32* aReturn)
{

  *aReturn = nsInstall::SUCCESS;

#ifdef XP_MACOSX

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_MAC_ALIAS, aSourceFile, aAliasFile, aReturn);
  if (!ifop)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

#endif 

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileUnixLink(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  return NS_OK;
}

PRInt32
nsInstall::FileOpWinRegisterServer(nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_WIN_REGISTER_SERVER, localFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS)
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

void
nsInstall::LogComment(const nsAString& aComment)
{
  if(mListener)
    mListener->OnLogComment(PromiseFlatString(aComment).get());
}











PRInt32
nsInstall::ScheduleForInstall(nsInstallObject* ob)
{
    PRInt32 error = nsInstall::SUCCESS;

    char *objString = ob->toString();

    

    if (mListener)
        mListener->OnItemScheduled(NS_ConvertASCIItoUTF16(objString).get());


    
    error = ob->Prepare();

    if (error == nsInstall::SUCCESS)
    {
        
        mInstalledFiles->AppendElement( ob );

        
        

        if (ob->CanUninstall())
            mUninstallPackage = PR_TRUE;

        if (ob->RegisterPackageNode())
            mRegisterPackage = PR_TRUE;
    }
    else if ( mListener )
    {
        
        char* errRsrc = GetResourcedString(NS_LITERAL_STRING("ERROR"));
        if (errRsrc)
        {
            char* errprefix = PR_smprintf("%s (%d): ", errRsrc, error);
            nsString errstr; errstr.AssignWithConversion(errprefix);
            errstr.AppendWithConversion(objString);

            mListener->OnLogComment( errstr.get() );

            PR_smprintf_free(errprefix);
            nsCRT::free(errRsrc);
        }
    }

    if (error != SUCCESS)
        SaveError(error);

    if (objString)
        delete [] objString;

    return error;
}








PRInt32
nsInstall::SanityCheck(void)
{
    if ( mInstalledFiles == nsnull || mStartInstallCompleted == PR_FALSE )
    {
        return INSTALL_NOT_STARTED;
    }

    if (mUserCancelled)
    {
        InternalAbort(USER_CANCELLED);
        return USER_CANCELLED;
    }

    return 0;
}








PRInt32
nsInstall::GetQualifiedPackageName( const nsString& name, nsString& qualifiedName )
{
    nsString startOfName;
    name.Left(startOfName, 7);

    if ( startOfName.EqualsLiteral("=USER=/") )
    {
        CurrentUserNode(qualifiedName);
        qualifiedName += name;
    }
    else
    {
        qualifiedName = name;
    }

    if (BadRegName(qualifiedName))
    {
        return BAD_PACKAGE_NAME;
    }


    

    if (qualifiedName.Last() == '/')
    {
        PRInt32 index = qualifiedName.Length();
        qualifiedName.Truncate(--index);
    }

    return SUCCESS;
}








PRInt32
nsInstall::GetQualifiedRegName(const nsString& name, nsString& qualifiedRegName )
{
    nsString startOfName;
    name.Left(startOfName, 7);

    if ( startOfName.EqualsLiteral("=COMM=/") || startOfName.EqualsLiteral("=USER=/"))
    {
        qualifiedRegName = startOfName;
    }
    else if (name.CharAt(0) != '/' &&
             !mRegistryPackageName.IsEmpty())
    {
        qualifiedRegName = mRegistryPackageName
                         + NS_LITERAL_STRING("/")
                         + name;
    }
    else
    {
        qualifiedRegName = name;
    }

    if (BadRegName(qualifiedRegName))
    {
        return BAD_PACKAGE_NAME;
    }

    return SUCCESS;
}


void
nsInstall::CurrentUserNode(nsString& userRegNode)
{
    nsXPIDLCString profname;
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);

    if ( prefBranch )
    {
        prefBranch->GetCharPref("profile.name", getter_Copies(profname));
    }

    userRegNode.AssignLiteral("/Netscape/Users/");
    if ( !profname.IsEmpty() )
    {
        userRegNode.AppendWithConversion(profname);
        userRegNode.AppendLiteral("/");
    }
}



PRBool
nsInstall::BadRegName(const nsString& regName)
{
    if ( regName.IsEmpty() )
        return PR_TRUE;

    if ((regName.First() == ' ' ) || (regName.Last() == ' ' ))
        return PR_TRUE;

    if ( regName.Find("//") != -1 )
        return PR_TRUE;

    if ( regName.Find(" /") != -1 )
        return PR_TRUE;

    if ( regName.Find("/ ") != -1  )
        return PR_TRUE;

    return PR_FALSE;
}

PRInt32
nsInstall::SaveError(PRInt32 errcode)
{
  if ( errcode != nsInstall::SUCCESS )
    mLastError = errcode;

  return errcode;
}







void
nsInstall::CleanUp(void)
{
    nsInstallObject* ie;

    if ( mInstalledFiles != nsnull )
    {
        for (PRInt32 i=0; i < mInstalledFiles->Count(); i++)
        {
            ie = (nsInstallObject*)mInstalledFiles->ElementAt(i);
            if (ie)
                delete ie;
        }

        mInstalledFiles->Clear();
        delete (mInstalledFiles);
        mInstalledFiles = nsnull;
    }

    if (mPatchList != nsnull)
    {
        mPatchList->Reset();
        delete mPatchList;
        mPatchList = nsnull;
    }

    if (mPackageFolder != nsnull)
    {
      delete (mPackageFolder);
      mPackageFolder = nsnull;
    }

    mRegistryPackageName.SetLength(0); 
    mStartInstallCompleted = PR_FALSE;
}


void
nsInstall::SetJarFileLocation(nsIFile* aFile)
{
    mJarFileLocation = aFile;
}

void
nsInstall::GetInstallArguments(nsString& args)
{
    args = mInstallArguments;
}

void
nsInstall::SetInstallArguments(const nsString& args)
{
    mInstallArguments = args;
}


void nsInstall::GetInstallURL(nsString& url)        { url = mInstallURL; }
void nsInstall::SetInstallURL(const nsString& url)  { mInstallURL = url; }








PRUnichar *GetTranslatedString(const PRUnichar* aString)
{
    nsCOMPtr<nsIStringBundleService> stringService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    nsCOMPtr<nsIStringBundle> stringBundle;
    PRUnichar* translatedString;

    nsresult rv = stringService->CreateBundle(kInstallLocaleProperties, getter_AddRefs(stringBundle));
    if (NS_FAILED(rv)) return nsnull;

    rv = stringBundle->GetStringFromName(aString, &translatedString);
    if (NS_FAILED(rv)) return nsnull;

    return translatedString;
}

PRInt32
nsInstall::Alert(nsString& string)
{
    nsPIXPIProxy *ui = GetUIThreadProxy();
    if (!ui)
        return UNEXPECTED_ERROR;

    nsAutoString title;
    title.AssignLiteral("Alert");
    if (!mUIName.IsEmpty())
    {
        title = mUIName;
    }
    else
    {
        PRUnichar *t = GetTranslatedString(title.get());
        if (t)
            title.Adopt(t);
    }
    return ui->Alert( title.get(), string.get());
}

PRInt32
nsInstall::ConfirmEx(nsString& aDialogTitle, nsString& aText, PRUint32 aButtonFlags, nsString& aButton0Title, nsString& aButton1Title, nsString& aButton2Title, nsString& aCheckMsg, PRBool* aCheckState, PRInt32* aReturn)
{
    *aReturn = -1; 

    nsPIXPIProxy *ui = GetUIThreadProxy();
    if (!ui)
        return UNEXPECTED_ERROR;

    nsAutoString title;
    title.AssignLiteral("Confirm");
    if (!aDialogTitle.IsEmpty())
    {
        title = aDialogTitle;
    }
    else if (!mUIName.IsEmpty())
    {
        title = mUIName;
    }
    else
    {
        PRUnichar *t = GetTranslatedString(title.get());
        if (t)
            title.Adopt(t);
    }
    return ui->ConfirmEx( title.get(),
                          aText.get(),
                          aButtonFlags,
                          aButton0Title.get(),
                          aButton1Title.get(),
                          aButton2Title.get(),
                          aCheckMsg.get(),
                          aCheckState,
                          aReturn);
}






PRInt32
nsInstall::ExtractFileFromJar(const nsString& aJarfile, nsIFile* aSuggestedName, nsIFile** aRealName)
{
    PRInt32 extpos = 0;
    nsCOMPtr<nsIFile> extractHereSpec;
    nsCOMPtr<nsILocalFile> tempFile;
    nsresult rv;

    if (aSuggestedName == nsnull)
    {
        nsCOMPtr<nsIProperties> directoryService =
                 do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);

        directoryService->Get(NS_OS_TEMP_DIR, NS_GET_IID(nsIFile), getter_AddRefs(tempFile));

        nsAutoString tempFileName(NS_LITERAL_STRING("xpinstall"));

        
        extpos = aJarfile.RFindChar('.');
        if (extpos != -1)
        {
            
            nsString extension;
            aJarfile.Right(extension, (aJarfile.Length() - extpos) );
            tempFileName += extension;
        }
        tempFile->Append(tempFileName);
        tempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0664);
        tempFile->Clone(getter_AddRefs(extractHereSpec));

        if (extractHereSpec == nsnull)
            return nsInstall::OUT_OF_MEMORY;
    }
    else
    {
        nsCOMPtr<nsIFile> temp;
        aSuggestedName->Clone(getter_AddRefs(temp));

        PRBool exists;
        temp->Exists(&exists);
        if (exists)
        {
            PRBool writable;
            temp->IsWritable(&writable);
            if (!writable) 
                return nsInstall::READ_ONLY;

            tempFile = do_QueryInterface(temp, &rv); 
            if (tempFile == nsnull)
                return nsInstall::OUT_OF_MEMORY;

            
            nsAutoString newLeafName;
            tempFile->GetLeafName(newLeafName);

            PRInt32 extpos = newLeafName.RFindChar('.');
            if (extpos != -1)
            {
                
                newLeafName.Truncate(extpos + 1); 
            }
            newLeafName.AppendLiteral("new");

            
            tempFile->SetLeafName(newLeafName);
            tempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0644);
            extractHereSpec = tempFile;
        }
        extractHereSpec = temp;
    }

    rv = mJarFileData->Extract(NS_LossyConvertUTF16toASCII(aJarfile).get(),
                               extractHereSpec);
    if (NS_FAILED(rv))
    {
        switch (rv) {
          case NS_ERROR_FILE_ACCESS_DENIED:         return ACCESS_DENIED;
          case NS_ERROR_FILE_DISK_FULL:             return INSUFFICIENT_DISK_SPACE;
          case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST: return DOES_NOT_EXIST;
          default:                                  return EXTRACTION_FAILED;
        }
    }

#ifdef XP_MACOSX
    FSRef finalRef, extractedRef;

    nsCOMPtr<nsILocalFileMac> tempExtractHereSpec;
    tempExtractHereSpec = do_QueryInterface(extractHereSpec, &rv);
    tempExtractHereSpec->GetFSRef(&extractedRef);

    if ( nsAppleSingleDecoder::IsAppleSingleFile(&extractedRef) )
    {
        nsAppleSingleDecoder *asd = 
          new nsAppleSingleDecoder(&extractedRef, &finalRef);
        OSErr decodeErr = fnfErr;

        if (asd)
            decodeErr = asd->Decode();

        if (decodeErr != noErr)
        {
            if (asd)
                delete asd;
            return EXTRACTION_FAILED;
        }

        if (noErr != FSCompareFSRefs(&extractedRef, &finalRef))
        {
            
            FSDeleteObject(&extractedRef);

            
            tempExtractHereSpec->InitWithFSRef(&finalRef);
            extractHereSpec = do_QueryInterface(tempExtractHereSpec, &rv);
        }
    }
#endif 

    extractHereSpec->Clone(aRealName);
    
    return nsInstall::SUCCESS;
}










char*
nsInstall::GetResourcedString(const nsAString& aResName)
{
    if (mStringBundle)
    {
        nsXPIDLString ucRscdStr;
        nsresult rv = mStringBundle->GetStringFromName(PromiseFlatString(aResName).get(),
                                                     getter_Copies(ucRscdStr));
        if (NS_SUCCEEDED(rv))
            return ToNewCString(ucRscdStr);
    }

    




    return nsCRT::strdup(nsInstallResources::GetDefaultVal(
                    NS_LossyConvertUTF16toASCII(aResName).get()));
}
