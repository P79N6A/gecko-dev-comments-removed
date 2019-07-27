





#ifndef mozStoragePrivateHelpers_h
#define mozStoragePrivateHelpers_h





#include "sqlite3.h"
#include "nsIVariant.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"
#include "Variant.h"

class mozIStorageCompletionCallback;
class nsIRunnable;

namespace mozilla {
namespace storage {




#define ENSURE_INDEX_VALUE(aIndex, aCount) \
  NS_ENSURE_TRUE(aIndex < aCount, NS_ERROR_INVALID_ARG)











nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);













nsIVariant *convertJSValToVariant(JSContext *aCtx, JS::Value aValue);









Variant_base *convertVariantToStorageVariant(nsIVariant *aVariant);








already_AddRefed<nsIRunnable> newCompletionEvent(
  mozIStorageCompletionCallback *aCallback
);





template<class T, class V>
nsresult
DoGetBlobAsString(T* aThis, uint32_t aIndex, V& aValue)
{
  typedef typename V::char_type char_type;

  uint32_t size;
  char_type* blob;
  nsresult rv =
    aThis->GetBlob(aIndex, &size, reinterpret_cast<uint8_t**>(&blob));
  NS_ENSURE_SUCCESS(rv, rv);

  aValue.Adopt(blob, size / sizeof(char_type));
  return NS_OK;
}





template<class T, class V>
nsresult
DoBindStringAsBlobByName(T* aThis, const nsACString& aName, const V& aValue)
{
  typedef typename V::char_type char_type;
  return aThis->BindBlobByName(aName,
                        reinterpret_cast<const uint8_t*>(aValue.BeginReading()),
                        aValue.Length() * sizeof(char_type));
}





template<class T, class V>
nsresult
DoBindStringAsBlobByIndex(T* aThis, uint32_t aIndex, const V& aValue)
{
  typedef typename V::char_type char_type;
  return aThis->BindBlobByIndex(aIndex,
                        reinterpret_cast<const uint8_t*>(aValue.BeginReading()),
                        aValue.Length() * sizeof(char_type));
}

} 
} 

#endif 
