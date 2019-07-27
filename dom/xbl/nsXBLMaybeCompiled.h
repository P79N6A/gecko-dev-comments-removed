




#ifndef nsXBLMaybeCompiled_h__
#define nsXBLMaybeCompiled_h__

#include "js/GCAPI.h"













template <class UncompiledT>
class nsXBLMaybeCompiled
{
public:
  nsXBLMaybeCompiled() : mUncompiled(BIT_UNCOMPILED) {}

  explicit nsXBLMaybeCompiled(UncompiledT* uncompiled)
    : mUncompiled(reinterpret_cast<uintptr_t>(uncompiled) | BIT_UNCOMPILED) {}

  explicit nsXBLMaybeCompiled(JSObject* compiled) : mCompiled(compiled) {}

  bool IsCompiled() const
  {
    return !(mUncompiled & BIT_UNCOMPILED);
  }

  UncompiledT* GetUncompiled() const
  {
    MOZ_ASSERT(!IsCompiled(), "Attempt to get compiled function as uncompiled");
    uintptr_t unmasked = mUncompiled & ~BIT_UNCOMPILED;
    return reinterpret_cast<UncompiledT*>(unmasked);
  }

  JSObject* GetJSFunction() const
  {
    MOZ_ASSERT(IsCompiled(), "Attempt to get uncompiled function as compiled");
    if (mCompiled) {
      JS::ExposeObjectToActiveJS(mCompiled);
    }
    return mCompiled;
  }

  
  JSObject* GetJSFunctionPreserveColor() const
  {
    MOZ_ASSERT(IsCompiled(), "Attempt to get uncompiled function as compiled");
    return mCompiled;
  }

private:
  JSObject*& UnsafeGetJSFunction()
  {
    MOZ_ASSERT(IsCompiled(), "Attempt to get uncompiled function as compiled");
    return mCompiled;
  }

  enum { BIT_UNCOMPILED = 1 << 0 };

  union
  {
    
    
    uintptr_t mUncompiled;

    
    JSObject* mCompiled;
  };

  friend struct js::GCMethods<nsXBLMaybeCompiled<UncompiledT>>;
};


namespace js {

template <class UncompiledT>
struct GCMethods<nsXBLMaybeCompiled<UncompiledT> >
{
  typedef struct GCMethods<JSObject *> Base;

  static nsXBLMaybeCompiled<UncompiledT> initial() { return nsXBLMaybeCompiled<UncompiledT>(); }

  static bool poisoned(nsXBLMaybeCompiled<UncompiledT> function)
  {
    return function.IsCompiled() && Base::poisoned(function.GetJSFunction());
  }

  static bool needsPostBarrier(nsXBLMaybeCompiled<UncompiledT> function)
  {
    return function.IsCompiled() && Base::needsPostBarrier(function.GetJSFunction());
  }

#ifdef JSGC_GENERATIONAL
  static void postBarrier(nsXBLMaybeCompiled<UncompiledT>* functionp)
  {
    Base::postBarrier(&functionp->UnsafeGetJSFunction());
  }

  static void relocate(nsXBLMaybeCompiled<UncompiledT>* functionp)
  {
    Base::relocate(&functionp->UnsafeGetJSFunction());
  }
#endif
};

template <class UncompiledT>
class HeapBase<nsXBLMaybeCompiled<UncompiledT> >
{
  const JS::Heap<nsXBLMaybeCompiled<UncompiledT> >& wrapper() const {
    return *static_cast<const JS::Heap<nsXBLMaybeCompiled<UncompiledT> >*>(this);
  }

  JS::Heap<nsXBLMaybeCompiled<UncompiledT> >& wrapper() {
    return *static_cast<JS::Heap<nsXBLMaybeCompiled<UncompiledT> >*>(this);
  }

  const nsXBLMaybeCompiled<UncompiledT>* extract() const {
    return wrapper().address();
  }

  nsXBLMaybeCompiled<UncompiledT>* extract() {
    return wrapper().unsafeGet();
  }

public:
  bool IsCompiled() const { return extract()->IsCompiled(); }
  UncompiledT* GetUncompiled() const { return extract()->GetUncompiled(); }
  JSObject* GetJSFunction() const { return extract()->GetJSFunction(); }
  JSObject* GetJSFunctionPreserveColor() const { return extract()->GetJSFunctionPreserveColor(); }

  void SetUncompiled(UncompiledT* source) {
    wrapper().set(nsXBLMaybeCompiled<UncompiledT>(source));
  }

  void SetJSFunction(JSObject* function) {
    wrapper().set(nsXBLMaybeCompiled<UncompiledT>(function));
  }

  JS::Heap<JSObject*>& AsHeapObject()
  {
    MOZ_ASSERT(extract()->IsCompiled());
    return *reinterpret_cast<JS::Heap<JSObject*>*>(this);
  }
};

} 

#endif 
