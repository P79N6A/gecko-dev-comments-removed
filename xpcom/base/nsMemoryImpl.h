





#ifndef nsMemoryImpl_h__
#define nsMemoryImpl_h__

#include "mozilla/Atomics.h"

#include "nsIMemory.h"
#include "nsIRunnable.h"





class nsMemoryImpl : public nsIMemory
{
public:
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aResult);
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void)
  {
    return 1;
  }
  NS_IMETHOD_(MozExternalRefCountType) Release(void)
  {
    return 1;
  }

  NS_DECL_NSIMEMORY

  static nsresult Create(nsISupports* aOuter,
                         const nsIID& aIID, void** aResult);

  nsresult FlushMemory(const char16_t* aReason, bool aImmediate);
  nsresult RunFlushers(const char16_t* aReason);

protected:
  struct FlushEvent : public nsIRunnable
  {
    MOZ_CONSTEXPR FlushEvent() : mReason(nullptr) {}
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIRUNNABLE
    const char16_t* mReason;
  };

  static mozilla::Atomic<bool> sIsFlushing;
  static FlushEvent sFlushEvent;
  static PRIntervalTime sLastFlushTime;
};

#endif 
