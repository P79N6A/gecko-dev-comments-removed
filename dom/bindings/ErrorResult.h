









#ifndef mozilla_ErrorResult_h
#define mozilla_ErrorResult_h

#include "nscore.h"
#include "mozilla/Assertions.h"

namespace mozilla {

class ErrorResult {
public:
  ErrorResult() {
    mResult = NS_OK;
  }

  void Throw(nsresult rv) {
    MOZ_ASSERT(NS_FAILED(rv), "Please don't try throwing success");
    mResult = rv;
  }

  
  
  

  
  
  
  void operator=(nsresult rv) {
    mResult = rv;
  }

  bool Failed() const {
    return NS_FAILED(mResult);
  }

  nsresult ErrorCode() const {
    return mResult;
  }

private:
  nsresult mResult;

  
  
  ErrorResult(const ErrorResult&) MOZ_DELETE;
};

} 

#endif 
