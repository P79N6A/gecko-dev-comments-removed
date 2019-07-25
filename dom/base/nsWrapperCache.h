





































#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"

struct JSObject;
struct JSContext;
class nsContentUtils;

typedef PRUptrdiff PtrBits;

#define NS_WRAPPERCACHE_IID \
{ 0x6f3179a1, 0x36f7, 0x4a5c, \
  { 0x8c, 0xf1, 0xad, 0xc8, 0x7c, 0xde, 0x3e, 0x87 } }








class nsWrapperCache
{
  friend class nsContentUtils;

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

  







  inline JSObject* GetWrapper() const;

  







  JSObject* GetWrapperPreserveColor() const
  {
    return reinterpret_cast<JSObject*>(mWrapperPtrBits & ~kWrapperBitMask);
  }

  void SetWrapper(JSObject* aWrapper)
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    mWrapperPtrBits = reinterpret_cast<PtrBits>(aWrapper) |
                      (mWrapperPtrBits & WRAPPER_IS_PROXY);
  }

  void ClearWrapper()
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    mWrapperPtrBits = 0;
  }

  bool PreservingWrapper()
  {
    return (mWrapperPtrBits & WRAPPER_BIT_PRESERVED) != 0;
  }

  void SetIsProxy()
  {
    mWrapperPtrBits |= WRAPPER_IS_PROXY;
  }

  bool IsProxy()
  {
    return (mWrapperPtrBits & WRAPPER_IS_PROXY) != 0;
  }

  




  virtual JSObject* WrapObject(JSContext *cx) {
    return nsnull;
  }

private:
  
  void SetPreservingWrapper(bool aPreserve)
  {
    if(aPreserve) {
      mWrapperPtrBits |= WRAPPER_BIT_PRESERVED;
    }
    else {
      mWrapperPtrBits &= ~WRAPPER_BIT_PRESERVED;
    }
  }

  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };
  enum { WRAPPER_IS_PROXY = 1 << 1 };
  enum { kWrapperBitMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_PROXY) };

  PtrBits mWrapperPtrBits;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsWrapperCache, NS_WRAPPERCACHE_IID)

#define NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY                                   \
  if ( aIID.Equals(NS_GET_IID(nsWrapperCache)) ) {                            \
    *aInstancePtr = static_cast<nsWrapperCache*>(this);                       \
    return NS_OK;                                                             \
  }

#endif 
