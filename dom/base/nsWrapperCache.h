




#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"
#include "mozilla/Assertions.h"

class JSObject;
struct JSContext;
class XPCWrappedNativeScope;

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
    MOZ_ASSERT(!PreservingWrapper(),
               "Destroying cache with a preserved wrapper!");
  }

  





  JSObject* GetWrapper() const;

  









  JSObject* GetWrapperPreserveColor() const
  {
    return GetJSObjectFromBits();
  }

  void SetWrapper(JSObject* aWrapper)
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");
    MOZ_ASSERT(aWrapper, "Use ClearWrapper!");

    SetWrapperBits(aWrapper);
  }

  



  void ClearWrapper()
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");

    SetWrapperBits(NULL);
  }

  bool PreservingWrapper()
  {
    return (mWrapperPtrBits & WRAPPER_BIT_PRESERVED) != 0;
  }

  void SetIsDOMBinding()
  {
    MOZ_ASSERT(!mWrapperPtrBits,
               "This flag should be set before creating any wrappers.");
    mWrapperPtrBits = WRAPPER_IS_DOM_BINDING;
  }

  bool IsDOMBinding() const
  {
    return (mWrapperPtrBits & WRAPPER_IS_DOM_BINDING) != 0;
  }

  void SetHasSystemOnlyWrapper()
  {
    MOZ_ASSERT(GetWrapperPreserveColor(),
               "This flag should be set after wrapper creation.");
    MOZ_ASSERT(IsDOMBinding(),
               "This flag should only be set for DOM bindings.");
    mWrapperPtrBits |= WRAPPER_HAS_SOW;
  }

  bool HasSystemOnlyWrapper() const
  {
    return (mWrapperPtrBits & WRAPPER_HAS_SOW) != 0;
  }

  



  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope)
  {
    MOZ_ASSERT(!IsDOMBinding(), "Someone forgot to override WrapObject");
    return nullptr;
  }

  


  bool IsBlack();

  



  bool IsBlackAndDoesNotNeedTracing(nsISupports* aThis);

  
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
    mWrapperPtrBits = reinterpret_cast<uintptr_t>(aWrapper) |
                      (mWrapperPtrBits & WRAPPER_IS_DOM_BINDING);
  }

  










  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };

  



  enum { WRAPPER_IS_DOM_BINDING = 1 << 1 };

  




  enum { WRAPPER_HAS_SOW = 1 << 2 };

  enum { kWrapperBitMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_DOM_BINDING |
                            WRAPPER_HAS_SOW) };

  uintptr_t mWrapperPtrBits;
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
  nsContentUtils::ReleaseWrapper(p, tmp);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class) \
  NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(_class)              \
    NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER        \
  NS_IMPL_CYCLE_COLLECTION_TRACE_END

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(_class) \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)         \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER   \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                   \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)       \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS    \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                 \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(_class, _field) \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field)                     \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field)                   \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(_class, _field1,\
                                                _field2)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_3(_class, _field1,\
                                                _field2,        \
                                                _field3)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_5(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4,        \
                                                _field5)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_6(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4,        \
                                                _field5,        \
                                                _field6)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field6)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field6)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_7(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4,        \
                                                _field5,        \
                                                _field6,        \
                                                _field7)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field6)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field7)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field6)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field7)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_8(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4,        \
                                                _field5,        \
                                                _field6,        \
                                                _field7,        \
                                                _field8)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field6)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field7)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field8)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field6)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field7)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field8)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_9(_class, _field1,\
                                                _field2,        \
                                                _field3,        \
                                                _field4,        \
                                                _field5,        \
                                                _field6,        \
                                                _field7,        \
                                                _field8,        \
                                                _field9)        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field6)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field7)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field8)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field9)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field6)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field7)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field8)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field9)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_10(_class, _field1,\
                                                 _field2,       \
                                                 _field3,       \
                                                 _field4,       \
                                                 _field5,       \
                                                 _field6,       \
                                                 _field7,       \
                                                 _field8,       \
                                                 _field9,       \
                                                 _field10)      \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                 \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field1)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field2)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field3)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field4)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field5)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field6)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field7)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field8)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field9)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field10)                   \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)               \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field1)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field2)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field3)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field4)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field5)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field6)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field7)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field8)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field9)                  \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field10)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#endif 
