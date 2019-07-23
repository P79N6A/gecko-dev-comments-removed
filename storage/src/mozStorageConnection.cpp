






































#include <stdio.h>

#include "nsError.h"
#include "nsIMutableArray.h"
#include "nsIFile.h"

#include "mozIStorageFunction.h"

#include "mozStorageConnection.h"
#include "mozStorageService.h"
#include "mozStorageStatement.h"
#include "mozStorageValueArray.h"

#include "prlog.h"
#include "prprf.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gStorageLog = nsnull;
#endif

NS_IMPL_ISUPPORTS1(mozStorageConnection, mozIStorageConnection)

mozStorageConnection::mozStorageConnection(mozIStorageService* aService)
    : mDBConn(nsnull), mTransactionInProgress(PR_FALSE),
      mStorageService(aService)
{
    
}

mozStorageConnection::~mozStorageConnection()
{
    if (mDBConn) {
        int srv = sqlite3_close (mDBConn);
        if (srv != SQLITE_OK) {
            NS_WARNING("sqlite3_close failed.  There are probably outstanding statements!");
        }

        
        ((mozStorageService*)(mStorageService.get()))->FlushAsyncIO();
    }
}


static nsresult
ConvertResultCode (int srv)
{
    switch (srv) {
        case SQLITE_OK:
            return NS_OK;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB:
            return NS_ERROR_FILE_CORRUPTED;
        case SQLITE_PERM:
        case SQLITE_CANTOPEN:
            return NS_ERROR_FILE_ACCESS_DENIED;
        case SQLITE_BUSY:
            return NS_ERROR_FILE_IS_LOCKED;
    }

    
    return NS_ERROR_FAILURE;
}

#ifdef PR_LOGGING
void tracefunc (void *closure, const char *stmt)
{
    PR_LOG(gStorageLog, PR_LOG_DEBUG, ("%s", stmt));
}
#endif





NS_IMETHODIMP
mozStorageConnection::Initialize(nsIFile *aDatabaseFile)
{
    NS_ASSERTION (!mDBConn, "Initialize called on already opened database!");

    int srv;
    nsresult rv;

    mDatabaseFile = aDatabaseFile;

    if (aDatabaseFile) {
        nsAutoString path;
        rv = aDatabaseFile->GetPath(path);
        NS_ENSURE_SUCCESS(rv, rv);

        srv = sqlite3_open (NS_ConvertUTF16toUTF8(path).get(), &mDBConn);
    } else {
        
        srv = sqlite3_open (":memory:", &mDBConn);
    }
    if (srv != SQLITE_OK) {
        mDBConn = nsnull;
        return ConvertResultCode(srv);
    }

#ifdef PR_LOGGING
    if (! gStorageLog)
        gStorageLog = PR_NewLogModule("mozStorage");

    sqlite3_trace (mDBConn, tracefunc, nsnull);
#endif

    


    sqlite3_stmt *stmt = nsnull;
    nsCString query("SELECT * FROM sqlite_master");
    srv = sqlite3_prepare (mDBConn, query.get(), query.Length(), &stmt, nsnull);
 
    if (srv == SQLITE_OK) {
        srv = sqlite3_step(stmt);
 
        if (srv == SQLITE_DONE || srv == SQLITE_ROW)
            srv = SQLITE_OK;
    } else {
        stmt = nsnull;
    }

    if (stmt != nsnull)
        sqlite3_finalize (stmt);
 
    if (srv != SQLITE_OK) {
        sqlite3_close (mDBConn);
        mDBConn = nsnull;

        
        ((mozStorageService*)(mStorageService.get()))->FlushAsyncIO();

        return ConvertResultCode(srv);
    }

    mFunctions = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}









NS_IMETHODIMP
mozStorageConnection::GetConnectionReady(PRBool *aConnectionReady)
{
    *aConnectionReady = (mDBConn != nsnull);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::GetDatabaseFile(nsIFile **aFile)
{
    NS_ASSERTION(mDBConn, "connection not initialized");

    NS_IF_ADDREF(*aFile = mDatabaseFile);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::GetLastInsertRowID(PRInt64 *aLastInsertRowID)
{
    NS_ASSERTION(mDBConn, "connection not initialized");

    sqlite_int64 id = sqlite3_last_insert_rowid(mDBConn);
    *aLastInsertRowID = id;

    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::GetLastError(PRInt32 *aLastError)
{
    NS_ASSERTION(mDBConn, "connection not initialized");

    *aLastError = sqlite3_errcode(mDBConn);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::GetLastErrorString(nsACString& aLastErrorString)
{
    NS_ASSERTION(mDBConn, "connection not initialized");

    const char *serr = sqlite3_errmsg(mDBConn);
    aLastErrorString.Assign(serr);

    return NS_OK;
}





NS_IMETHODIMP
mozStorageConnection::CreateStatement(const nsACString& aSQLStatement,
                                      mozIStorageStatement **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    NS_ASSERTION(mDBConn, "connection not initialized");

    mozStorageStatement *statement = new mozStorageStatement();
    NS_ADDREF(statement);

    nsresult rv = statement->Initialize (this, aSQLStatement);
    if (NS_FAILED(rv)) {
        NS_RELEASE(statement);
        return rv;
    }

    *_retval = statement;
    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::ExecuteSimpleSQL(const nsACString& aSQLStatement)
{
    NS_ENSURE_ARG_POINTER(mDBConn);

    int srv = sqlite3_exec (mDBConn, PromiseFlatCString(aSQLStatement).get(),
                            NULL, NULL, NULL);
    if (srv != SQLITE_OK) {
        HandleSqliteError(nsPromiseFlatCString(aSQLStatement).get());
        return ConvertResultCode(srv);
    }

    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::TableExists(const nsACString& aSQLStatement, PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(mDBConn);

    nsCString query("SELECT name FROM sqlite_master WHERE type = 'table' AND name ='");
    query.Append(aSQLStatement);
    query.AppendLiteral("'");

    sqlite3_stmt *stmt = nsnull;
    int srv = sqlite3_prepare (mDBConn, query.get(), query.Length(), &stmt, nsnull);
    if (srv != SQLITE_OK) {
        HandleSqliteError(query.get());
        return ConvertResultCode(srv);
    }

    PRBool exists = PR_FALSE;

    srv = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);

    if (srv == SQLITE_ROW) {
        exists = PR_TRUE;
    } else if (srv == SQLITE_DONE) {
        exists = PR_FALSE;
    } else if (srv == SQLITE_ERROR) {
        HandleSqliteError("TableExists finalize");
        return NS_ERROR_FAILURE;
    }

    *_retval = exists;
    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::IndexExists(const nsACString& aIndexName, PRBool* _retval)
{
    NS_ENSURE_ARG_POINTER(mDBConn);

    nsCString query("SELECT name FROM sqlite_master WHERE type = 'index' AND name ='");
    query.Append(aIndexName);
    query.AppendLiteral("'");

    sqlite3_stmt *stmt = nsnull;
    int srv = sqlite3_prepare(mDBConn, query.get(), query.Length(), &stmt, nsnull);
    if (srv != SQLITE_OK) {
        HandleSqliteError(query.get());
        return ConvertResultCode(srv);
    }

    PRBool exists = PR_FALSE;

    srv = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);

    if (srv == SQLITE_ROW) {
        exists = PR_TRUE;
    } else if (srv == SQLITE_DONE) {
        exists = PR_FALSE;
    } else if (srv == SQLITE_ERROR) {
        HandleSqliteError("IndexExists finalize");
        return NS_ERROR_FAILURE;
    }

    *_retval = exists;
    return NS_OK;
}






NS_IMETHODIMP
mozStorageConnection::GetTransactionInProgress(PRBool *_retval)
{
    *_retval = mTransactionInProgress;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageConnection::BeginTransaction()
{
    if (mTransactionInProgress)
        return NS_ERROR_FAILURE; 
    nsresult rv = ExecuteSimpleSQL (NS_LITERAL_CSTRING("BEGIN TRANSACTION"));
    if (NS_SUCCEEDED(rv))
        mTransactionInProgress = PR_TRUE;
    return rv;
}

NS_IMETHODIMP
mozStorageConnection::BeginTransactionAs(PRInt32 aTransactionType)
{
    if (mTransactionInProgress)
        return NS_ERROR_FAILURE; 
    nsresult rv;
    switch(aTransactionType) {
        case TRANSACTION_DEFERRED:
            rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN DEFERRED"));
            break;
        case TRANSACTION_IMMEDIATE:
            rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN IMMEDIATE"));
            break;
        case TRANSACTION_EXCLUSIVE:
            rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN EXCLUSIVE"));
            break;
        default:
            return NS_ERROR_ILLEGAL_VALUE;
    }
    if (NS_SUCCEEDED(rv))
        mTransactionInProgress = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
mozStorageConnection::CommitTransaction()
{
    if (!mTransactionInProgress)
        return NS_ERROR_FAILURE;
    nsresult rv = ExecuteSimpleSQL (NS_LITERAL_CSTRING("COMMIT TRANSACTION"));
    
    mTransactionInProgress = PR_FALSE;
    return rv;
}

NS_IMETHODIMP
mozStorageConnection::RollbackTransaction()
{
    if (!mTransactionInProgress)
        return NS_ERROR_FAILURE;
    nsresult rv = ExecuteSimpleSQL (NS_LITERAL_CSTRING("ROLLBACK TRANSACTION"));
    mTransactionInProgress = PR_FALSE;
    return rv;
}





NS_IMETHODIMP
mozStorageConnection::CreateTable(
                                  const char *aTableName,
                                  const char *aTableSchema)
{
    int srv;
    char *buf;

    buf = PR_smprintf("CREATE TABLE %s (%s)", aTableName, aTableSchema);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    srv = sqlite3_exec (mDBConn, buf,
                        NULL, NULL, NULL);

    PR_smprintf_free(buf);

    return ConvertResultCode(srv);
}





static void
mozStorageSqlFuncHelper (sqlite3_context *ctx,
                         int argc,
                         sqlite3_value **argv)
{
    void *userData = sqlite3_user_data (ctx);
    
    mozIStorageFunction *userFunction = NS_STATIC_CAST(mozIStorageFunction *, userData);

    nsCOMPtr<mozStorageArgvValueArray> ava = new mozStorageArgvValueArray (argc, argv);
    nsresult rv = userFunction->OnFunctionCall (ava);
    if (NS_FAILED(rv)) {
        NS_WARNING("mozIStorageConnection: User function returned error code!\n");
    }
}

NS_IMETHODIMP
mozStorageConnection::CreateFunction(const char *aFunctionName,
                                     PRInt32 aNumArguments,
                                     mozIStorageFunction *aFunction)
{
    nsresult rv;

    
    
    PRUint32 idx;
    rv = mFunctions->IndexOf (0, aFunction, &idx);
    if (rv != NS_ERROR_FAILURE) {
        
        return NS_ERROR_FAILURE;
    }

    int srv = sqlite3_create_function (mDBConn,
                                       aFunctionName,
                                       aNumArguments,
                                       SQLITE_ANY,
                                       aFunction,
                                       mozStorageSqlFuncHelper,
                                       nsnull,
                                       nsnull);
    if (srv != SQLITE_OK) {
        HandleSqliteError(nsnull);
        return ConvertResultCode(srv);
    }

    rv = mFunctions->AppendElement (aFunction, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}





nsresult
mozStorageConnection::Preload()
{
  int srv = sqlite3Preload(mDBConn);
  return ConvertResultCode(srv);
}




void
mozStorageConnection::HandleSqliteError(const char *aSqlStatement)
{
    
#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Sqlite error: %d '%s'", sqlite3_errcode(mDBConn), sqlite3_errmsg(mDBConn)));
    if (aSqlStatement)
        PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Statement was: %s", aSqlStatement));
#endif
}
