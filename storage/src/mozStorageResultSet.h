






































#ifndef __mozStorageResultSet_h__
#define __mozStorageResultSet_h__

#include "mozIStorageResultSet.h"
#include "nsCOMArray.h"
class mozIStorageRow;

class mozStorageResultSet : public mozIStorageResultSet
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGERESULTSET

  mozStorageResultSet();
  ~mozStorageResultSet();

  


  nsresult add(mozIStorageRow *aTuple);

private:
  


  PRInt32 mCurrentIndex;
  


  nsCOMArray<mozIStorageRow> mData;
};

#endif 
