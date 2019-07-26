




#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"
#include "mozilla/Assertions.h"
#include "js/Id.h"          
#include "js/Value.h"       
#include "js/RootingAPI.h"
#include "js/TracingAPI.h"

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

  



  virtual JSObject* WrapObject(JSContext* cx)
  {
    MOZ_ASSERT(!IsDOMBinding(), "Someone forgot to override WrapObject");
    return nullptr;
  }

  


  bool IsBlack();

  



  bool IsBlackAndDoesNotNeedTracing(nsISupports* aThis);

  bool HasNothingToTrace(nsISupports* aThis);

  
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

protected:
  void TraceWrapper(JSTracer* aTrc, const char* name)
  {
    if (mWrapper) {
      JS_CallHeapObjectTracer(aTrc, &mWrapper, name);
    }
  }

  void PoisonWrapper()
  {
    if (mWrapper) {
      mWrapper.setToCrashOnTouch();
    }
  }

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

  enum { kWrapperFlagsMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_DOM_BINDING) };

  JS::Heap<JSObject*> mWrapper;
  uint32_t            mFlags;
};

enum { WRAPPER_CACHE_FLAGS_BITS_USED = 2 };

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

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                   \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)            \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(__VA_ARGS__)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER      \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                      \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)          \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(__VA_ARGS__)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS       \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                    \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)


#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_3(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_5(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_6(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_7(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_8(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_9(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_10(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_11(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_12(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_13(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)

#endif 
