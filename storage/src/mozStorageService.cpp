








































#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"
#include "plstr.h"

#include "sqlite3.h"
#include "sqlite3file.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(mozStorageService, mozIStorageService, nsIObserver)

static const char kShutdownMessage[] = "xpcom-shutdown-threads";

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
    FreeLocks();
    gStorageService = nsnull;
}

nsresult
mozStorageService::Init()
{
    
    
    
    
    NS_ENSURE_STATE(NS_IsMainThread());

    
    
    sqlite3_enable_shared_cache(1);

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = 
            do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InitStorageAsyncIO();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->AddObserver(this, kShutdownMessage, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

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
mozStorageService::Observe(nsISupports *aSubject, const char *aTopic,
                           const PRUnichar *aData)
{
    nsresult rv;
    if (strcmp(aTopic, kShutdownMessage) == 0) {
        rv = FinishAsyncIO();
    }
    return rv;
}
