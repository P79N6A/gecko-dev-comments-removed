





#ifndef nsISizeOfEventTarget_h___
#define nsISizeOfEventTarget_h___

#include "nsISupports.h"

#define NS_ISIZEOFEVENTTARGET_IID \
  {0xa1e08cb9, 0x5455, 0x4593, \
    { 0xb4, 0x1f, 0x38, 0x7a, 0x85, 0x44, 0xd0, 0xb5 }}









class nsISizeOfEventTarget : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISIZEOFEVENTTARGET_IID)

  



  virtual size_t
    SizeOfEventTargetIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISizeOfEventTarget, NS_ISIZEOFEVENTTARGET_IID)

#endif 
