







































#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsLocalFile.h"
#include "nsDebug.h"
#include "nsStaticAtom.h"
#include "nsEnumeratorUtils.h"

#include "nsICategoryManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

#if defined(XP_WIN)
#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#elif defined(XP_UNIX)
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include "prenv.h"
#ifdef XP_MACOSX
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#endif
#elif defined(XP_OS2)
#define MAX_PATH _MAX_PATH
#elif defined(XP_BEOS)
#include <FindDirectory.h>
#include <Path.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <OS.h>
#include <image.h>
#include "prenv.h"
#endif

#include "SpecialSystemDirectory.h"
#include "nsAppFileLocationProvider.h"

#define COMPONENT_REGISTRY_NAME NS_LITERAL_CSTRING("compreg.dat")
#define COMPONENT_DIRECTORY     NS_LITERAL_CSTRING("components")



#if defined (XP_WIN)
#define HOME_DIR NS_WIN_APPDATA_DIR
#elif defined (XP_MACOSX)
#define HOME_DIR NS_OSX_HOME_DIR
#elif defined (XP_UNIX)
#define HOME_DIR NS_UNIX_HOME_DIR
#elif defined (XP_OS2)
#define HOME_DIR NS_OS2_HOME_DIR
#elif defined (XP_BEOS)
#define HOME_DIR NS_BEOS_HOME_DIR
#endif


nsresult 
nsDirectoryService::GetCurrentProcessDirectory(nsILocalFile** aFile)

{
    NS_ENSURE_ARG_POINTER(aFile);
    *aFile = nsnull;
    
   
    if (!gService)
        return NS_ERROR_FAILURE;

    nsresult rv; 
 
    nsCOMPtr<nsIProperties> dirService;
    rv = nsDirectoryService::Create(nsnull, 
                                    NS_GET_IID(nsIProperties), 
                                    getter_AddRefs(dirService));  

    if (dirService)
    {
      nsCOMPtr <nsILocalFile> aLocalFile;
      dirService->Get(NS_XPCOM_INIT_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(aLocalFile));
      if (aLocalFile)
      {
        *aFile = aLocalFile;
        NS_ADDREF(*aFile);
        return NS_OK;
      }
    }

    nsLocalFile* localFile = new nsLocalFile;

    if (localFile == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(localFile);



#ifdef XP_WIN
    PRUnichar buf[MAX_PATH];
    if ( ::GetModuleFileNameW(0, buf, sizeof(buf)) )
    {
        
        PRUnichar* lastSlash = wcsrchr(buf, L'\\');
        if (lastSlash)
            *(lastSlash + 1) = L'\0';

        localFile->InitWithPath(nsDependentString(buf));
        *aFile = localFile;
        return NS_OK;
    }

#elif defined(XP_MACOSX)
    
    CFBundleRef appBundle = CFBundleGetMainBundle();
    if (appBundle != nsnull)
    {
        CFURLRef bundleURL = CFBundleCopyExecutableURL(appBundle);
        if (bundleURL != nsnull)
        {
            CFURLRef parentURL = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, bundleURL);
            if (parentURL)
            {
                
                
                
                char buffer[PATH_MAX];
                if (CFURLGetFileSystemRepresentation(parentURL, PR_TRUE, (UInt8 *)buffer, sizeof(buffer)))
                {
#ifdef DEBUG_conrad
                    printf("nsDirectoryService - CurrentProcessDir is: %s\n", buffer);
#endif
                    rv = localFile->InitWithNativePath(nsDependentCString(buffer));
                    if (NS_SUCCEEDED(rv))
                        *aFile = localFile;
                }
                CFRelease(parentURL);
            }
            CFRelease(bundleURL);
        }
    }
    
    NS_ASSERTION(*aFile, "nsDirectoryService - Could not determine CurrentProcessDir.\n");
    if (*aFile)
        return NS_OK;

#elif defined(XP_UNIX)

    
    
    
    
    char buf[MAXPATHLEN];

    
    
    
    
    
    
    
    
    
#ifdef MOZ_DEFAULT_MOZILLA_FIVE_HOME
    const char *home = PR_GetEnv("MOZILLA_FIVE_HOME");
    if (!home || !*home)
    {
        putenv("MOZILLA_FIVE_HOME=" MOZ_DEFAULT_MOZILLA_FIVE_HOME);
    }
#endif

    char *moz5 = PR_GetEnv("MOZILLA_FIVE_HOME");
    if (moz5 && *moz5)
    {
        if (realpath(moz5, buf)) {
            localFile->InitWithNativePath(nsDependentCString(buf));
            *aFile = localFile;
            return NS_OK;
        }
    }
#if defined(DEBUG)
    static PRBool firstWarning = PR_TRUE;

    if((!moz5 || !*moz5) && firstWarning) {
        
        printf("Warning: MOZILLA_FIVE_HOME not set.\n");
        firstWarning = PR_FALSE;
    }
#endif 

    
    if (getcwd(buf, sizeof(buf)))
    {
        localFile->InitWithNativePath(nsDependentCString(buf));
        *aFile = localFile;
        return NS_OK;
    }

#elif defined(XP_OS2)
    PPIB ppib;
    PTIB ptib;
    char buffer[CCHMAXPATH];
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName( ppib->pib_hmte, CCHMAXPATH, buffer);
    *strrchr( buffer, '\\') = '\0'; 
    localFile->InitWithNativePath(nsDependentCString(buffer));
    *aFile = localFile;
    return NS_OK;

#elif defined(XP_BEOS)
    char buf[MAXPATHLEN];
    int32 cookie = 0;
    image_info info;
    char *p;
    *buf = 0;
    if(get_next_image_info(0, &cookie, &info) == B_OK)
    {
        strcpy(buf, info.name);
        if((p = strrchr(buf, '/')) != 0)
        {
            if( (p-2 >= buf) && *(p-2)=='/' && *(p-1)=='.')
                p -=2; 
            *p = 0;
            localFile->InitWithNativePath(nsDependentCString(buf));
            *aFile = localFile;
            return NS_OK;
        }
    }
#endif
    
    NS_RELEASE(localFile);

    NS_ERROR("unable to get current process directory");
    return NS_ERROR_FAILURE;
} 

nsDirectoryService* nsDirectoryService::gService = nsnull;

nsDirectoryService::nsDirectoryService() :
    mHashtable(256, PR_TRUE)
{
}

NS_METHOD
nsDirectoryService::Create(nsISupports *outer, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_NO_AGGREGATION(outer);

    if (!gService)
    {
        return NS_ERROR_NOT_INITIALIZED;
    }

    return gService->QueryInterface(aIID, aResult);
}

#define DIR_ATOM(name_, value_) nsIAtom* nsDirectoryService::name_ = nsnull;
#include "nsDirectoryServiceAtomList.h"
#undef DIR_ATOM

#define DIR_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsDirectoryServiceAtomList.h"
#undef DIR_ATOM

static const nsStaticAtom directory_atoms[] = {
#define DIR_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &nsDirectoryService::name_),
#include "nsDirectoryServiceAtomList.h"
#undef DIR_ATOM
};    

NS_IMETHODIMP
nsDirectoryService::Init()
{
    NS_NOTREACHED("nsDirectoryService::Init() for internal use only!");
    return NS_OK;
}

nsresult
nsDirectoryService::RealInit()
{
    NS_ASSERTION(!gService, 
                 "nsDirectoryService::RealInit Mustn't initialize twice!");

    nsresult rv;

    nsRefPtr<nsDirectoryService> self = new nsDirectoryService();
    if (!self)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = NS_NewISupportsArray(getter_AddRefs(((nsDirectoryService*) self)->mProviders));
    if (NS_FAILED(rv))
        return rv;

    NS_RegisterStaticAtoms(directory_atoms, NS_ARRAY_LENGTH(directory_atoms));
    
    
    nsAppFileLocationProvider *defaultProvider = new nsAppFileLocationProvider;
    if (!defaultProvider)
        return NS_ERROR_OUT_OF_MEMORY;
    
    rv = ((nsDirectoryService*) self)->mProviders->AppendElement(defaultProvider) ? NS_OK : NS_ERROR_FAILURE;
    if (NS_FAILED(rv))
        return rv;

    self.swap(gService);
    return NS_OK;
}

PRBool
nsDirectoryService::ReleaseValues(nsHashKey* key, void* data, void* closure)
{
    nsISupports* value = (nsISupports*)data;
    NS_IF_RELEASE(value);
    return PR_TRUE;
}

nsDirectoryService::~nsDirectoryService()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsDirectoryService, nsIProperties, nsIDirectoryService, nsIDirectoryServiceProvider, nsIDirectoryServiceProvider2)


NS_IMETHODIMP
nsDirectoryService::Undefine(const char* prop)
{
    NS_ENSURE_ARG(prop);

    nsCStringKey key(prop);
    if (!mHashtable.Exists(&key))
        return NS_ERROR_FAILURE;

    mHashtable.Remove (&key);
    return NS_OK;
 }

NS_IMETHODIMP
nsDirectoryService::GetKeys(PRUint32 *count, char ***keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

struct FileData
{
  FileData(const char* aProperty,
           const nsIID& aUUID) :
    property(aProperty),
    data(nsnull),
    persistent(PR_TRUE),
    uuid(aUUID) {}
    
  const char*   property;
  nsISupports*  data;
  PRBool        persistent;
  const nsIID&  uuid;
};

static PRBool FindProviderFile(nsISupports* aElement, void *aData)
{
  nsresult rv;
  FileData* fileData = (FileData*)aData;
  if (fileData->uuid.Equals(NS_GET_IID(nsISimpleEnumerator)))
  {
      
      nsCOMPtr<nsIDirectoryServiceProvider2> prov2 = do_QueryInterface(aElement);
      if (prov2)
      {
          nsCOMPtr<nsISimpleEnumerator> newFiles;
          rv = prov2->GetFiles(fileData->property, getter_AddRefs(newFiles));
          if (NS_SUCCEEDED(rv) && newFiles) {
              if (fileData->data) {
                  nsCOMPtr<nsISimpleEnumerator> unionFiles;

                  NS_NewUnionEnumerator(getter_AddRefs(unionFiles),
                                        (nsISimpleEnumerator*) fileData->data, newFiles);

                  if (unionFiles)
                      unionFiles.swap(* (nsISimpleEnumerator**) &fileData->data);
              }
              else
              {
                  NS_ADDREF(fileData->data = newFiles);
              }
                  
              fileData->persistent = PR_FALSE; 
              return rv == NS_SUCCESS_AGGREGATE_RESULT;
          }
      }
  }
  else
  {
      nsCOMPtr<nsIDirectoryServiceProvider> prov = do_QueryInterface(aElement);
      if (prov)
      {
          rv = prov->GetFile(fileData->property, &fileData->persistent, (nsIFile **)&fileData->data);
          if (NS_SUCCEEDED(rv) && fileData->data)
              return PR_FALSE;
      }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsDirectoryService::Get(const char* prop, const nsIID & uuid, void* *result)
{
    NS_ENSURE_ARG(prop);

    nsCStringKey key(prop);
    
    nsCOMPtr<nsISupports> value = dont_AddRef(mHashtable.Get(&key));
    
    if (value)
    {
        nsCOMPtr<nsIFile> cloneFile;
        nsCOMPtr<nsIFile> cachedFile = do_QueryInterface(value);
        NS_ASSERTION(cachedFile, 
                     "nsDirectoryService::Get nsIFile expected");

        cachedFile->Clone(getter_AddRefs(cloneFile));
        return cloneFile->QueryInterface(uuid, result);
    }

    
    FileData fileData(prop, uuid);

    mProviders->EnumerateBackwards(FindProviderFile, &fileData);
    if (fileData.data)
    {
        if (fileData.persistent)
        {
            Set(prop, static_cast<nsIFile*>(fileData.data));
        }
        nsresult rv = (fileData.data)->QueryInterface(uuid, result);
        NS_RELEASE(fileData.data);  
        return rv;
    }

    FindProviderFile(static_cast<nsIDirectoryServiceProvider*>(this), &fileData);
    if (fileData.data)
    {
        if (fileData.persistent)
        {
            Set(prop, static_cast<nsIFile*>(fileData.data));
        }
        nsresult rv = (fileData.data)->QueryInterface(uuid, result);
        NS_RELEASE(fileData.data);  
        return rv;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDirectoryService::Set(const char* prop, nsISupports* value)
{
    NS_ENSURE_ARG(prop);

    nsCStringKey key(prop);
    if (mHashtable.Exists(&key) || value == nsnull)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIFile> ourFile;
    value->QueryInterface(NS_GET_IID(nsIFile), getter_AddRefs(ourFile));
    if (ourFile)
    {
      nsCOMPtr<nsIFile> cloneFile;
      ourFile->Clone (getter_AddRefs (cloneFile));
      mHashtable.Put(&key, cloneFile);

      return NS_OK;
    }

    return NS_ERROR_FAILURE;   
}

NS_IMETHODIMP
nsDirectoryService::Has(const char *prop, PRBool *_retval)
{
    NS_ENSURE_ARG(prop);

    *_retval = PR_FALSE;
    nsCOMPtr<nsIFile> value;
    nsresult rv = Get(prop, NS_GET_IID(nsIFile), getter_AddRefs(value));
    if (NS_FAILED(rv))
        return NS_OK;
    
    if (value)
    {
        *_retval = PR_TRUE;
    }
    
    return rv;
}

NS_IMETHODIMP
nsDirectoryService::RegisterProvider(nsIDirectoryServiceProvider *prov)
{
    nsresult rv;
    if (!prov)
        return NS_ERROR_FAILURE;
    if (!mProviders)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsISupports> supports = do_QueryInterface(prov, &rv);
    if (NS_FAILED(rv)) return rv;

    
    return mProviders->AppendElement(supports) ? NS_OK : NS_ERROR_FAILURE;
}

void
nsDirectoryService::RegisterCategoryProviders()
{
    nsCOMPtr<nsICategoryManager> catman
        (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
    if (!catman)
        return;

    nsCOMPtr<nsISimpleEnumerator> entries;
    catman->EnumerateCategory(XPCOM_DIRECTORY_PROVIDER_CATEGORY,
                              getter_AddRefs(entries));

    nsCOMPtr<nsIUTF8StringEnumerator> strings(do_QueryInterface(entries));
    if (!strings)
        return;

    PRBool more;
    while (NS_SUCCEEDED(strings->HasMore(&more)) && more) {
        nsCAutoString entry;
        strings->GetNext(entry);

        nsXPIDLCString contractID;
        catman->GetCategoryEntry(XPCOM_DIRECTORY_PROVIDER_CATEGORY, entry.get(), getter_Copies(contractID));

        if (contractID) {
            nsCOMPtr<nsIDirectoryServiceProvider> provider = do_GetService(contractID.get());
            if (provider)
                RegisterProvider(provider);
        }
    }
}

NS_IMETHODIMP
nsDirectoryService::UnregisterProvider(nsIDirectoryServiceProvider *prov)
{
    nsresult rv;
    if (!prov)
        return NS_ERROR_FAILURE;
    if (!mProviders)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsISupports> supports = do_QueryInterface(prov, &rv);
    if (NS_FAILED(rv)) return rv;

    
    return mProviders->RemoveElement(supports) ? NS_OK : NS_ERROR_FAILURE;
}






NS_IMETHODIMP
nsDirectoryService::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{
    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = NS_ERROR_FAILURE;

    *_retval = nsnull;
    *persistent = PR_TRUE;

    nsCOMPtr<nsIAtom> inAtom = do_GetAtom(prop);

    
        
    if (inAtom == nsDirectoryService::sCurrentProcess || 
        inAtom == nsDirectoryService::sOS_CurrentProcessDirectory )
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sComponentRegistry)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
        if (!localFile)
            return NS_ERROR_FAILURE;

        localFile->AppendNative(COMPONENT_DIRECTORY);           
        localFile->AppendNative(COMPONENT_REGISTRY_NAME);           
    }
    
    
    
    else if (inAtom == nsDirectoryService::sGRE_Directory)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
    }
    
    
    
    else if (inAtom == nsDirectoryService::sGRE_ComponentDirectory)
    {
        rv = Get(NS_GRE_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localFile));
        if (localFile) {
            nsCOMPtr<nsIFile> cdir;
            localFile->Clone(getter_AddRefs(cdir));
            cdir->AppendNative(COMPONENT_DIRECTORY);
            localFile = do_QueryInterface(cdir);
        }
    }
    else if (inAtom == nsDirectoryService::sComponentDirectory)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
        if (localFile) {
            nsCOMPtr<nsIFile> cdir;
            localFile->Clone(getter_AddRefs(cdir));
            cdir->AppendNative(COMPONENT_DIRECTORY);
            localFile = do_QueryInterface(cdir);
        }
    }
    else if (inAtom == nsDirectoryService::sOS_DriveDirectory)
    {
        rv = GetSpecialSystemDirectory(OS_DriveDirectory, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_TemporaryDirectory)
    {
        rv = GetSpecialSystemDirectory(OS_TemporaryDirectory, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_CurrentProcessDirectory)
    {
        rv = GetSpecialSystemDirectory(OS_CurrentProcessDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sOS_CurrentWorkingDirectory)
    {
        rv = GetSpecialSystemDirectory(OS_CurrentWorkingDirectory, getter_AddRefs(localFile)); 
    }
       
#if defined(XP_MACOSX)
    else if (inAtom == nsDirectoryService::sDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kSystemFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sTrashDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kTrashFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sStartupDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kStartupFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sShutdownDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kShutdownFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sAppleMenuDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kAppleMenuFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sControlPanelDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kControlPanelFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sExtensionDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kExtensionFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sFontsDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kFontsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sPreferencesDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kPreferencesFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sDocumentsDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sInternetSearchDirectory)
    {
        rv = GetOSXFolderType(kClassicDomain, kInternetSearchSitesFolderType, getter_AddRefs(localFile));
    }   
    else if (inAtom == nsDirectoryService::sUserLibDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kDomainLibraryFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_HomeDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kDomainTopLevelFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sDefaultDownloadDirectory)
    {
        
        
        
        
#ifndef kDownloadsFolderType
#define kDownloadsFolderType 'down'
#endif

        rv = GetOSXFolderType(kUserDomain, kDownloadsFolderType,
                              getter_AddRefs(localFile));
        if (NS_FAILED(rv)) {
            rv = GetOSXFolderType(kUserDomain, kDesktopFolderType,
                                  getter_AddRefs(localFile));
        }
    }
    else if (inAtom == nsDirectoryService::sUserDesktopDirectory ||
             inAtom == nsDirectoryService::sOS_DesktopDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kDesktopFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalDesktopDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kDesktopFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sUserApplicationsDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kApplicationsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalApplicationsDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kApplicationsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sUserDocumentsDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalDocumentsDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sUserInternetPlugInDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kInternetPlugInFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalInternetPlugInDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kInternetPlugInFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sUserFrameworksDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kFrameworksFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalFrameworksDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kFrameworksFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sUserPreferencesDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kPreferencesFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sLocalPreferencesDirectory)
    {
        rv = GetOSXFolderType(kLocalDomain, kPreferencesFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sPictureDocumentsDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kPictureDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sMovieDocumentsDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kMovieDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sMusicDocumentsDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kMusicDocumentsFolderType, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sInternetSitesDirectory)
    {
        rv = GetOSXFolderType(kUserDomain, kInternetSitesFolderType, getter_AddRefs(localFile));
    }
#elif defined (XP_WIN)
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_SystemDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sWindowsDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_WindowsDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sWindowsProgramFiles)
    {
        rv = GetSpecialSystemDirectory(Win_ProgramFiles, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_HomeDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_HomeDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sDesktop)
    {
        rv = GetSpecialSystemDirectory(Win_Desktop, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sPrograms)
    {
        rv = GetSpecialSystemDirectory(Win_Programs, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sControls)
    {
        rv = GetSpecialSystemDirectory(Win_Controls, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sPrinters)
    {
        rv = GetSpecialSystemDirectory(Win_Printers, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sPersonal)
    {
        rv = GetSpecialSystemDirectory(Win_Personal, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sFavorites)
    {
        rv = GetSpecialSystemDirectory(Win_Favorites, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sStartup)
    {
        rv = GetSpecialSystemDirectory(Win_Startup, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sRecent)
    {
        rv = GetSpecialSystemDirectory(Win_Recent, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sSendto)
    {
        rv = GetSpecialSystemDirectory(Win_Sendto, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sBitbucket)
    {
        rv = GetSpecialSystemDirectory(Win_Bitbucket, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sStartmenu)
    {
        rv = GetSpecialSystemDirectory(Win_Startmenu, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sDesktopdirectory ||
             inAtom == nsDirectoryService::sOS_DesktopDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_Desktopdirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sDrives)
    {
        rv = GetSpecialSystemDirectory(Win_Drives, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sNetwork)
    {
        rv = GetSpecialSystemDirectory(Win_Network, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sNethood)
    {
        rv = GetSpecialSystemDirectory(Win_Nethood, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sFonts)
    {
        rv = GetSpecialSystemDirectory(Win_Fonts, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sTemplates)
    {
        rv = GetSpecialSystemDirectory(Win_Templates, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sCommon_Startmenu)
    {
        rv = GetSpecialSystemDirectory(Win_Common_Startmenu, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sCommon_Programs)
    {
        rv = GetSpecialSystemDirectory(Win_Common_Programs, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sCommon_Startup)
    {
        rv = GetSpecialSystemDirectory(Win_Common_Startup, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sCommon_Desktopdirectory)
    {
        rv = GetSpecialSystemDirectory(Win_Common_Desktopdirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sAppdata)
    {
        rv = GetSpecialSystemDirectory(Win_Appdata, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sLocalAppdata)
    {
        rv = GetSpecialSystemDirectory(Win_LocalAppdata, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sPrinthood)
    {
        rv = GetSpecialSystemDirectory(Win_Printhood, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sWinCookiesDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_Cookies, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sDefaultDownloadDirectory)
    {
        rv = GetSpecialSystemDirectory(Win_Downloads, getter_AddRefs(localFile));
    }
#elif defined (XP_UNIX)

    else if (inAtom == nsDirectoryService::sLocalDirectory)
    {
        rv = GetSpecialSystemDirectory(Unix_LocalDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sLibDirectory)
    {
        rv = GetSpecialSystemDirectory(Unix_LibDirectory, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_HomeDirectory)
    {
        rv = GetSpecialSystemDirectory(Unix_HomeDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sXDGDesktop ||
             inAtom == nsDirectoryService::sOS_DesktopDirectory)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Desktop, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGDocuments)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Documents, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGDownload ||
             inAtom == nsDirectoryService::sDefaultDownloadDirectory)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Download, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGMusic)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Music, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGPictures)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Pictures, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGPublicShare)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_PublicShare, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGTemplates)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Templates, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
    else if (inAtom == nsDirectoryService::sXDGVideos)
    {
        rv = GetSpecialSystemDirectory(Unix_XDG_Videos, getter_AddRefs(localFile));
        *persistent = PR_FALSE;
    }
#elif defined (XP_OS2)
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        rv = GetSpecialSystemDirectory(OS2_SystemDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sOS2Directory)
    {
        rv = GetSpecialSystemDirectory(OS2_OS2Directory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sOS_HomeDirectory)
    {
        rv = GetSpecialSystemDirectory(OS2_HomeDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sOS_DesktopDirectory)
    {
        rv = GetSpecialSystemDirectory(OS2_DesktopDirectory, getter_AddRefs(localFile)); 
    }
#elif defined (XP_BEOS)
    else if (inAtom == nsDirectoryService::sSettingsDirectory)
    {
        rv = GetSpecialSystemDirectory(BeOS_SettingsDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sOS_HomeDirectory)
    {
        rv = GetSpecialSystemDirectory(BeOS_HomeDirectory, getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sOS_DesktopDirectory)
    {
        rv = GetSpecialSystemDirectory(BeOS_DesktopDirectory, getter_AddRefs(localFile)); 
    }
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        rv = GetSpecialSystemDirectory(BeOS_SystemDirectory, getter_AddRefs(localFile)); 
    }
#endif

    if (NS_FAILED(rv))
        return rv;

    if (!localFile)
        return NS_ERROR_FAILURE;

    return CallQueryInterface(localFile, _retval);
}

NS_IMETHODIMP
nsDirectoryService::GetFiles(const char *prop, nsISimpleEnumerator **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = nsnull;
        
    return NS_ERROR_FAILURE;
}
