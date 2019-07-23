






































#ifndef __mozStorageError_h__
#define __mozStorageError_h__

#include "mozIStorageError.h"
#include "nsString.h"

class mozStorageError : public mozIStorageError
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEERROR

  mozStorageError(int aResult, const char *aMessage);

private:
  int mResult;
  nsCString mMessage;
};

#endif 
