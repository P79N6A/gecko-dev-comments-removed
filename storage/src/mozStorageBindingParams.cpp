






































#include <limits.h>

#include "nsString.h"

#include "mozStorageError.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageBindingParams.h"
#include "mozStorageBindingParamsArray.h"
#include "Variant.h"

namespace mozilla {
namespace storage {




namespace {

struct BindingColumnData
{
  BindingColumnData(sqlite3_stmt *aStmt,
                    int aColumn)
  : stmt(aStmt)
  , column(aColumn)
  {
  }
  sqlite3_stmt *stmt;
  int column;
};

} 




template < >
int
sqlite3_T_int(BindingColumnData aData,
              int aValue)
{
  return ::sqlite3_bind_int(aData.stmt, aData.column + 1, aValue);
}

template < >
int
sqlite3_T_int64(BindingColumnData aData,
                sqlite3_int64 aValue)
{
  return ::sqlite3_bind_int64(aData.stmt, aData.column + 1, aValue);
}

template < >
int
sqlite3_T_double(BindingColumnData aData,
                 double aValue)
{
  return ::sqlite3_bind_double(aData.stmt, aData.column + 1, aValue);
}

template < >
int
sqlite3_T_text16(BindingColumnData aData,
                 nsString aValue)
{
  return ::sqlite3_bind_text16(aData.stmt,
                               aData.column + 1,
                               PromiseFlatString(aValue).get(),
                               aValue.Length() * 2, 
                               SQLITE_TRANSIENT);
}

template < >
int
sqlite3_T_null(BindingColumnData aData)
{
  return ::sqlite3_bind_null(aData.stmt, aData.column + 1);
}

template < >
int
sqlite3_T_blob(BindingColumnData aData,
               const void *aBlob,
               int aSize)
{
  return ::sqlite3_bind_blob(aData.stmt, aData.column + 1, aBlob, aSize,
                             NS_Free);

}




BindingParams::BindingParams(BindingParamsArray *aOwningArray,
                             Statement *aOwningStatement)
: mOwningArray(aOwningArray)
, mOwningStatement(aOwningStatement)
, mLocked(false)
{
  (void)mOwningStatement->GetParameterCount(&mParamCount);
}

void
BindingParams::lock()
{
  NS_ASSERTION(mLocked == false, "Parameters have already been locked!");
  mLocked = true;

  
  
  
  mOwningStatement = nsnull;
  mOwningArray = nsnull;
}

const BindingParamsArray *
BindingParams::getOwner() const
{
  return mOwningArray;
}

already_AddRefed<mozIStorageError>
BindingParams::bind(sqlite3_stmt *aStatement)
{
  
  for (PRInt32 i = 0; i < mParameters.Count(); i++) {
    int rc = variantToSQLiteT(BindingColumnData(aStatement, i), mParameters[i]);
    if (rc != SQLITE_OK) {
      
      
      
      const char *msg = "Could not covert nsIVariant to SQLite type.";
      if (rc != SQLITE_MISMATCH)
        msg = ::sqlite3_errmsg(::sqlite3_db_handle(aStatement));

      nsCOMPtr<mozIStorageError> err(new Error(rc, msg));
      return err.forget();
    }
  }

  
  return nsnull;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(
  BindingParams,
  mozIStorageBindingParams
)




NS_IMETHODIMP
BindingParams::BindByName(const nsACString &aName,
                          nsIVariant *aValue)
{
  NS_ENSURE_FALSE(mLocked, NS_ERROR_UNEXPECTED);

  
  PRUint32 index;
  nsresult rv = mOwningStatement->GetParameterIndex(aName, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  return BindByIndex(index, aValue);
}

NS_IMETHODIMP
BindingParams::BindUTF8StringByName(const nsACString &aName,
                                    const nsACString &aValue)
{
  nsCOMPtr<nsIVariant> value(new UTF8TextVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindStringByName(const nsACString &aName,
                                const nsAString &aValue)
{
  nsCOMPtr<nsIVariant> value(new TextVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindDoubleByName(const nsACString &aName,
                                double aValue)
{
  nsCOMPtr<nsIVariant> value(new FloatVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindInt32ByName(const nsACString &aName,
                               PRInt32 aValue)
{
  nsCOMPtr<nsIVariant> value(new IntegerVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindInt64ByName(const nsACString &aName,
                               PRInt64 aValue)
{
  nsCOMPtr<nsIVariant> value(new IntegerVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindNullByName(const nsACString &aName)
{
  nsCOMPtr<nsIVariant> value(new NullVariant());
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindBlobByName(const nsACString &aName,
                              const PRUint8 *aValue,
                              PRUint32 aValueSize)
{
  NS_ENSURE_ARG_MAX(aValueSize, INT_MAX);
  std::pair<const void *, int> data(
    static_cast<const void *>(aValue),
    int(aValueSize)
  );
  nsCOMPtr<nsIVariant> value(new BlobVariant(data));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByName(aName, value);
}

NS_IMETHODIMP
BindingParams::BindByIndex(PRUint32 aIndex,
                           nsIVariant *aValue)
{
  NS_ENSURE_FALSE(mLocked, NS_ERROR_UNEXPECTED);
  ENSURE_INDEX_VALUE(aIndex, mParamCount);

  
  NS_ENSURE_TRUE(mParameters.InsertObjectAt(aValue, aIndex),
                 NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

NS_IMETHODIMP
BindingParams::BindUTF8StringByIndex(PRUint32 aIndex,
                                     const nsACString &aValue)
{
  nsCOMPtr<nsIVariant> value(new UTF8TextVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindStringByIndex(PRUint32 aIndex,
                                 const nsAString &aValue)
{
  nsCOMPtr<nsIVariant> value(new TextVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindDoubleByIndex(PRUint32 aIndex,
                                 double aValue)
{
  nsCOMPtr<nsIVariant> value(new FloatVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindInt32ByIndex(PRUint32 aIndex,
                                PRInt32 aValue)
{
  nsCOMPtr<nsIVariant> value(new IntegerVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindInt64ByIndex(PRUint32 aIndex,
                                PRInt64 aValue)
{
  nsCOMPtr<nsIVariant> value(new IntegerVariant(aValue));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindNullByIndex(PRUint32 aIndex)
{
  nsCOMPtr<nsIVariant> value(new NullVariant());
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

NS_IMETHODIMP
BindingParams::BindBlobByIndex(PRUint32 aIndex,
                               const PRUint8 *aValue,
                               PRUint32 aValueSize)
{
  NS_ENSURE_ARG_MAX(aValueSize, INT_MAX);
  std::pair<const void *, int> data(
    static_cast<const void *>(aValue),
    int(aValueSize)
  );
  nsCOMPtr<nsIVariant> value(new BlobVariant(data));
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  return BindByIndex(aIndex, value);
}

} 
} 
