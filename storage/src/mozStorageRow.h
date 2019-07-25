





#ifndef mozStorageRow_h
#define mozStorageRow_h

#include "mozIStorageRow.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "mozilla/Attributes.h"
class nsIVariant;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class Row MOZ_FINAL : public mozIStorageRow
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEROW
  NS_DECL_MOZISTORAGEVALUEARRAY

  






  nsresult initialize(sqlite3_stmt *aStatement);

private:
  


  uint32_t mNumCols;

  


  nsCOMArray<nsIVariant> mData;

  


  nsDataHashtable<nsCStringHashKey, uint32_t> mNameHashtable;
};

} 
} 

#endif 
