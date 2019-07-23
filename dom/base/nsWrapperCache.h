





































#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"

struct JSObject;

typedef PRUptrdiff PtrBits;

#define NS_WRAPPERCACHE_IID \
{ 0x3a51ca81, 0xddab, 0x422c, \
  { 0x95, 0x3a, 0x13, 0x06, 0x28, 0x0e, 0xee, 0x14 } }








class nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_WRAPPERCACHE_IID)

  nsWrapperCache() : mWrapperPtrBits(0)
  {
  }
  ~nsWrapperCache()
  {
    NS_ASSERTION(!PreservingWrapper(),
                 "Destroying cache with a preserved wrapper!");
  }

  JSObject* GetWrapper() const
  {
    return reinterpret_cast<JSObject*>(mWrapperPtrBits & ~kWrapperBitMask);
  }

  void SetWrapper(JSObject* aWrapper)
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    mWrapperPtrBits = reinterpret_cast<PtrBits>(aWrapper);
  }

  void ClearWrapper()
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    mWrapperPtrBits = 0;
  }

  void SetPreservingWrapper(PRBool aPreserve)
  {
    if(aPreserve) {
      mWrapperPtrBits |= WRAPPER_BIT_PRESERVED;
    }
    else {
      mWrapperPtrBits &= ~WRAPPER_BIT_PRESERVED;
    }
  }

  PRBool PreservingWrapper()
  {
    return mWrapperPtrBits & WRAPPER_BIT_PRESERVED;
  }

private:
  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };
  enum { kWrapperBitMask = 0x1 };

  PtrBits mWrapperPtrBits;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsWrapperCache, NS_WRAPPERCACHE_IID)

#define NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY                                   \
  if ( aIID.Equals(NS_GET_IID(nsWrapperCache)) ) {                            \
    *aInstancePtr = static_cast<nsWrapperCache*>(this);                       \
    return NS_OK;                                                             \
  }

#endif
