








































#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"
#include "plstr.h"

#include "sqlite3.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(mozStorageService, mozIStorageService)

mozStorageService *mozStorageService::gStorageService = nsnull;

mozStorageService *
mozStorageService::GetSingleton()
{
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
}

nsresult
mozStorageService::Init()
{
    
    
    sqlite3_enable_shared_cache(1);

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
    nsresult rv;

    mozStorageConnection *msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = msc->Initialize (aDatabaseFile);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = msc);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageService::OpenUnsharedDatabase(nsIFile *aDatabaseFile, mozIStorageConnection **_retval)
{
    nsresult rv;

    mozStorageConnection *msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    
    
    
    sqlite3_enable_shared_cache(0);
    rv = msc->Initialize (aDatabaseFile);
    sqlite3_enable_shared_cache(1);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = msc);
    return NS_OK;
}
