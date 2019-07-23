






























 
#include "winEmbedFileLocProvider.h"
#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsIProperties.h"
#include "nsServiceManagerUtils.h"

#include <windows.h>
#include <shlobj.h>

 


#define APP_REGISTRY_NAME nsEmbedCString("registry.dat")

#define PROFILE_ROOT_DIR_NAME       nsEmbedCString("Profiles")
#define DEFAULTS_DIR_NAME           nsEmbedCString("defaults")
#define DEFAULTS_PREF_DIR_NAME      nsEmbedCString("pref")
#define DEFAULTS_PROFILE_DIR_NAME   nsEmbedCString("profile")
#define RES_DIR_NAME                nsEmbedCString("res")
#define CHROME_DIR_NAME             nsEmbedCString("chrome")
#define PLUGINS_DIR_NAME            nsEmbedCString("plugins")
#define SEARCH_DIR_NAME             nsEmbedCString("searchplugins")
#define COMPONENTS_DIR_NAME         nsEmbedCString("components")





winEmbedFileLocProvider::winEmbedFileLocProvider(const nsACString& aAppDataDirName)
{
    mProductDirName = aAppDataDirName;
}

winEmbedFileLocProvider::~winEmbedFileLocProvider()
{
}






NS_IMPL_ISUPPORTS1(winEmbedFileLocProvider, nsIDirectoryServiceProvider)





NS_IMETHODIMP
winEmbedFileLocProvider::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{    
    nsCOMPtr<nsILocalFile>  localFile;
    nsresult rv = NS_ERROR_FAILURE;

    *_retval = nsnull;
    *persistent = PR_TRUE;
    
    if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
    {
        rv = GetProductDirectory(getter_AddRefs(localFile));
    }
    else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
    {
        rv = GetProductDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendNative(APP_REGISTRY_NAME);
    }
    else if (strcmp(prop, NS_APP_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_PREF_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PREF_DIR_NAME);
        }
    }
    else if (strcmp(prop, NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR) == 0 ||
             strcmp(prop, NS_APP_PROFILE_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PROFILE_DIR_NAME);
        }
    }
    else if (strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
    {
        rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile));   
    }
    else if (strcmp(prop, NS_APP_RES_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(RES_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_CHROME_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(CHROME_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_PLUGINS_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(PLUGINS_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_SEARCH_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(SEARCH_DIR_NAME);
    }

#ifdef XPCOM_GLUE
    
    
    
    
    
    
    
    
    
    else if (strcmp(prop, NS_GRE_DIR) == 0)
    {
        rv = GRE_GetGREDirectory(getter_AddRefs(localFile));
    }
#endif

    if (localFile && NS_SUCCEEDED(rv))
        return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);
        
    return rv;
}

NS_METHOD winEmbedFileLocProvider::CloneMozBinDirectory(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    nsresult rv;
    
    if (!mMozBinDirectory)
    {        
        
        
        
        
        
        nsCOMPtr<nsIProperties> directoryService = 
                 do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return rv;
        
        rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), getter_AddRefs(mMozBinDirectory));
        if (NS_FAILED(rv)) {
            rv = directoryService->Get(NS_OS_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), getter_AddRefs(mMozBinDirectory));
            if (NS_FAILED(rv))
                return rv;
        }
    }
    
    nsCOMPtr<nsIFile> aFile;
    rv = mMozBinDirectory->Clone(getter_AddRefs(aFile));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsILocalFile> lfile = do_QueryInterface (aFile);
    if (!lfile)
        return NS_ERROR_FAILURE;
    
    NS_IF_ADDREF(*aLocalFile = lfile);
    return NS_OK;
}







NS_METHOD winEmbedFileLocProvider::GetProductDirectory(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    
    nsresult rv;
    PRBool exists;
    nsCOMPtr<nsILocalFile> localDir;
   
    nsCOMPtr<nsIProperties> directoryService = 
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = directoryService->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
    if (NS_SUCCEEDED(rv))
        rv = localDir->Exists(&exists);
    if (NS_FAILED(rv) || !exists)
    {
        
        localDir = nsnull;
        rv = directoryService->Get(NS_WIN_WINDOWS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
    }
    if (NS_FAILED(rv)) return rv;

    rv = localDir->AppendNative(mProductDirName);
    if (NS_FAILED(rv)) return rv;
    rv = localDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return rv;

    *aLocalFile = localDir;
    NS_ADDREF(*aLocalFile);

   return rv; 
}







NS_METHOD winEmbedFileLocProvider::GetDefaultUserProfileRoot(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    
    nsresult rv;
    PRBool exists;
    nsCOMPtr<nsILocalFile> localDir;
   
    rv = GetProductDirectory(getter_AddRefs(localDir));
    if (NS_FAILED(rv)) return rv;

    
    rv = localDir->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
    if (NS_FAILED(rv)) return rv;
    rv = localDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return rv;

    *aLocalFile = localDir;
    NS_ADDREF(*aLocalFile);

   return rv; 
}

