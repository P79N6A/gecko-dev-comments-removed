






































#ifndef __mozStorageRow_h__
#define __mozStorageRow_h__

#include "mozIStorageRow.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
class nsIVariant;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class Row : public mozIStorageRow
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEROW
  NS_DECL_MOZISTORAGEVALUEARRAY

  






  nsresult initialize(sqlite3_stmt *aStatement);

private:
  


  PRUint32 mNumCols;

  


  nsCOMArray<nsIVariant> mData;

  


  nsDataHashtable<nsCStringHashKey, PRUint32> mNameHashtable;
};

} 
} 

#endif 
