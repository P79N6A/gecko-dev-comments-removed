








































#ifndef __nsconsoleservice_h__
#define __nsconsoleservice_h__

#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsHashtable.h"

#include "nsIConsoleService.h"

class nsConsoleService : public nsIConsoleService
{
public:
    nsConsoleService();
    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLESERVICE

private:
    ~nsConsoleService();

    
    nsresult GetProxyForListener(nsIConsoleListener* aListener,
                                 nsIConsoleListener** aProxy);

    
    nsIConsoleMessage **mMessages;

    
    PRUint32 mBufferSize;

    
    PRUint32 mCurrent;

    
    bool mFull;

    
    nsSupportsHashtable mListeners;

    
    
    bool mListening;

    
    mozilla::Mutex mLock;
};

#endif 
