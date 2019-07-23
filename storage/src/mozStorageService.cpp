








































#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "nsCRT.h"
#include "plstr.h"
#include "prinit.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsEmbedCID.h"
#include "mozStoragePrivateHelpers.h"
#include "nsIXPConnect.h"
#include "nsIObserverService.h"

#include "sqlite3.h"

#include "nsIPromptService.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(mozStorageService, mozIStorageService, nsIObserver)

mozStorageService *mozStorageService::gStorageService = nsnull;

mozStorageService *
mozStorageService::GetSingleton()
{
    if (gStorageService) {
        NS_ADDREF(gStorageService);
        return gStorageService;
    }

    
    
    
    if (SQLITE_VERSION_NUMBER > sqlite3_libversion_number()) {
        nsCOMPtr<nsIPromptService> ps =
            do_GetService(NS_PROMPTSERVICE_CONTRACTID);
        if (ps) {
            nsAutoString title, message;
            title.AppendASCII("SQLite Version Error");
            message.AppendASCII("The application has been updated, but your "
                                "version of SQLite is too old and the "
                                "application cannot run.");
            (void)ps->Alert(nsnull, title.get(), message.get());
        }
        PR_Abort();
    }

    gStorageService = new mozStorageService();
    if (gStorageService) {
        NS_ADDREF(gStorageService);
        if (NS_FAILED(gStorageService->Init()))
            NS_RELEASE(gStorageService);
    }

    return gStorageService;
}

already_AddRefed<nsIXPConnect>
mozStorageService::XPConnect()
{
    NS_ASSERTION(gStorageService,
                 "Can not get XPConnect without an instance of our service!");

    
    
    nsCOMPtr<nsIXPConnect> xpc(sXPConnect);
    if (!xpc)
        xpc = do_GetService(nsIXPConnect::GetCID());
    NS_ASSERTION(xpc, "Could not get XPConnect!");
    return xpc.forget();
}

mozStorageService::~mozStorageService()
{
    
    
    int rc = sqlite3_shutdown();
    if (rc != SQLITE_OK)
        NS_WARNING("sqlite3 did not shutdown cleanly.");

    gStorageService = nsnull;
    PR_DestroyLock(mLock);
}

void
mozStorageService::Shutdown()
{
    NS_IF_RELEASE(sXPConnect);
}

nsIXPConnect *mozStorageService::sXPConnect = nsnull;

nsresult
mozStorageService::Init()
{
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    int rc = sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0);
    if (rc != SQLITE_OK)
        return ConvertResultCode(rc);

    
    
    
    rc = sqlite3_initialize();
    if (rc != SQLITE_OK)
        return ConvertResultCode(rc);

    
    
    
    
    
    rc = sqlite3_enable_shared_cache(1);
    if (rc != SQLITE_OK)
        return ConvertResultCode(rc);

    nsCOMPtr<nsIObserverService> os =
        do_GetService("@mozilla.org/observer-service;1");
    NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

    nsresult rv = os->AddObserver(this, "xpcom-shutdown", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    (void)CallGetService(nsIXPConnect::GetCID(), &sXPConnect);
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
    nsRefPtr<mozStorageConnection> msc = new mozStorageConnection(this);
    if (!msc)
        return NS_ERROR_OUT_OF_MEMORY;

    {
        nsAutoLock lock(mLock);
        nsresult rv = msc->Initialize(aDatabaseFile);
        NS_ENSURE_SUCCESS(rv, rv);
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

    
    
    
    
    
    
    nsresult rv;
    {
        nsAutoLock lock(mLock);
        int rc = sqlite3_enable_shared_cache(0);
        if (rc != SQLITE_OK)
            return ConvertResultCode(rc);

        rv = msc->Initialize(aDatabaseFile);

        rc = sqlite3_enable_shared_cache(1);
        if (rc != SQLITE_OK)
            return ConvertResultCode(rc);
    }
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = msc);
    return NS_OK;
}





NS_IMETHODIMP
mozStorageService::BackupDatabaseFile(nsIFile *aDBFile,
                                      const nsAString &aBackupFileName,
                                      nsIFile *aBackupParentDirectory,
                                      nsIFile **backup)
{
    nsresult rv;
    nsCOMPtr<nsIFile> parentDir = aBackupParentDirectory;
    if (!parentDir) {
        
        
        rv = aDBFile->GetParent(getter_AddRefs(parentDir));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIFile> backupDB;
    rv = parentDir->Clone(getter_AddRefs(backupDB));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = backupDB->Append(aBackupFileName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = backupDB->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString fileName;
    rv = backupDB->GetLeafName(fileName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = backupDB->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    backupDB.swap(*backup);

    return aDBFile->CopyTo(parentDir, fileName);
}




NS_IMETHODIMP
mozStorageService::Observe(nsISupports *, const char *aTopic, const PRUnichar *)
{
    if (strcmp(aTopic, "xpcom-shutdown") == 0)
        Shutdown();
    return NS_OK;
}
