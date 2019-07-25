




#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"

struct JSObject;
struct JSContext;
class XPCWrappedNativeScope;

typedef PRUptrdiff PtrBits;

namespace mozilla {
namespace dom {
namespace workers {

class DOMBindingBase;

} 
} 
} 

#define NS_WRAPPERCACHE_IID \
{ 0x6f3179a1, 0x36f7, 0x4a5c, \
  { 0x8c, 0xf1, 0xad, 0xc8, 0x7c, 0xde, 0x3e, 0x87 } }






























class nsWrapperCache
{
  friend class mozilla::dom::workers::DOMBindingBase;

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

  





  JSObject* GetWrapper() const;

  









  JSObject* GetWrapperPreserveColor() const
  {
    return GetJSObjectFromBits();
  }

  void SetWrapper(JSObject* aWrapper)
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    NS_ASSERTION(aWrapper, "Use ClearWrapper!");

    SetWrapperBits(aWrapper);
  }

  



  void ClearWrapper()
  {
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");

    SetWrapperBits(NULL);
  }

  bool PreservingWrapper()
  {
    return (mWrapperPtrBits & WRAPPER_BIT_PRESERVED) != 0;
  }

  void SetIsDOMBinding()
  {
    NS_ASSERTION(!mWrapperPtrBits,
                 "This flag should be set before creating any wrappers.");
    mWrapperPtrBits = WRAPPER_IS_DOM_BINDING;
  }
  void ClearIsDOMBinding()
  {
    NS_ASSERTION(!mWrapperPtrBits || mWrapperPtrBits == WRAPPER_IS_DOM_BINDING,
                 "This flag should be cleared before creating any wrappers.");
    mWrapperPtrBits = 0;
  }

  bool IsDOMBinding() const
  {
    return (mWrapperPtrBits & WRAPPER_IS_DOM_BINDING) != 0;
  }


  







  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
  {
    *triedToWrap = false;
    return nsnull;
  }

  


  bool IsBlack();

  
  void SetPreservingWrapper(bool aPreserve)
  {
    if(aPreserve) {
      mWrapperPtrBits |= WRAPPER_BIT_PRESERVED;
    }
    else {
      mWrapperPtrBits &= ~WRAPPER_BIT_PRESERVED;
    }
  }

private:
  JSObject *GetJSObjectFromBits() const
  {
    return reinterpret_cast<JSObject*>(mWrapperPtrBits & ~kWrapperBitMask);
  }
  void SetWrapperBits(void *aWrapper)
  {
    mWrapperPtrBits = reinterpret_cast<PtrBits>(aWrapper) |
                      (mWrapperPtrBits & WRAPPER_IS_DOM_BINDING);
  }

  










  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };

  



  enum { WRAPPER_IS_DOM_BINDING = 1 << 1 };

  enum { kWrapperBitMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_DOM_BINDING) };

  PtrBits mWrapperPtrBits;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsWrapperCache, NS_WRAPPERCACHE_IID)

#define NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY                                   \
  if ( aIID.Equals(NS_GET_IID(nsWrapperCache)) ) {                            \
    *aInstancePtr = static_cast<nsWrapperCache*>(this);                       \
    return NS_OK;                                                             \
  }




#define NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER \
  nsContentUtils::TraceWrapper(tmp, aCallback, aClosure);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  nsContentUtils::ReleaseWrapper(s, tmp);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class) \
  NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(_class)              \
    NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER        \
  NS_IMPL_CYCLE_COLLECTION_TRACE_END

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(_class) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)         \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER   \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                   \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)       \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS    \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                 \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(_class, _field) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field)            \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field)          \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(_class, _field1,\
                                                _field2)        \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field1)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field2)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field1)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field2)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_3(_class, _field1,\
                                                _field2,        \
                                                _field3)        \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field1)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field2)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field3)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field1)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field2)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field3)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#endif 
