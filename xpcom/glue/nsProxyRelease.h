




#ifndef nsProxyRelease_h__
#define nsProxyRelease_h__

#include "nsIEventTarget.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "MainThreadUtils.h"
#include "mozilla/Likely.h"

#ifdef XPCOM_GLUE_AVOID_NSPR
#error NS_ProxyRelease implementation depends on NSPR.
#endif






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsCOMPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nullptr;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsRefPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nullptr;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}














NS_COM_GLUE nsresult
NS_ProxyRelease
    (nsIEventTarget *target, nsISupports *doomed, bool alwaysProxy=false);







































template<class T>
class nsMainThreadPtrHolder MOZ_FINAL
{
public:
  
  
  
  
  
  nsMainThreadPtrHolder(T* ptr, bool strict = true) : mRawPtr(NULL), mStrict(strict) {
    
    
    MOZ_ASSERT(!mStrict || NS_IsMainThread());
    NS_IF_ADDREF(mRawPtr = ptr);
  }

  
  ~nsMainThreadPtrHolder() {
    if (NS_IsMainThread()) {
      NS_IF_RELEASE(mRawPtr);
    } else if (mRawPtr) {
      nsCOMPtr<nsIThread> mainThread;
      NS_GetMainThread(getter_AddRefs(mainThread));
      if (!mainThread) {
        NS_WARNING("Couldn't get main thread! Leaking pointer.");
        return;
      }
      NS_ProxyRelease(mainThread, mRawPtr);
    }
  }

  T* get() {
    
    if (mStrict && MOZ_UNLIKELY(!NS_IsMainThread())) {
      NS_ERROR("Can't dereference nsMainThreadPtrHolder off main thread");
      MOZ_CRASH();
    }
    return mRawPtr;
  }

  bool operator==(const nsMainThreadPtrHolder<T>& aOther) const { return mRawPtr == aOther.mRawPtr; }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsMainThreadPtrHolder<T>)

private:
  
  T* mRawPtr;

  
  bool mStrict;

  
  
  T& operator=(nsMainThreadPtrHolder& other);
  nsMainThreadPtrHolder(const nsMainThreadPtrHolder& other);
};

template<class T>
class nsMainThreadPtrHandle
{
  nsRefPtr<nsMainThreadPtrHolder<T> > mPtr;

  public:
  nsMainThreadPtrHandle() : mPtr(NULL) {}
  nsMainThreadPtrHandle(nsMainThreadPtrHolder<T> *aHolder) : mPtr(aHolder) {}
  nsMainThreadPtrHandle(const nsMainThreadPtrHandle& aOther) : mPtr(aOther.mPtr) {}
  nsMainThreadPtrHandle& operator=(const nsMainThreadPtrHandle& aOther) {
    mPtr = aOther.mPtr;
    return *this;
  }

  
  
  
  T* get()
  {
    if (mPtr) {
      return mPtr.get()->get();
    }
    return nullptr;
  }
  const T* get() const
  {
    if (mPtr) {
      return mPtr.get()->get();
    }
    return nullptr;
  }

  operator T*() { return get(); }
  T* operator->() { return get(); }

  
  bool operator==(const nsMainThreadPtrHandle<T>& aOther) const {
    if (!mPtr || !aOther.mPtr)
      return mPtr == aOther.mPtr;
    return *mPtr == *aOther.mPtr;
  }
  bool operator!() { return !mPtr; }
};

#endif
