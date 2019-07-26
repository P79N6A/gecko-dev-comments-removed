




#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"
#include "mozilla/Assertions.h"
#include "js/Id.h"          
#include "js/Value.h"       
#include "js/RootingAPI.h"

struct JSTracer;
class XPCWrappedNativeScope;

#define NS_WRAPPERCACHE_IID \
{ 0x6f3179a1, 0x36f7, 0x4a5c, \
  { 0x8c, 0xf1, 0xad, 0xc8, 0x7c, 0xde, 0x3e, 0x87 } }






























class nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_WRAPPERCACHE_IID)

  nsWrapperCache() : mWrapper(nullptr), mFlags(0)
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
    return GetWrapperJSObject();
  }

  void SetWrapper(JSObject* aWrapper)
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");
    MOZ_ASSERT(aWrapper, "Use ClearWrapper!");

    SetWrapperJSObject(aWrapper);
  }

  



  void ClearWrapper()
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");

    SetWrapperJSObject(nullptr);
  }

  bool PreservingWrapper()
  {
    return HasWrapperFlag(WRAPPER_BIT_PRESERVED);
  }

  void SetIsDOMBinding()
  {
    MOZ_ASSERT(!mWrapper && !(GetWrapperFlags() & ~WRAPPER_IS_DOM_BINDING),
               "This flag should be set before creating any wrappers.");
    SetWrapperFlags(WRAPPER_IS_DOM_BINDING);
  }

  bool IsDOMBinding() const
  {
    return HasWrapperFlag(WRAPPER_IS_DOM_BINDING);
  }

  void SetHasSystemOnlyWrapper()
  {
    MOZ_ASSERT(GetWrapperPreserveColor(),
               "This flag should be set after wrapper creation.");
    MOZ_ASSERT(IsDOMBinding(),
               "This flag should only be set for DOM bindings.");
    SetWrapperFlags(WRAPPER_HAS_SOW);
  }

  bool HasSystemOnlyWrapper() const
  {
    return HasWrapperFlag(WRAPPER_HAS_SOW);
  }

  



  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
  {
    MOZ_ASSERT(!IsDOMBinding(), "Someone forgot to override WrapObject");
    return nullptr;
  }

  


  bool IsBlack();

  



  bool IsBlackAndDoesNotNeedTracing(nsISupports* aThis);

  
  void SetPreservingWrapper(bool aPreserve)
  {
    if(aPreserve) {
      SetWrapperFlags(WRAPPER_BIT_PRESERVED);
    }
    else {
      UnsetWrapperFlags(WRAPPER_BIT_PRESERVED);
    }
  }

  void TraceWrapper(const TraceCallbacks& aCallbacks, void* aClosure)
  {
    if (PreservingWrapper() && mWrapper) {
      aCallbacks.Trace(&mWrapper, "Preserved wrapper", aClosure);
    }
  }

  




  uint32_t GetFlags() const
  {
    return mFlags & ~kWrapperFlagsMask;
  }

  bool HasFlag(uint32_t aFlag) const
  {
    MOZ_ASSERT((aFlag & kWrapperFlagsMask) == 0, "Bad flag mask");
    return !!(mFlags & aFlag);
  }

  void SetFlags(uint32_t aFlagsToSet)
  {
    MOZ_ASSERT((aFlagsToSet & kWrapperFlagsMask) == 0, "Bad flag mask");
    mFlags |= aFlagsToSet;
  }

  void UnsetFlags(uint32_t aFlagsToUnset)
  {
    MOZ_ASSERT((aFlagsToUnset & kWrapperFlagsMask) == 0, "Bad flag mask");
    mFlags &= ~aFlagsToUnset;
  }

  void PreserveWrapper(nsISupports* aScriptObjectHolder)
  {
    if (PreservingWrapper()) {
      return;
    }

    nsISupports* ccISupports;
    aScriptObjectHolder->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                                        reinterpret_cast<void**>(&ccISupports));
    MOZ_ASSERT(ccISupports);

    nsXPCOMCycleCollectionParticipant* participant;
    CallQueryInterface(ccISupports, &participant);
    PreserveWrapper(ccISupports, participant);
  }

  void PreserveWrapper(void* aScriptObjectHolder, nsScriptObjectTracer* aTracer)
  {
    if (PreservingWrapper()) {
      return;
    }

    HoldJSObjects(aScriptObjectHolder, aTracer);
    SetPreservingWrapper(true);
#ifdef DEBUG
    
    CheckCCWrapperTraversal(aScriptObjectHolder, aTracer);
#endif
  }

  void ReleaseWrapper(void* aScriptObjectHolder);

private:
  JSObject *GetWrapperJSObject() const
  {
    return mWrapper;
  }

  void SetWrapperJSObject(JSObject* aWrapper)
  {
    mWrapper = aWrapper;
    UnsetWrapperFlags(kWrapperFlagsMask & ~WRAPPER_IS_DOM_BINDING);
  }

  void TraceWrapperJSObject(JSTracer* aTrc, const char* aName);

  uint32_t GetWrapperFlags() const
  {
    return mFlags & kWrapperFlagsMask;
  }

  bool HasWrapperFlag(uint32_t aFlag) const
  {
    MOZ_ASSERT((aFlag & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    return !!(mFlags & aFlag);
  }

  void SetWrapperFlags(uint32_t aFlagsToSet)
  {
    MOZ_ASSERT((aFlagsToSet & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    mFlags |= aFlagsToSet;
  }

  void UnsetWrapperFlags(uint32_t aFlagsToUnset)
  {
    MOZ_ASSERT((aFlagsToUnset & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    mFlags &= ~aFlagsToUnset;
  }

  static void HoldJSObjects(void* aScriptObjectHolder,
                            nsScriptObjectTracer* aTracer);

#ifdef DEBUG
  void CheckCCWrapperTraversal(void* aScriptObjectHolder,
                               nsScriptObjectTracer* aTracer);
#endif 

  










  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };

  



  enum { WRAPPER_IS_DOM_BINDING = 1 << 1 };

  




  enum { WRAPPER_HAS_SOW = 1 << 2 };

  enum { kWrapperFlagsMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_DOM_BINDING |
                              WRAPPER_HAS_SOW) };

  JS::Heap<JSObject*> mWrapper;
  uint32_t            mFlags;
};

enum { WRAPPER_CACHE_FLAGS_BITS_USED = 3 };

NS_DEFINE_STATIC_IID_ACCESSOR(nsWrapperCache, NS_WRAPPERCACHE_IID)

#define NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY                                   \
  if ( aIID.Equals(NS_GET_IID(nsWrapperCache)) ) {                            \
    *aInstancePtr = static_cast<nsWrapperCache*>(this);                       \
    return NS_OK;                                                             \
  }




#define NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER \
  tmp->TraceWrapper(aCallbacks, aClosure);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  tmp->ReleaseWrapper(p);

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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_11(_class,        \
                                                 _field1,       \
                                                 _field2,       \
                                                 _field3,       \
                                                 _field4,       \
                                                 _field5,       \
                                                 _field6,       \
                                                 _field7,       \
                                                 _field8,       \
                                                 _field9,       \
                                                 _field10,      \
                                                 _field11)      \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field11)                   \
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
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field11)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_12(_class,        \
                                                 _field1,       \
                                                 _field2,       \
                                                 _field3,       \
                                                 _field4,       \
                                                 _field5,       \
                                                 _field6,       \
                                                 _field7,       \
                                                 _field8,       \
                                                 _field9,       \
                                                 _field10,      \
                                                 _field11,      \
                                                 _field12)      \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field11)                   \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field12)                   \
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
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field11)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field12)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_13(_class,        \
                                                 _field1,       \
                                                 _field2,       \
                                                 _field3,       \
                                                 _field4,       \
                                                 _field5,       \
                                                 _field6,       \
                                                 _field7,       \
                                                 _field8,       \
                                                 _field9,       \
                                                 _field10,      \
                                                 _field11,      \
                                                 _field12,      \
                                                 _field13)      \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                        \
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
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field11)                   \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field12)                   \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(_field13)                   \
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
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field11)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field12)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(_field13)                 \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS            \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                         \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#endif 
