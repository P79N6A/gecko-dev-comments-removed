








































#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "nsCRT.h"
#include "plstr.h"
#include "prinit.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "mozStorage.h"

#include "sqlite3.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(mozStorageService, mozIStorageService)






static PRLock *gSingletonLock;







static
PRStatus
SingletonInit()
{
    gSingletonLock = PR_NewLock();
    NS_ENSURE_TRUE(gSingletonLock, PR_FAILURE);
    return PR_SUCCESS;
}

mozStorageService *mozStorageService::gStorageService = nsnull;

mozStorageService *
mozStorageService::GetSingleton()
{
    
    
    static PRCallOnceType sInitOnce;
    PRStatus rc = PR_CallOnce(&sInitOnce, SingletonInit);
    if (rc != PR_SUCCESS)
        return nsnull;

    
    if (!gSingletonLock)
        return nsnull;

    nsAutoLock lock(gSingletonLock);
    if (gStorageService) {
        NS_ADDREF(gStorageService);
        return gStorageService;
    }
    
    gStorageService = new mozStorageService();
    if (gStorageService) {
        NS_ADDREF(gStorageService);
        if (NS_FAILED(gStorageService->Init()))
            NS_RELEASE(gStorageService);
    }
    
    return gStorageService;
}

mozStorageService::~mozStorageService()
{
    gStorageService = nsnull;
    PR_DestroyLock(mLock);
    PR_DestroyLock(gSingletonLock);
    gSingletonLock = nsnull;
}

nsresult
mozStorageService::Init()
{
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    
    int rc = sqlite3_enable_shared_cache(1);
    if (rc != SQLITE_OK)
        return ConvertResultCode(rc);

    return NS_OK;
}

#ifndef NS_APP_STORAGE_50_FILE
#define NS_APP_STORAGE_50_FILE "UStor"
#endif


NS_IMETHODIMP
mozStorageService::OpenSpecialDatabase(const char *aStorageKey, mozIStorageConnection **_retval)
{
    nsresult rv;

    nsCOMPtr<nsIFile> storageFile;
    if (PL_strcmp(aStorageKey, "memory") == 0) {
        
        
    } else if (PL_strcmp(aStorageKey, "profile") == 0) {

        rv = NS_GetSpecialDirectory(NS_APP_STORAGE_50_FILE, getter_AddRefs(storageFile));
        if (NS_FAILED(rv)) {
            
            return rv;
        }

        nsString filename;
        storageFile->GetPath(filename);
        nsCString filename8 = NS_ConvertUTF16toUTF8(filename.get());
        
    } else {
        return NS_ERROR_INVALID_ARG;
    }

    mozStorageConnection *msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = msc->Initialize (storageFile);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = msc);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageService::OpenDatabase(nsIFile *aDatabaseFile, mozIStorageConnection **_retval)
{
    mozStorageConnection *msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    {
        nsAutoLock lock(mLock);
        (void)msc->Initialize(aDatabaseFile);
    }
    NS_ADDREF(*_retval = msc);

    return NS_OK;
}


NS_IMETHODIMP
mozStorageService::OpenUnsharedDatabase(nsIFile *aDatabaseFile, mozIStorageConnection **_retval)
{
    nsRefPtr<mozStorageConnection> msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    
    
    
    
    
    {
        nsAutoLock lock(mLock);
        int rc = sqlite3_enable_shared_cache(0);
        if (rc != SQLITE_OK)
            return ConvertResultCode(rc);

        (void)msc->Initialize(aDatabaseFile);

        rc = sqlite3_enable_shared_cache(1);
        if (rc != SQLITE_OK)
            return ConvertResultCode(rc);
    }
    NS_ADDREF(*_retval = msc);

    return NS_OK;
}
