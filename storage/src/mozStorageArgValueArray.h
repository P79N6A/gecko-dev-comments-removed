






































#ifndef _mozStorageArgValueArray_h_
#define _mozStorageArgValueArray_h_

#include "mozIStorageValueArray.h"

#include "sqlite3.h"

namespace mozilla {
namespace storage {

class ArgValueArray : public mozIStorageValueArray
{
public:
  ArgValueArray(PRInt32 aArgc, sqlite3_value **aArgv);

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEVALUEARRAY

private:
  PRUint32 mArgc;
  sqlite3_value **mArgv;
};

} 
} 

#endif 
