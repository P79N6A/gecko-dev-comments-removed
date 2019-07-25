






































#ifndef mozStorageResultSet_h
#define mozStorageResultSet_h

#include "mozIStorageResultSet.h"
#include "nsCOMArray.h"
class mozIStorageRow;

namespace mozilla {
namespace storage {

class ResultSet : public mozIStorageResultSet
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGERESULTSET

  ResultSet();
  ~ResultSet();

  


  nsresult add(mozIStorageRow *aTuple);

  


  PRInt32 rows() const { return mData.Count(); }

private:
  


  PRInt32 mCurrentIndex;

  


  nsCOMArray<mozIStorageRow> mData;
};

} 
} 

#endif 
