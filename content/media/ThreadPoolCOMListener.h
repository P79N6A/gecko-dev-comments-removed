





#ifndef MSCOMInitThreadPoolListener_h_
#define MSCOMInitThreadPoolListener_h_

#include "nsIThreadPool.h"
#include <objbase.h>

namespace mozilla {




class MSCOMInitThreadPoolListener MOZ_FINAL : public nsIThreadPoolListener {
  ~MSCOMInitThreadPoolListener() {}
  public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITHREADPOOLLISTENER
};

} 


#endif
