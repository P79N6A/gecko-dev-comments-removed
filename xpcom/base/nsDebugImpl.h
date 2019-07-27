





#ifndef nsDebugImpl_h
#define nsDebugImpl_h

#include "nsIDebug2.h"

class nsDebugImpl : public nsIDebug2
{
public:
  nsDebugImpl()
  {
  }
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDEBUG2

  static nsresult Create(nsISupports* aOuter, const nsIID& aIID,
                         void** aInstancePtr);

  





  static void SetMultiprocessMode(const char* aDesc);
};


#define NS_DEBUG_CONTRACTID "@mozilla.org/xpcom/debug;1"
#define NS_DEBUG_CID                                 \
{ /* cb6cdb94-e417-4601-b4a5-f991bf41453d */         \
  0xcb6cdb94,                                        \
    0xe417,                                          \
    0x4601,                                          \
    {0xb4, 0xa5, 0xf9, 0x91, 0xbf, 0x41, 0x45, 0x3d} \
}

#endif 
