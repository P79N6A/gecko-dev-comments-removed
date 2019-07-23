





































 
#include "CAppFileLocationProvider.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIAtom.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"

#include <Folders.h>
#include <Script.h>
#include <Processes.h>
#include "nsILocalFileMac.h"

#include "ApplIDs.h"  





CAppFileLocationProvider::CAppFileLocationProvider(const nsAString& aAppDataDirName) :
    mAppDataDirName(aAppDataDirName)
{
}

CAppFileLocationProvider::~CAppFileLocationProvider()
{
}






NS_IMPL_ISUPPORTS1(CAppFileLocationProvider, nsIDirectoryServiceProvider)





NS_IMETHODIMP
CAppFileLocationProvider::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{    
    nsCOMPtr<nsILocalFile>  localFile;
    nsresult rv = NS_ERROR_FAILURE;

    *_retval = nsnull;
    *persistent = PR_TRUE;
	
    if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
    {
        rv = GetAppDataDirectory(getter_AddRefs(localFile));
    }
    else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
    {
        rv = GetAppDataDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendNative(nsDependentCString("Application Registry"));
    }
    else if (strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
    {
        rv = GetAppDataDirectory(getter_AddRefs(localFile));
        if (NS_FAILED(rv))
          return rv;
        rv = localFile->AppendNative(nsDependentCString("Profiles"));
        if (NS_FAILED(rv))
          return rv;
          
        PRBool exists;
        rv = localFile->Exists(&exists);
        if (NS_SUCCEEDED(rv) && !exists)
          rv = localFile->Create(nsIFile::DIRECTORY_TYPE, 0775);
        if (NS_FAILED(rv))
          return rv;
    }
    
    if (localFile && NS_SUCCEEDED(rv))
      rv = localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);
		
    return rv;
}




NS_METHOD CAppFileLocationProvider::GetAppDataDirectory(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    NS_ENSURE_TRUE(!mAppDataDirName.IsEmpty(), NS_ERROR_NOT_INITIALIZED);
    
    nsresult rv;
    PRBool exists;
    nsCOMPtr<nsILocalFile> localDir;

    nsCOMPtr<nsIProperties> directoryService = 
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    OSErr   err;
 
#if defined(XP_MACOSX)
    FSRef fsRef;
    err = ::FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &fsRef);
    if (err) return NS_ERROR_FAILURE;
    NS_NewLocalFile(EmptyString(), PR_TRUE, getter_AddRefs(localDir));
    if (!localDir) return NS_ERROR_FAILURE;
    nsCOMPtr<nsILocalFileMac> localDirMac(do_QueryInterface(localDir));
    rv = localDirMac->InitWithFSRef(&fsRef);
    if (NS_FAILED(rv)) return rv;
#else   
    long    response;
    err = ::Gestalt(gestaltSystemVersion, &response);
    const char *prop = (!err && response >= 0x00001000) ? NS_MAC_USER_LIB_DIR : NS_MAC_DOCUMENTS_DIR;
    rv = directoryService->Get(prop, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
    if (NS_FAILED(rv)) return rv;   
#endif

    rv = localDir->Append(mAppDataDirName);
    if (NS_FAILED(rv))
        return rv;
    rv = localDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv))
        return rv;

    *aLocalFile = localDir;
    NS_ADDREF(*aLocalFile);

    return rv; 
}
