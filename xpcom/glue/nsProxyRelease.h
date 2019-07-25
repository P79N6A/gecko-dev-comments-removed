




#ifndef nsProxyRelease_h__
#define nsProxyRelease_h__

#include "nsIEventTarget.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

#ifdef XPCOM_GLUE_AVOID_NSPR
#error NS_ProxyRelease implementation depends on NSPR.
#endif






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsCOMPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nsnull;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsRefPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nsnull;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}














NS_COM_GLUE nsresult
NS_ProxyRelease
    (nsIEventTarget *target, nsISupports *doomed, bool alwaysProxy=false);







































template<class T>
class nsMainThreadPtrHolder
{
public:
  
  nsMainThreadPtrHolder(T* ptr) : mRawPtr(NULL) {
    
    
    MOZ_ASSERT(NS_IsMainThread());
    NS_IF_ADDREF(mRawPtr = ptr);
  }

  
  ~nsMainThreadPtrHolder() {
    if (NS_IsMainThread()) {
      NS_IF_RELEASE(mRawPtr);
    } else {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      if (!mainThread) {
        NS_WARNING("Couldn't get main thread! Leaking pointer.");
        return;
      }
      NS_ProxyRelease(mainThread, mRawPtr);
    }
  }

  T* get() {
    
    if (NS_UNLIKELY(!NS_IsMainThread()))
      MOZ_CRASH();
    return mRawPtr;
  }

  nsrefcnt Release();
  nsrefcnt AddRef();

private:
  
  nsAutoRefCnt mRefCnt;

  
  T* mRawPtr;

  
  
  T& operator=(nsMainThreadPtrHolder& other);
  nsMainThreadPtrHolder(const nsMainThreadPtrHolder& other);
};

template<class T>
NS_IMPL_THREADSAFE_ADDREF(nsMainThreadPtrHolder<T>)
template<class T>
NS_IMPL_THREADSAFE_RELEASE(nsMainThreadPtrHolder<T>)

template<class T>
class nsMainThreadPtrHandle
{
  nsRefPtr<nsMainThreadPtrHolder<T> > mPtr;

  public:
  nsMainThreadPtrHandle(nsMainThreadPtrHolder<T> *aHolder) : mPtr(aHolder) {}
  nsMainThreadPtrHandle(const nsMainThreadPtrHandle& aOther) : mPtr(aOther.mPtr) {}
  nsMainThreadPtrHandle& operator=(const nsMainThreadPtrHandle& aOther) {
    mPtr = aOther.mPtr;
  }

  operator nsMainThreadPtrHolder<T>*() { return mPtr.get(); }

  
  
  
  T* get() { return mPtr.get()->get(); }
  operator T*() { return get(); }
  T* operator->() { return get(); }
};

#endif
