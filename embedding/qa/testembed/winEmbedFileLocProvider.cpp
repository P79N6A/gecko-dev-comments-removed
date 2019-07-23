





































 
#include "winEmbedFileLocProvider.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"


#include <windows.h>
#include <shlobj.h>

 


#define APP_REGISTRY_NAME NS_LITERAL_CSTRING("registry.dat")

#define PROFILE_ROOT_DIR_NAME       NS_LITERAL_CSTRING("Profiles")
#define DEFAULTS_DIR_NAME           NS_LITERAL_CSTRING("defaults")
#define DEFAULTS_PREF_DIR_NAME      NS_LITERAL_CSTRING("pref")
#define DEFAULTS_PROFILE_DIR_NAME   NS_LITERAL_CSTRING("profile")
#define RES_DIR_NAME                NS_LITERAL_CSTRING("res")
#define CHROME_DIR_NAME             NS_LITERAL_CSTRING("chrome")
#define PLUGINS_DIR_NAME            NS_LITERAL_CSTRING("plugins")
#define SEARCH_DIR_NAME             NS_LITERAL_CSTRING("searchplugins")
#define COMPONENTS_DIR_NAME         NS_LITERAL_CSTRING("components")





winEmbedFileLocProvider::winEmbedFileLocProvider(const char* productDirName)
{
    strncpy(mProductDirName, productDirName, sizeof(mProductDirName) - 1);
    mProductDirName[sizeof(mProductDirName) - 1] = '\0';
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
	
    if (nsCRT::strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
    {
        rv = GetProductDirectory(getter_AddRefs(localFile));
    }
    else if (nsCRT::strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
    {
        rv = GetProductDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendNative(APP_REGISTRY_NAME);
    }
    else if (nsCRT::strcmp(prop, NS_APP_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
    }
    else if (nsCRT::strcmp(prop, NS_APP_PREF_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PREF_DIR_NAME);
        }
    }
    else if (nsCRT::strcmp(prop, NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR) == 0 ||
             nsCRT::strcmp(prop, NS_APP_PROFILE_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PROFILE_DIR_NAME);
        }
    }
    else if (nsCRT::strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
    {
        rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile));   
    }
    else if (nsCRT::strcmp(prop, NS_APP_RES_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(RES_DIR_NAME);
    }
    else if (nsCRT::strcmp(prop, NS_APP_CHROME_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(CHROME_DIR_NAME);
    }
    else if (nsCRT::strcmp(prop, NS_APP_PLUGINS_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(PLUGINS_DIR_NAME);
    }
    else if (nsCRT::strcmp(prop, NS_APP_SEARCH_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(SEARCH_DIR_NAME);
    }
    
    
    
    
    
    
    
    
    
    else if (nsCRT::strcmp(prop, NS_GRE_DIR) == 0)
    {
        rv = GetGreDirectory(getter_AddRefs(localFile));
    }    
    else if (nsCRT::strcmp(prop, NS_GRE_COMPONENT_DIR) == 0)
    {
        rv = GetGreDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(COMPONENTS_DIR_NAME);
    }    
   
	if (localFile && NS_SUCCEEDED(rv))
		return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);
		
	return rv;
}




static char * GetGreLocationFromRegistry()
{
    char szKey[256];
    HKEY hRegKey = NULL;
    DWORD dwLength = _MAX_PATH * sizeof(char);
    long rc;
    char keyValue[_MAX_PATH + 1];
    char *pGreLocation = NULL;

    
    
    
    
    
    
    
    
    
    
    
    strcpy(szKey, "Software\\Mozilla\\GRE\\" MOZILLA_VERSION);

    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
        if ((rc = ::RegQueryValueEx(hRegKey, "GreHome", NULL, NULL, (BYTE *)keyValue, &dwLength))==ERROR_SUCCESS)
        {
            pGreLocation = ::strdup(keyValue);
            ::RegCloseKey(hRegKey);
        }
    }

    return pGreLocation;
}


















NS_METHOD winEmbedFileLocProvider::GetGreDirectory(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    nsresult rv = NS_ERROR_FAILURE;

    
    
    
    char *pGreDir = GetGreLocationFromRegistry();
    if(pGreDir)
    {
        nsCOMPtr<nsILocalFile> tempLocal;
	    rv = NS_NewNativeLocalFile(nsDependentCString(pGreDir), TRUE, getter_AddRefs(tempLocal));

        if (tempLocal)
        {
           *aLocalFile = tempLocal;
           NS_ADDREF(*aLocalFile);
           rv = NS_OK;
        }

        ::free(pGreDir);
    }

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

    rv = localDir->AppendRelativeNativePath(nsDependentCString(mProductDirName));
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

