






































#include "nsString.h"

#include "sqlite3.h"
#include "mozStorageVariant.h"
#include "mozStorageRow.h"

using namespace mozilla::storage;








NS_IMPL_THREADSAFE_ISUPPORTS2(
  mozStorageRow,
  mozIStorageRow,
  mozIStorageValueArray
)

nsresult
mozStorageRow::initialize(sqlite3_stmt *aStatement)
{
  
  NS_ENSURE_TRUE(mNameHashtable.Init(), NS_ERROR_OUT_OF_MEMORY);

  
  mNumCols = sqlite3_column_count(aStatement);

  
  for (PRUint32 i = 0; i < mNumCols; i++) {
    
    nsIVariant *variant = nsnull;
    int type = sqlite3_column_type(aStatement, i);
    switch (type) {
      case SQLITE_INTEGER:
        variant = new IntegerVariant(sqlite3_column_int64(aStatement, i));
        break;
      case SQLITE_FLOAT:
        variant = new FloatVariant(sqlite3_column_double(aStatement, i));
        break;
      case SQLITE_TEXT:
      {
        nsDependentString str(
          static_cast<const PRUnichar *>(sqlite3_column_text16(aStatement, i))
        );
        variant = new TextVariant(str);
        break;
      }
      case SQLITE_NULL:
        variant = new NullVariant();
        break;
      case SQLITE_BLOB:
      {
        int size = sqlite3_column_bytes(aStatement, i);
        const void *data = sqlite3_column_blob(aStatement, i);
        variant = new BlobVariant(std::pair<const void *, int>(data, size));
        break;
      }
      default:
        return NS_ERROR_UNEXPECTED;
    }
    NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

    
    NS_ENSURE_TRUE(mData.InsertObjectAt(variant, i), NS_ERROR_OUT_OF_MEMORY);

    
    const char *name = sqlite3_column_name(aStatement, i);
    if (!name) break;
    nsCAutoString colName(name);
    mNameHashtable.Put(colName, i);
  }

  return NS_OK;
}




NS_IMETHODIMP
mozStorageRow::GetResultByIndex(PRUint32 aIndex, nsIVariant **_result)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  NS_ADDREF(*_result = mData.ObjectAt(aIndex));
  return NS_OK;
}

NS_IMETHODIMP
mozStorageRow::GetResultByName(const nsACString &aName, nsIVariant **_result)
{
  PRUint32 index;
  NS_ENSURE_TRUE(mNameHashtable.Get(aName, &index), NS_ERROR_NOT_AVAILABLE);
  return GetResultByIndex(index, _result);
}




NS_IMETHODIMP
mozStorageRow::GetNumEntries(PRUint32 *_entries)
{
  *_entries = mNumCols;
  return NS_OK;
}

NS_IMETHODIMP
mozStorageRow::GetTypeOfIndex(PRUint32 aIndex, PRInt32 *_type)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  PRUint16 type;
  (void)mData.ObjectAt(aIndex)->GetDataType(&type);
  switch (type) {
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_INT64:
      *_type = mozIStorageValueArray::VALUE_TYPE_INTEGER;
      break;
    case nsIDataType::VTYPE_DOUBLE:
      *_type = mozIStorageValueArray::VALUE_TYPE_FLOAT;
      break;
    case nsIDataType::VTYPE_ASTRING:
      *_type = mozIStorageValueArray::VALUE_TYPE_TEXT;
      break;
    case nsIDataType::VTYPE_ARRAY:
      *_type = mozIStorageValueArray::VALUE_TYPE_BLOB;
      break;
    default:
      *_type = mozIStorageValueArray::VALUE_TYPE_NULL;
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP
mozStorageRow::GetInt32(PRUint32 aIndex, PRInt32 *_value)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  return mData.ObjectAt(aIndex)->GetAsInt32(_value);
}

NS_IMETHODIMP
mozStorageRow::GetInt64(PRUint32 aIndex, PRInt64 *_value)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  return mData.ObjectAt(aIndex)->GetAsInt64(_value);
}

NS_IMETHODIMP
mozStorageRow::GetDouble(PRUint32 aIndex, double *_value)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  return mData.ObjectAt(aIndex)->GetAsDouble(_value);
}

NS_IMETHODIMP
mozStorageRow::GetUTF8String(PRUint32 aIndex, nsACString &_value)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  return mData.ObjectAt(aIndex)->GetAsAUTF8String(_value);
}

NS_IMETHODIMP
mozStorageRow::GetString(PRUint32 aIndex, nsAString &_value)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  return mData.ObjectAt(aIndex)->GetAsAString(_value);
}

NS_IMETHODIMP
mozStorageRow::GetBlob(PRUint32 aIndex, PRUint32 *_size, PRUint8 **_blob)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  PRUint16 type;
  nsIID interfaceIID;
  return mData.ObjectAt(aIndex)->GetAsArray(&type, &interfaceIID, _size,
                                            reinterpret_cast<void **>(_blob));
}

NS_IMETHODIMP
mozStorageRow::GetIsNull(PRUint32 aIndex, PRBool *_isNull)
{
  if (aIndex >= mNumCols)
    return NS_ERROR_ILLEGAL_VALUE;

  PRUint16 type;
  (void)mData.ObjectAt(aIndex)->GetDataType(&type);
  *_isNull = type == nsIDataType::VTYPE_VOID;
  return NS_OK;
}

NS_IMETHODIMP
mozStorageRow::GetSharedUTF8String(PRUint32, PRUint32 *, char const **)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozStorageRow::GetSharedString(PRUint32, PRUint32 *, const PRUnichar **)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozStorageRow::GetSharedBlob(PRUint32, PRUint32 *, const PRUint8 **)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
