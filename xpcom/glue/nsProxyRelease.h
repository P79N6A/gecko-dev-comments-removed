





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






template<class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease(nsIEventTarget* aTarget, nsCOMPtr<T>& aDoomed,
                bool aAlwaysProxy = false)
{
  T* raw = nullptr;
  aDoomed.swap(raw);
  return NS_ProxyRelease(aTarget, raw, aAlwaysProxy);
}






template<class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease(nsIEventTarget* aTarget, nsRefPtr<T>& aDoomed,
                bool aAlwaysProxy = false)
{
  T* raw = nullptr;
  aDoomed.swap(raw);
  return NS_ProxyRelease(aTarget, raw, aAlwaysProxy);
}














nsresult
NS_ProxyRelease(nsIEventTarget* aTarget, nsISupports* aDoomed,
                bool aAlwaysProxy = false);







































template<class T>
class nsMainThreadPtrHolder MOZ_FINAL
{
public:
  
  
  
  
  
  explicit nsMainThreadPtrHolder(T* aPtr, bool aStrict = true)
    : mRawPtr(nullptr)
    , mStrict(aStrict)
  {
    
    
    MOZ_ASSERT(!mStrict || NS_IsMainThread());
    NS_IF_ADDREF(mRawPtr = aPtr);
  }

private:
  
  ~nsMainThreadPtrHolder()
  {
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

public:
  T* get()
  {
    
    if (mStrict && MOZ_UNLIKELY(!NS_IsMainThread())) {
      NS_ERROR("Can't dereference nsMainThreadPtrHolder off main thread");
      MOZ_CRASH();
    }
    return mRawPtr;
  }

  bool operator==(const nsMainThreadPtrHolder<T>& aOther) const
  {
    return mRawPtr == aOther.mRawPtr;
  }
  bool operator!() const
  {
    return !mRawPtr;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsMainThreadPtrHolder<T>)

private:
  
  T* mRawPtr;

  
  bool mStrict;

  
  
  T& operator=(nsMainThreadPtrHolder& aOther);
  nsMainThreadPtrHolder(const nsMainThreadPtrHolder& aOther);
};

template<class T>
class nsMainThreadPtrHandle
{
  nsRefPtr<nsMainThreadPtrHolder<T>> mPtr;

public:
  nsMainThreadPtrHandle() : mPtr(nullptr) {}
  explicit nsMainThreadPtrHandle(nsMainThreadPtrHolder<T>* aHolder)
    : mPtr(aHolder)
  {
  }
  nsMainThreadPtrHandle(const nsMainThreadPtrHandle& aOther)
    : mPtr(aOther.mPtr)
  {
  }
  nsMainThreadPtrHandle& operator=(const nsMainThreadPtrHandle& aOther)
  {
    mPtr = aOther.mPtr;
    return *this;
  }
  nsMainThreadPtrHandle& operator=(nsMainThreadPtrHolder<T>* aHolder)
  {
    mPtr = aHolder;
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

  
  bool operator==(const nsMainThreadPtrHandle<T>& aOther) const
  {
    if (!mPtr || !aOther.mPtr) {
      return mPtr == aOther.mPtr;
    }
    return *mPtr == *aOther.mPtr;
  }
  bool operator!() const {
    return !mPtr || !*mPtr;
  }
};

#endif
