




































#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"

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
    if (PreservingWrapper()) {
      GetWrapper()->Release();
    }
  }

  




  nsISupports* GetWrapper()
  {
    return reinterpret_cast<nsISupports*>(mWrapperPtrBits & ~kWrapperBitMask);
  }

  




  void SetWrapper(nsISupports* aWrapper)
  {
    NS_ASSERTION(!mWrapperPtrBits, "Already have a wrapper!");
    mWrapperPtrBits = reinterpret_cast<PtrBits>(aWrapper);
  }

  void ClearWrapper()
  {
    if (PreservingWrapper()) {
      GetWrapper()->Release();
    }
    mWrapperPtrBits = 0;
  }

  void PreserveWrapper()
  {
    NS_ASSERTION(mWrapperPtrBits, "No wrapper to preserve?");
    if (!PreservingWrapper()) {
      NS_ADDREF(reinterpret_cast<nsISupports*>(mWrapperPtrBits));
      mWrapperPtrBits |= WRAPPER_BIT_PRESERVED;
    }
  }

  void ReleaseWrapper()
  {
    if (PreservingWrapper()) {
      nsISupports* wrapper = GetWrapper();
      mWrapperPtrBits = reinterpret_cast<PtrBits>(wrapper);
      NS_RELEASE(wrapper);
    }
  }

  void TraverseWrapper(nsCycleCollectionTraversalCallback &cb)
  {
    if (PreservingWrapper()) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mWrapper");
      cb.NoteXPCOMChild(GetWrapper());
    }
  }

private:
  PRBool PreservingWrapper()
  {
    return mWrapperPtrBits & WRAPPER_BIT_PRESERVED;
  }

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
