




































#ifndef nsMemoryImpl_h__
#define nsMemoryImpl_h__

#include "nsIMemory.h"
#include "nsIRunnable.h"
#include "prtime.h"





class nsMemoryImpl : public nsIMemory
{
public:
    
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aResult);
    NS_IMETHOD_(nsrefcnt) AddRef(void) { return 1; }
    NS_IMETHOD_(nsrefcnt) Release(void) { return 1; }

    NS_DECL_NSIMEMORY

    static NS_METHOD Create(nsISupports* outer,
                            const nsIID& aIID, void **aResult);

    NS_HIDDEN_(nsresult) FlushMemory(const PRUnichar* aReason, PRBool aImmediate);
    NS_HIDDEN_(nsresult) RunFlushers(const PRUnichar* aReason);

protected:
    struct FlushEvent : public nsIRunnable {
        NS_DECL_ISUPPORTS_INHERITED
        NS_DECL_NSIRUNNABLE
        const PRUnichar* mReason;
    };

    static PRInt32    sIsFlushing;
    static FlushEvent sFlushEvent;
    static PRIntervalTime sLastFlushTime;
};

#endif 
