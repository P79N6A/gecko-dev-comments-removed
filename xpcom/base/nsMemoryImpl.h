




#ifndef nsMemoryImpl_h__
#define nsMemoryImpl_h__

#include "mozilla/Atomics.h"

#include "nsIMemory.h"
#include "nsIRunnable.h"





class nsMemoryImpl : public nsIMemory
{
public:
    
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aResult);
    NS_IMETHOD_(nsrefcnt) AddRef(void) { return 1; }
    NS_IMETHOD_(nsrefcnt) Release(void) { return 1; }

    NS_DECL_NSIMEMORY

    static nsresult Create(nsISupports* outer,
                           const nsIID& aIID, void **aResult);

    NS_HIDDEN_(nsresult) FlushMemory(const char16_t* aReason, bool aImmediate);
    NS_HIDDEN_(nsresult) RunFlushers(const char16_t* aReason);

protected:
    struct FlushEvent : public nsIRunnable {
        NS_DECL_ISUPPORTS_INHERITED
        NS_DECL_NSIRUNNABLE
        const char16_t* mReason;
    };

    static mozilla::Atomic<int32_t> sIsFlushing;
    static FlushEvent sFlushEvent;
    static PRIntervalTime sLastFlushTime;
};

#endif 
