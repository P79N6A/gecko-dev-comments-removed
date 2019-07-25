








#ifndef __nsconsoleservice_h__
#define __nsconsoleservice_h__

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

#include "nsCOMPtr.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"

#include "nsIConsoleService.h"

class nsConsoleService MOZ_FINAL : public nsIConsoleService
{
public:
    nsConsoleService();
    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLESERVICE

    void SetIsDelivering() {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(!mDeliveringMessage);
        mDeliveringMessage = true;
    }

    void SetDoneDelivering() {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mDeliveringMessage);
        mDeliveringMessage = false;
    }

private:
    ~nsConsoleService();

    
    nsIConsoleMessage **mMessages;

    
    PRUint32 mBufferSize;

    
    PRUint32 mCurrent;

    
    bool mFull;

    
    
    
    bool mDeliveringMessage;

    
    nsInterfaceHashtable<nsISupportsHashKey, nsIConsoleListener> mListeners;

    
    mozilla::Mutex mLock;
};

#endif 
