






































#ifndef mozStorageError_h
#define mozStorageError_h

#include "mozIStorageError.h"
#include "nsString.h"

namespace mozilla {
namespace storage {

class Error : public mozIStorageError
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEERROR

  Error(int aResult, const char *aMessage);

private:
  int mResult;
  nsCString mMessage;
};

} 
} 

#endif 
