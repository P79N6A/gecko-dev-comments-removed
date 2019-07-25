








































#ifndef __nsconsoleservice_h__
#define __nsconsoleservice_h__

#include "nsCOMPtr.h"
#include "nsHashtable.h"
#include "nsAutoLock.h"

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

    
    PRBool mFull;

    
    nsSupportsHashtable mListeners;

    
    
    PRBool mListening;

    
    PRLock *mLock;
};

#endif 
