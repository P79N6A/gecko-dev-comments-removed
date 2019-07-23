








































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
    PR_DestroyLock(mLock);
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

