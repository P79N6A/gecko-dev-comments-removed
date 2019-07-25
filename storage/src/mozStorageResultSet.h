





#ifndef mozStorageResultSet_h
#define mozStorageResultSet_h

#include "mozIStorageResultSet.h"
#include "nsCOMArray.h"
#include "mozilla/Attributes.h"
class mozIStorageRow;

namespace mozilla {
namespace storage {

class ResultSet MOZ_FINAL : public mozIStorageResultSet
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGERESULTSET

  ResultSet();
  ~ResultSet();

  


  nsresult add(mozIStorageRow *aTuple);

  


  int32_t rows() const { return mData.Count(); }

private:
  


  int32_t mCurrentIndex;

  


  nsCOMArray<mozIStorageRow> mData;
};

} 
} 

#endif 
