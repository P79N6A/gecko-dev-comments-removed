





































#include "nsError.h"
#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageValueArray.h"






NS_IMPL_ISUPPORTS1(mozStorageStatementRowValueArray, mozIStorageValueArray)

mozStorageStatementRowValueArray::mozStorageStatementRowValueArray(sqlite3_stmt *aSqliteStmt)
{
    mSqliteStatement = aSqliteStmt;
    mNumEntries = sqlite3_data_count (aSqliteStmt);
}

mozStorageStatementRowValueArray::~mozStorageStatementRowValueArray()
{
    
}


NS_IMETHODIMP
mozStorageStatementRowValueArray::GetNumEntries(PRUint32 *aLength)
{
    *aLength = mNumEntries;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementRowValueArray::GetTypeOfIndex(PRUint32 aIndex, PRInt32 *_retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    int t = sqlite3_column_type (mSqliteStatement, aIndex);
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
mozStorageStatementRowValueArray::GetInt32(PRUint32 aIndex, PRInt32 *_retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    *_retval = sqlite3_column_int (mSqliteStatement, aIndex);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetInt64(PRUint32 aIndex, PRInt64 *_retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    *_retval = sqlite3_column_int64 (mSqliteStatement, aIndex);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetDouble(PRUint32 aIndex, double *_retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    *_retval = sqlite3_column_double (mSqliteStatement, aIndex);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetUTF8String(PRUint32 aIndex, nsACString &_retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    PRInt32 type;
    nsresult rv = GetTypeOfIndex (aIndex, &type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == SQLITE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        PRUint32 slen = (PRUint32) sqlite3_column_bytes (mSqliteStatement, aIndex);
        const char *cstr = (const char *) sqlite3_column_text (mSqliteStatement, aIndex);
        _retval.Assign(cstr, slen);
    }
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetString(PRUint32 aIndex, nsAString & _retval)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    PRInt32 type;
    nsresult rv = GetTypeOfIndex (aIndex, &type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == SQLITE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_column_bytes16 (mSqliteStatement, aIndex);
        const PRUnichar *wstr = (const PRUnichar *) sqlite3_column_text16 (mSqliteStatement, aIndex);
        _retval.Assign (wstr, slen/2);
    }
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetBlob(PRUint32 aIndex, PRUint32 *aDataSize, PRUint8 **aData)
{
    NS_ASSERTION (aIndex < mNumEntries, "aIndex out of range");

    int blobsize = sqlite3_column_bytes (mSqliteStatement, aIndex);
    const void *blob = sqlite3_column_blob (mSqliteStatement, aIndex);

    void *blobcopy = nsMemory::Clone(blob, blobsize);
    if (blobcopy == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    *aData = (PRUint8*) blobcopy;
    *aDataSize = blobsize;

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetIsNull(PRUint32 aIndex, PRBool *_retval)
{
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    if (NS_FAILED(rv))
        return rv;

    if (t == VALUE_TYPE_NULL)
        *_retval = PR_TRUE;
    else
        *_retval = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetSharedUTF8String(PRUint32 aIndex, PRUint32 *aLength, const char **_retval)
{
    if (aLength) {
        int slen = sqlite3_column_bytes (mSqliteStatement, aIndex);
        *aLength = slen;
    }

    *_retval = (const char*) sqlite3_column_text (mSqliteStatement, aIndex);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetSharedString(PRUint32 aIndex, PRUint32 *aLength, const PRUnichar **_retval)
{
    if (aLength) {
        int slen = sqlite3_column_bytes16 (mSqliteStatement, aIndex);
        *aLength = slen;
    }

    *_retval = (const PRUnichar *) sqlite3_column_text16 (mSqliteStatement, aIndex);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementRowValueArray::GetSharedBlob(PRUint32 aIndex, PRUint32 *aDataSize, const PRUint8 **aData)
{
    *aDataSize = sqlite3_column_bytes (mSqliteStatement, aIndex);
    *aData = (const PRUint8*) sqlite3_column_blob (mSqliteStatement, aIndex);

    return NS_OK;
}







NS_IMPL_ISUPPORTS1(mozStorageArgvValueArray, mozIStorageValueArray)

mozStorageArgvValueArray::mozStorageArgvValueArray(PRInt32 aArgc, sqlite3_value **aArgv)
    : mArgc(aArgc), mArgv(aArgv)
{
}

mozStorageArgvValueArray::~mozStorageArgvValueArray()
{
    
}


NS_IMETHODIMP
mozStorageArgvValueArray::GetNumEntries(PRUint32 *aLength)
{
    *aLength = mArgc;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageArgvValueArray::GetTypeOfIndex(PRUint32 aIndex, PRInt32 *_retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    int t = sqlite3_value_type (mArgv[aIndex]);
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
mozStorageArgvValueArray::GetInt32(PRUint32 aIndex, PRInt32 *_retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    *_retval = sqlite3_value_int (mArgv[aIndex]);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetInt64(PRUint32 aIndex, PRInt64 *_retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    *_retval = sqlite3_value_int64 (mArgv[aIndex]);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetDouble(PRUint32 aIndex, double *_retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    *_retval = sqlite3_value_double (mArgv[aIndex]);

    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetUTF8String(PRUint32 aIndex, nsACString & _retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    if (sqlite3_value_type (mArgv[aIndex]) == SQLITE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_value_bytes (mArgv[aIndex]);
        const unsigned char *cstr = sqlite3_value_text (mArgv[aIndex]);
        _retval.Assign ((char *) cstr, slen);
    }
    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetString(PRUint32 aIndex, nsAString & _retval)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    if (sqlite3_value_type (mArgv[aIndex]) == SQLITE_NULL) {
        
        _retval.Truncate(0);
        _retval.SetIsVoid(PR_TRUE);
    } else {
        int slen = sqlite3_value_bytes16 (mArgv[aIndex]);
        const PRUnichar *wstr = (const PRUnichar *) sqlite3_value_text16 (mArgv[aIndex]);
        _retval.Assign (wstr, slen);
    }
    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetBlob(PRUint32 aIndex, PRUint32 *aDataSize, PRUint8 **aData)
{
    NS_ASSERTION (aIndex < mArgc, "aIndex out of range");

    int blobsize = sqlite3_value_bytes (mArgv[aIndex]);
    const void *blob = sqlite3_value_blob (mArgv[aIndex]);

    void *blobcopy = nsMemory::Clone(blob, blobsize);
    if (blobcopy == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    *aData = (PRUint8*) blobcopy;
    *aDataSize = blobsize;

    return NS_OK;
}


NS_IMETHODIMP
mozStorageArgvValueArray::GetIsNull(PRUint32 aIndex, PRBool *_retval)
{
    PRInt32 t;
    nsresult rv = GetTypeOfIndex (aIndex, &t);
    if (NS_FAILED(rv))
        return rv;

    if (t == VALUE_TYPE_NULL)
        *_retval = PR_TRUE;
    else
        *_retval = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetSharedUTF8String(PRUint32 aIndex, PRUint32 *aLength, const char **_retval)
{
    if (aLength) {
        int slen = sqlite3_value_bytes (mArgv[aIndex]);
        *aLength = slen;
    }

    *_retval = (const char*) sqlite3_value_text (mArgv[aIndex]);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetSharedString(PRUint32 aIndex, PRUint32 *aLength, const PRUnichar **_retval)
{
    if (aLength) {
        int slen = sqlite3_value_bytes16 (mArgv[aIndex]);
        *aLength = slen;
    }

    *_retval = (const PRUnichar*) sqlite3_value_text16 (mArgv[aIndex]);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageArgvValueArray::GetSharedBlob(PRUint32 aIndex, PRUint32 *aDataSize, const PRUint8 **aData)
{
    *aDataSize = sqlite3_value_bytes (mArgv[aIndex]);
    *aData = (const PRUint8*) sqlite3_value_blob (mArgv[aIndex]);

    return NS_OK;
}
