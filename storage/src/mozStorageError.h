





#ifndef mozStorageError_h
#define mozStorageError_h

#include "mozIStorageError.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace storage {

class Error MOZ_FINAL : public mozIStorageError
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGEERROR

  Error(int aResult, const char *aMessage);

private:
  ~Error() {}

  int mResult;
  nsCString mMessage;
};

} 
} 

#endif 
