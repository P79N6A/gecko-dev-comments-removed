





#ifndef mozStorageArgValueArray_h
#define mozStorageArgValueArray_h

#include "mozIStorageValueArray.h"
#include "mozilla/Attributes.h"

#include "sqlite3.h"

namespace mozilla {
namespace storage {

class ArgValueArray MOZ_FINAL : public mozIStorageValueArray
{
public:
  ArgValueArray(int32_t aArgc, sqlite3_value **aArgv);

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEVALUEARRAY

private:
  uint32_t mArgc;
  sqlite3_value **mArgv;
};

} 
} 

#endif 
