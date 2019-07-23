






































#ifndef _MOZSTORAGECONNECTION_H_
#define _MOZSTORAGECONNECTION_H_

#include "nsCOMPtr.h"
#include "nsAutoLock.h"

#include "nsString.h"
#include "nsInterfaceHashtable.h"
#include "mozIStorageProgressHandler.h"
#include "mozIStorageConnection.h"

#include "nsIMutableArray.h"

#include <sqlite3.h>

class nsIFile;
class nsIEventTarget;
class nsIThread;
class mozIStorageService;

class mozStorageConnection : public mozIStorageConnection
{
public:

    mozStorageConnection(mozIStorageService* aService);

    NS_IMETHOD Initialize(nsIFile *aDatabaseFile);

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGECONNECTION

    
    sqlite3 *GetNativeConnection() { return mDBConn; }

    






    already_AddRefed<nsIEventTarget> getAsyncExecutionTarget();

private:
    ~mozStorageConnection();

protected:
    struct FindFuncEnumArgs {
        nsISupports *mTarget;
        PRBool       mFound;
    };

    void HandleSqliteError(const char *aSqlStatement);
    static PLDHashOperator s_FindFuncEnum(const nsACString &aKey,
                                          nsISupports* aData, void* userArg);
    PRBool FindFunctionByInstance(nsISupports *aInstance);

    static int s_ProgressHelper(void *arg);
    
    
    
    int ProgressHandler();

    sqlite3 *mDBConn;
    nsCOMPtr<nsIFile> mDatabaseFile;

    


    PRLock *mAsyncExecutionMutex;

    




    nsCOMPtr<nsIThread> mAsyncExecutionThread;

    PRLock *mTransactionMutex;
    PRBool mTransactionInProgress;

    PRLock *mFunctionsMutex;
    nsInterfaceHashtable<nsCStringHashKey, nsISupports> mFunctions;

    PRLock *mProgressHandlerMutex;
    nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

    
    
    
    nsCOMPtr<mozIStorageService> mStorageService;
};

#endif 
