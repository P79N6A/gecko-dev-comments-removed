





































#ifndef _MOZSTORAGECONNECTION_H_
#define _MOZSTORAGECONNECTION_H_

#include "nsCOMPtr.h"

#include "nsString.h"
#include "mozIStorageConnection.h"

#include "nsIMutableArray.h"

#include <sqlite3.h>

class nsIFile;
class mozIStorageService;

class mozStorageConnection : public mozIStorageConnection
{
public:

    mozStorageConnection(mozIStorageService* aService);

    NS_IMETHOD Initialize(nsIFile *aDatabaseFile);

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGECONNECTION

    
    sqlite3 *GetNativeConnection() { return mDBConn; }

private:
    ~mozStorageConnection();

protected:
    void HandleSqliteError(const char *aSqlStatement);

    sqlite3 *mDBConn;
    nsCOMPtr<nsIFile> mDatabaseFile;
    PRBool mTransactionInProgress;

    nsCOMPtr<nsIMutableArray> mFunctions;

    
    
    
    nsCOMPtr<mozIStorageService> mStorageService;
};

#endif 
