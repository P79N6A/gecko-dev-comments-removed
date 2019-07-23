








































#include <stdio.h>

#include "nsAutoLock.h"
#include "nsError.h"
#include "nsISimpleEnumerator.h"
#include "nsMemory.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"

#include "mozStorageConnection.h"
#include "mozStorageStatement.h"
#include "mozStorageStatementJSHelper.h"
#include "mozStorageValueArray.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageEvents.h"
#include "mozStorageStatementParams.h"
#include "mozStorageStatementRow.h"

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gStorageLog;
#endif




NS_IMPL_CI_INTERFACE_GETTER2(
    mozStorageStatement
,   mozIStorageStatement
,   mozIStorageValueArray
)

class mozStorageStatementClassInfo : public nsIClassInfo
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHODIMP
    GetInterfaces(PRUint32 *_count, nsIID ***_array)
    {
        return NS_CI_INTERFACE_GETTER_NAME(mozStorageStatement)(_count, _array);
    }

    NS_IMETHODIMP
    GetHelperForLanguage(PRUint32 aLanguage, nsISupports **_helper)
    {
        if (aLanguage == nsIProgrammingLanguage::JAVASCRIPT) {
            static mozStorageStatementJSHelper sJSHelper;
            *_helper = &sJSHelper;
            return NS_OK;
        }

        *_helper = nsnull;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetContractID(char **_contractID)
    {
        *_contractID = nsnull;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetClassDescription(char **_desc)
    {
        *_desc = nsnull;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetClassID(nsCID **_id)
    {
        *_id = nsnull;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetImplementationLanguage(PRUint32 *_language)
    {
        *_language = nsIProgrammingLanguage::CPLUSPLUS;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetFlags(PRUint32 *_flags)
    {
        *_flags = nsnull;
        return NS_OK;
    }

    NS_IMETHODIMP
    GetClassIDNoAlloc(nsCID *_cid)
    {
        return NS_ERROR_NOT_AVAILABLE;
    }
};

NS_IMETHODIMP_(nsrefcnt) mozStorageStatementClassInfo::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) mozStorageStatementClassInfo::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1(mozStorageStatementClassInfo, nsIClassInfo)

static mozStorageStatementClassInfo sStatementClassInfo;




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






NS_IMPL_THREADSAFE_ADDREF(mozStorageStatement)
NS_IMPL_THREADSAFE_RELEASE(mozStorageStatement)

NS_INTERFACE_MAP_BEGIN(mozStorageStatement)
    NS_INTERFACE_MAP_ENTRY(mozIStorageStatement)
    NS_INTERFACE_MAP_ENTRY(mozIStorageValueArray)
    if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
        foundInterface = static_cast<nsIClassInfo *>(&sStatementClassInfo);
    } else
    NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

mozStorageStatement::mozStorageStatement()
    : mDBConnection (nsnull), mDBStatement(nsnull), mColumnNames(nsnull), mExecuting(PR_FALSE)
{
}

nsresult
mozStorageStatement::Initialize(mozStorageConnection *aDBConnection,
                                const nsACString & aSQLStatement)
{
    NS_ASSERTION(aDBConnection, "No database connection given!");
    NS_ASSERTION(!mDBStatement, "Calling Initialize on an already initialized statement!");

    sqlite3 *db = aDBConnection->GetNativeConnection();
    NS_ENSURE_TRUE(db != nsnull, NS_ERROR_NULL_POINTER);

    int srv = sqlite3_prepare_v2(db, PromiseFlatCString(aSQLStatement).get(),
                                 -1, &mDBStatement, NULL);
    if (srv != SQLITE_OK) {
#ifdef PR_LOGGING
        PR_LOG(gStorageLog, PR_LOG_ERROR, ("Sqlite statement prepare error: %d '%s'", srv, sqlite3_errmsg(db)));
        PR_LOG(gStorageLog, PR_LOG_ERROR, ("Statement was: '%s'", nsPromiseFlatCString(aSQLStatement).get()));
#endif
        return NS_ERROR_FAILURE;
    }

#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Initialized statement '%s' (0x%p)",
                                        nsPromiseFlatCString(aSQLStatement).get(),
                                        mDBStatement));
#endif

    mDBConnection = aDBConnection;
    mParamCount = sqlite3_bind_parameter_count (mDBStatement);
    mResultColumnCount = sqlite3_column_count (mDBStatement);
    mColumnNames.Clear();

    for (PRUint32 i = 0; i < mResultColumnCount; i++) {
        const char *name = sqlite3_column_name(mDBStatement, i);
        mColumnNames.AppendElement(nsDependentCString(name));
    }

#ifdef DEBUG
    
    
    
    
    const nsCaseInsensitiveCStringComparator c;
    nsACString::const_iterator start, end, e;
    aSQLStatement.BeginReading(start);
    aSQLStatement.EndReading(end);
    e = end;
    while (FindInReadable(NS_LITERAL_CSTRING(" LIKE"), start, e, c)) {
        
        
        
        nsACString::const_iterator s1, s2, s3;
        s1 = s2 = s3 = start;

        if (!(FindInReadable(NS_LITERAL_CSTRING(" LIKE ?"), s1, end, c) ||
              FindInReadable(NS_LITERAL_CSTRING(" LIKE :"), s2, end, c) ||
              FindInReadable(NS_LITERAL_CSTRING(" LIKE @"), s3, end, c))) {
            
            
            
            NS_WARNING("Unsafe use of LIKE detected!  Please ensure that you "
                       "are using mozIStorageConnection::escapeStringForLIKE "
                       "and that you are binding that result to the statement "
                       "to prevent SQL injection attacks.");
        }

        
        start = e;
        e = end;
    }
#endif

    return NS_OK;
}

mozStorageStatement::~mozStorageStatement()
{
    (void)Finalize();
}





NS_IMETHODIMP
mozStorageStatement::Clone(mozIStorageStatement **_retval)
{
    mozStorageStatement *mss = new mozStorageStatement();
    if (!mss)
      return NS_ERROR_OUT_OF_MEMORY;

    nsCAutoString sql(sqlite3_sql(mDBStatement));
    nsresult rv = mss->Initialize (mDBConnection, sql);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = mss);
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::Finalize()
{
    if (!mDBStatement)
        return NS_OK;

#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Finalizing statement '%s'",
                                        sqlite3_sql(mDBStatement)));
#endif

    int srv = sqlite3_finalize(mDBStatement);
    mDBStatement = NULL;

    
    
    if (mStatementParamsHolder) {
        nsCOMPtr<nsIXPConnectWrappedNative> wrapper =
            do_QueryInterface(mStatementParamsHolder);
        nsCOMPtr<mozIStorageStatementParams> iParams =
            do_QueryWrappedNative(wrapper);
        mozStorageStatementParams *params =
            static_cast<mozStorageStatementParams *>(iParams.get());
        params->mStatement = nsnull;
        mStatementParamsHolder = nsnull;
    }

    if (mStatementRowHolder) {
        nsCOMPtr<nsIXPConnectWrappedNative> wrapper =
            do_QueryInterface(mStatementRowHolder);
        nsCOMPtr<mozIStorageStatementRow> iRow =
            do_QueryWrappedNative(wrapper);
        mozStorageStatementRow *row =
            static_cast<mozStorageStatementRow *>(iRow.get());
        row->mStatement = nsnull;
        mStatementRowHolder = nsnull;
    }

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::GetParameterCount(PRUint32 *aParameterCount)
{
    NS_ENSURE_ARG_POINTER(aParameterCount);

    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    *aParameterCount = mParamCount;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetParameterName(PRUint32 aParamIndex, nsACString & _retval)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    
    
    
    
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
mozStorageStatement::GetParameterIndex(const nsACString &aName,
                                       PRUint32 *_retval)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int ind = sqlite3_bind_parameter_index(mDBStatement,
                                           nsPromiseFlatCString(aName).get());
    if (ind  == 0) 
        return NS_ERROR_INVALID_ARG;
    
    *_retval = ind - 1; 

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnCount(PRUint32 *aColumnCount)
{
    NS_ENSURE_ARG_POINTER(aColumnCount);

    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    *aColumnCount = mResultColumnCount;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnName(PRUint32 aColumnIndex, nsACString & _retval)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    
    
    if (aColumnIndex < 0 || aColumnIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;

    const char *cname = sqlite3_column_name(mDBStatement, aColumnIndex);
    _retval.Assign(nsDependentCString(cname));

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnIndex(const nsACString &aName, PRUint32 *_retval)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    
    
    for (PRUint32 i = 0; i < mResultColumnCount; i++) {
        if (mColumnNames[i].Equals(aName)) {
            *_retval = i;
            return NS_OK;
        }
    }

    return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
mozStorageStatement::Reset()
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

#ifdef DEBUG
    PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Resetting statement: '%s'",
                                       sqlite3_sql(mDBStatement)));

    CheckAndLogStatementPerformance(mDBStatement);
#endif

    sqlite3_reset(mDBStatement);
    sqlite3_clear_bindings(mDBStatement);

    mExecuting = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::BindUTF8StringParameter(PRUint32 aParamIndex, const nsACString & aValue)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_text (mDBStatement, aParamIndex + 1,
                                 nsPromiseFlatCString(aValue).get(),
                                 aValue.Length(), SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindStringParameter(PRUint32 aParamIndex, const nsAString & aValue)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_text16 (mDBStatement, aParamIndex + 1,
                                   nsPromiseFlatString(aValue).get(),
                                   aValue.Length() * 2, SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindDoubleParameter(PRUint32 aParamIndex, double aValue)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_double (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindInt32Parameter(PRUint32 aParamIndex, PRInt32 aValue)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_int (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindInt64Parameter(PRUint32 aParamIndex, PRInt64 aValue)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_int64 (mDBStatement, aParamIndex + 1, aValue);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindNullParameter(PRUint32 aParamIndex)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_null (mDBStatement, aParamIndex + 1);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::BindBlobParameter(PRUint32 aParamIndex, const PRUint8 *aValue, PRUint32 aValueSize)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    int srv = sqlite3_bind_blob (mDBStatement, aParamIndex + 1, aValue,
                                 aValueSize, SQLITE_TRANSIENT);

    return ConvertResultCode(srv);
}


NS_IMETHODIMP
mozStorageStatement::Execute()
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    PRBool ret;
    nsresult rv = ExecuteStep(&ret);
    NS_ENSURE_SUCCESS(rv, rv);

    return Reset();
}


NS_IMETHODIMP
mozStorageStatement::ExecuteStep(PRBool *_retval)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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
    } else if (srv == SQLITE_BUSY || srv == SQLITE_MISUSE) {
        mExecuting = PR_FALSE;
    } else if (mExecuting == PR_TRUE) {
#ifdef PR_LOGGING
        PR_LOG(gStorageLog, PR_LOG_ERROR, ("SQLite error after mExecuting was true!"));
#endif
        mExecuting = PR_FALSE;
    }

    return ConvertResultCode(srv);
}


nsresult
mozStorageStatement::ExecuteAsync(mozIStorageStatementCallback *aCallback,
                                  mozIStoragePendingStatement **_stmt)
{
    mozIStorageStatement * stmts[1] = {this};
    return mDBConnection->ExecuteAsync(stmts, 1, aCallback, _stmt);
}


NS_IMETHODIMP
mozStorageStatement::GetState(PRInt32 *_retval)
{
    if (!mDBStatement) {
        *_retval = MOZ_STORAGE_STATEMENT_INVALID;
    } else if (mExecuting) {
        *_retval = MOZ_STORAGE_STATEMENT_EXECUTING;
    } else {
        *_retval = MOZ_STORAGE_STATEMENT_READY;
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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    
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
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

    
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    NS_ENSURE_SUCCESS(rv, rv);
    if (t == VALUE_TYPE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_column_bytes16 (mDBStatement, aIndex);
        const void *text = sqlite3_column_text16 (mDBStatement, aIndex);
        const PRUnichar *wstr = static_cast<const PRUnichar *>(text);
        _retval.Assign (wstr, slen/2);
    }
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetBlob(PRUint32 aIndex, PRUint32 *aDataSize, PRUint8 **aData)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;

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


NS_IMETHODIMP
mozStorageStatement::EscapeStringForLIKE(const nsAString & aValue, 
                                         const PRUnichar aEscapeChar, 
                                         nsAString &aEscapedString)
{
    const PRUnichar MATCH_ALL('%');
    const PRUnichar MATCH_ONE('_');

    aEscapedString.Truncate(0);

    for (PRInt32 i = 0; i < aValue.Length(); i++) {
        if (aValue[i] == aEscapeChar || aValue[i] == MATCH_ALL || 
            aValue[i] == MATCH_ONE)
            aEscapedString += aEscapeChar;
        aEscapedString += aValue[i];
    }
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatement::GetColumnDecltype(PRUint32 aParamIndex,
                                       nsACString& aDeclType)
{
    if (!mDBStatement)
        return NS_ERROR_NOT_INITIALIZED;
    
    if (aParamIndex < 0 || aParamIndex >= mResultColumnCount)
        return NS_ERROR_ILLEGAL_VALUE;

    const char *declType = sqlite3_column_decltype(mDBStatement, aParamIndex);
    aDeclType.Assign(declType);
    
    return NS_OK;
}
