







































#include <stdio.h>

#include "nsError.h"
#include "nsISimpleEnumerator.h"
#include "nsMemory.h"

#include "mozStorageConnection.h"
#include "mozStorageStatement.h"
#include "mozStorageValueArray.h"
#include "mozStorage.h"

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gStorageLog;
#endif




class mozStorageStatementRowEnumerator : public nsISimpleEnumerator {
public:
    
    mozStorageStatementRowEnumerator (sqlite3_stmt *aDBStatement);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

private:
    ~mozStorageStatementRowEnumerator ();
protected:
    sqlite3_stmt *mDBStatement;
    PRBool mHasMore;
    PRBool mDidStep;

    void DoRealStep();
};






NS_IMPL_ISUPPORTS2(mozStorageStatement, mozIStorageStatement, mozIStorageValueArray)

mozStorageStatement::mozStorageStatement()
    : mDBConnection (nsnull), mDBStatement(nsnull), mColumnNames(nsnull), mExecuting(PR_FALSE)
{
}

NS_IMETHODIMP
mozStorageStatement::Initialize(mozIStorageConnection *aDBConnection, const nsACString & aSQLStatement)
{
    int srv;

    
    if (mExecuting) {
        return NS_ERROR_FAILURE;
    }

    sqlite3 *db = nsnull;
    
    
    mozStorageConnection *msc = NS_STATIC_CAST(mozStorageConnection*, aDBConnection);
    db = msc->GetNativeConnection();
    NS_ENSURE_TRUE(db != nsnull, NS_ERROR_NULL_POINTER);

    
    if (mDBStatement) {
        sqlite3_finalize(mDBStatement);
        mDBStatement = nsnull;
    }

    int nRetries = 0;

    while (nRetries < 2) {
        srv = sqlite3_prepare (db, nsPromiseFlatCString(aSQLStatement).get(), aSQLStatement.Length(), &mDBStatement, NULL);
        if ((srv == SQLITE_SCHEMA && nRetries != 0) ||
            (srv != SQLITE_SCHEMA && srv != SQLITE_OK))
        {
#ifdef PR_LOGGING
            PR_LOG(gStorageLog, PR_LOG_ERROR, ("Sqlite statement prepare error: %d '%s'", srv, sqlite3_errmsg(db)));
            PR_LOG(gStorageLog, PR_LOG_ERROR, ("Statement was: '%s'", nsPromiseFlatCString(aSQLStatement).get()));
#endif
            return NS_ERROR_FAILURE;
        }

        if (srv == SQLITE_OK)
            break;

        nRetries++;
    }

    mDBConnection = aDBConnection;
    mStatementString.Assign (aSQLStatement);
    mParamCount = sqlite3_bind_parameter_count (mDBStatement);
    mResultColumnCount = sqlite3_column_count (mDBStatement);
    mColumnNames.Clear();

    for (unsigned int i = 0; i < mResultColumnCount; i++) {
        const void *name = sqlite3_column_name16 (mDBStatement, i);
        if (name != nsnull)
            mColumnNames.AppendString(nsDependentString(NS_STATIC_CAST(const PRUnichar*, name)));
        else
            mColumnNames.AppendString(EmptyString());
    }

    
    
    
    
    sqlite3_exec (db, "", 0, 0, 0);

    return NS_OK;
}

mozStorageStatement::~mozStorageStatement()
{
    if (mDBStatement)
        sqlite3_finalize (mDBStatement);
}


NS_IMETHODIMP
mozStorageStatement::Clone(mozIStorageStatement **_retval)
{
    mozStorageStatement *mss = new mozStorageStatement();
    if (!mss)
      return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = mss->Initialize (mDBConnection, mStatementString);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = mss);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetParameterCount(PRUint32 *aParameterCount)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    NS_ENSURE_ARG_POINTER(aParameterCount);

    *aParameterCount = mParamCount;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetParameterName(PRUint32 aParamIndex, nsACString & _retval)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    
    
    
    
    if (aParamIndex < 0 || aParamIndex >= mParamCount)
        return NS_ERROR_ILLEGAL_VALUE;

    const char *pname = sqlite3_bind_parameter_name(mDBStatement, aParamIndex + 1);
    if (pname == NULL) {
        
        nsCAutoString pname(":");
        pname.AppendInt(aParamIndex);
        _retval.Assign(pname);
    } else {
        _retval.Assign(nsDependentCString(pname));
    }

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetParameterIndexes(const nsACString &aParameterName, PRUint32 *aCount, PRUint32 **aIndexes)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    NS_ENSURE_ARG_POINTER(aCount);
    NS_ENSURE_ARG_POINTER(aIndexes);

    int *indexes, count;
    count = sqlite3_bind_parameter_indexes(mDBStatement, nsPromiseFlatCString(aParameterName).get(), &indexes);
    if (count) {
        *aIndexes = (PRUint32*) nsMemory::Alloc(sizeof(PRUint32) * count);
        for (int i = 0; i < count; i++)
            (*aIndexes)[i] = indexes[i];
        sqlite3_free_parameter_indexes(indexes);
        *aCount = count;
    } else {
        *aCount = 0;
        *aIndexes = nsnull;
    }
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnCount(PRUint32 *aColumnCount)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    NS_ENSURE_ARG_POINTER(aColumnCount);

    *aColumnCount = mResultColumnCount;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnName(PRUint32 aColumnIndex, nsACString & _retval)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    
    
    if (aColumnIndex < 0 || aColumnIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;

    const char *cname = sqlite3_column_name(mDBStatement, aColumnIndex);
    _retval.Assign(nsDependentCString(cname));

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::Reset()
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Resetting statement: '%s'", nsPromiseFlatCString(mStatementString).get()));

    sqlite3_reset(mDBStatement);
    sqlite3_clear_bindings(mDBStatement);

    mExecuting = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::BindUTF8StringParameter(PRUint32 aParamIndex, const nsACString & aValue)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_text (mDBStatement, aParamIndex + 1,
                                 nsPromiseFlatCString(aValue).get(),
                                 aValue.Length(), SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindStringParameter(PRUint32 aParamIndex, const nsAString & aValue)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_text16 (mDBStatement, aParamIndex + 1,
                                   nsPromiseFlatString(aValue).get(),
                                   aValue.Length() * 2, SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindDoubleParameter(PRUint32 aParamIndex, double aValue)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_double (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindInt32Parameter(PRUint32 aParamIndex, PRInt32 aValue)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_int (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindInt64Parameter(PRUint32 aParamIndex, PRInt64 aValue)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_int64 (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindNullParameter(PRUint32 aParamIndex)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_null (mDBStatement, aParamIndex + 1);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindBlobParameter(PRUint32 aParamIndex, const PRUint8 *aValue, PRUint32 aValueSize)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    int srv = sqlite3_bind_blob (mDBStatement, aParamIndex + 1, aValue,
                                 aValueSize, SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::Execute()
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");

    PRBool ret;
    nsresult rv = ExecuteStep(&ret);
    NS_ENSURE_SUCCESS(rv, rv);

    return Reset();
}


NS_IMETHODIMP
mozStorageStatement::ExecuteStep(PRBool *_retval)
{
    NS_ASSERTION (mDBConnection && mDBStatement, "statement not initialized");
    nsresult rv;

    if (mExecuting == PR_FALSE) {
        
        if (sqlite3_expired(mDBStatement)) {
            PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Statement expired, recreating before step"));
            rv = Recreate();
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    int nRetries = 0;

    while (nRetries < 2) {
        int srv = sqlite3_step (mDBStatement);

#ifdef PR_LOGGING
        if (srv != SQLITE_ROW && srv != SQLITE_DONE)
        {
            nsCAutoString errStr;
            mDBConnection->GetLastErrorString(errStr);
            PR_LOG(gStorageLog, PR_LOG_DEBUG, ("mozStorageStatement::ExecuteStep error: %s", errStr.get()));
        }
#endif

        
        if (srv == SQLITE_ROW) {
            
            mExecuting = PR_TRUE;
            *_retval = PR_TRUE;
            return NS_OK;
        } else if (srv == SQLITE_DONE) {
            
            mExecuting = PR_FALSE;
            *_retval = PR_FALSE;
            return NS_OK;
        } else if (srv == SQLITE_BUSY ||
                   srv == SQLITE_MISUSE)
        {
            mExecuting = PR_FALSE;
            return NS_ERROR_FAILURE;
        } else if (srv == SQLITE_SCHEMA) {
            
            NS_NOTREACHED("sqlite3_step returned SQLITE_SCHEMA!");
            return NS_ERROR_FAILURE;
        } else if (srv == SQLITE_ERROR) {
            
            
            
            if (mExecuting == PR_TRUE) {
                PR_LOG(gStorageLog, PR_LOG_ERROR, ("SQLITE_ERROR after mExecuting was true!"));

                mExecuting = PR_FALSE;
                return NS_ERROR_FAILURE;
            }

            srv = sqlite3_reset(mDBStatement);
            if (srv == SQLITE_SCHEMA) {
                rv = Recreate();
                NS_ENSURE_SUCCESS(rv, rv);

                nRetries++;
            } else {
                return NS_ERROR_FAILURE;
            }
        } else {
            
            NS_ERROR ("sqlite3_step returned an error code we don't know about!");
        }
    }

    
    return NS_ERROR_FAILURE;
}


sqlite3_stmt*
mozStorageStatement::GetNativeStatementPointer()
{
    return mDBStatement;
}


NS_IMETHODIMP
mozStorageStatement::GetState(PRInt32 *_retval)
{
    if (!mDBConnection || !mDBStatement) {
        *_retval = MOZ_STORAGE_STATEMENT_INVALID;
    } else if (mExecuting) {
        *_retval = MOZ_STORAGE_STATEMENT_EXECUTING;
    } else {
        *_retval = MOZ_STORAGE_STATEMENT_READY;
    }

    return NS_OK;
}

nsresult
mozStorageStatement::Recreate()
{
    nsresult rv;
    int srv;
    sqlite3_stmt *savedStmt = mDBStatement;
    mDBStatement = nsnull;
    rv = Initialize(mDBConnection, mStatementString);
    NS_ENSURE_SUCCESS(rv, rv);

    
    srv = sqlite3_transfer_bindings(savedStmt, mDBStatement);

    
    
    sqlite3_finalize(savedStmt);

    if (srv != SQLITE_OK) {
        PR_LOG(gStorageLog, PR_LOG_ERROR, ("sqlite3_transfer_bindings returned: %d", srv));
        return ConvertResultCode(srv);
    }

    return NS_OK;
}






NS_IMETHODIMP
mozStorageStatement::GetNumEntries(PRUint32 *aLength)
{
    *aLength = mResultColumnCount;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetTypeOfIndex(PRUint32 aIndex, PRInt32 *_retval)
{
    if (aIndex < 0 || aIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;

    if (!mExecuting)
        return NS_ERROR_UNEXPECTED;

    int t = sqlite3_column_type (mDBStatement, aIndex);
    switch (t) {
        case SQLITE_INTEGER:
            *_retval = VALUE_TYPE_INTEGER;
            break;
        case SQLITE_FLOAT:
            *_retval = VALUE_TYPE_FLOAT;
            break;
        case SQLITE_TEXT:
            *_retval = VALUE_TYPE_TEXT;
            break;
        case SQLITE_BLOB:
            *_retval = VALUE_TYPE_BLOB;
            break;
        case SQLITE_NULL:
            *_retval = VALUE_TYPE_NULL;
            break;
        default:
            
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetInt32(PRUint32 aIndex, PRInt32 *_retval)
{
    if (aIndex < 0 || aIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;
    
    if (!mExecuting)
        return NS_ERROR_UNEXPECTED;

    *_retval = sqlite3_column_int (mDBStatement, aIndex);

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetInt64(PRUint32 aIndex, PRInt64 *_retval)
{
    if (aIndex < 0 || aIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;
    
    if (!mExecuting)
        return NS_ERROR_UNEXPECTED;

    *_retval = sqlite3_column_int64 (mDBStatement, aIndex);

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetDouble(PRUint32 aIndex, double *_retval)
{
    if (aIndex < 0 || aIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;
    
    if (!mExecuting)
        return NS_ERROR_UNEXPECTED;

    *_retval = sqlite3_column_double (mDBStatement, aIndex);

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetUTF8String(PRUint32 aIndex, nsACString & _retval)
{
    
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    NS_ENSURE_SUCCESS(rv, rv);
    if (t == VALUE_TYPE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_column_bytes (mDBStatement, aIndex);
        const unsigned char *cstr = sqlite3_column_text (mDBStatement, aIndex);
        _retval.Assign ((char *) cstr, slen);
    }
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetString(PRUint32 aIndex, nsAString & _retval)
{
    
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    NS_ENSURE_SUCCESS(rv, rv);
    if (t == VALUE_TYPE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_column_bytes16 (mDBStatement, aIndex);
        const void *text = sqlite3_column_text16 (mDBStatement, aIndex);
        const PRUnichar *wstr = NS_STATIC_CAST(const PRUnichar *, text);
        _retval.Assign (wstr, slen/2);
    }
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetBlob(PRUint32 aIndex, PRUint32 *aDataSize, PRUint8 **aData)
{
    if (aIndex < 0 || aIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;
    
    if (!mExecuting)
        return NS_ERROR_UNEXPECTED;

    int blobsize = sqlite3_column_bytes (mDBStatement, aIndex);
    if (blobsize == 0) {
      
      *aData = nsnull;
      *aDataSize = 0;
      return NS_OK;
    }
    const void *blob = sqlite3_column_blob (mDBStatement, aIndex);

    void *blobcopy = nsMemory::Clone(blob, blobsize);
    if (blobcopy == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    *aData = (PRUint8*) blobcopy;
    *aDataSize = blobsize;

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetSharedUTF8String(PRUint32 aIndex, PRUint32 *aLength, const char **_retval)
{
    if (aLength) {
        int slen = sqlite3_column_bytes (mDBStatement, aIndex);
        *aLength = slen;
    }

    *_retval = (const char *) sqlite3_column_text (mDBStatement, aIndex);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetSharedString(PRUint32 aIndex, PRUint32 *aLength, const PRUnichar **_retval)
{
    if (aLength) {
        int slen = sqlite3_column_bytes16 (mDBStatement, aIndex);
        *aLength = slen;
    }

    *_retval = (const PRUnichar *) sqlite3_column_text16 (mDBStatement, aIndex);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetSharedBlob(PRUint32 aIndex, PRUint32 *aDataSize, const PRUint8 **aData)
{
    *aDataSize = sqlite3_column_bytes (mDBStatement, aIndex);
    *aData = (const PRUint8*) sqlite3_column_blob (mDBStatement, aIndex);

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetIsNull(PRUint32 aIndex, PRBool *_retval)
{
    
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    NS_ENSURE_SUCCESS(rv, rv);

    if (t == VALUE_TYPE_NULL)
        *_retval = PR_TRUE;
    else
        *_retval = PR_FALSE;

    return NS_OK;
}
