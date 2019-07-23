




































#ifndef nsSimpleChromeHandler_h___
#define nsSimpleChromeHandler_h___

#include "nsIProtocolHandler.h"
#include "nsIIOService.h"
#include "nsWeakReference.h"

class nsSimpleChromeHandler : public nsIProtocolHandler, public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    nsSimpleChromeHandler();
    virtual ~nsSimpleChromeHandler();

    nsresult Init();
private:
    nsCOMPtr<nsIIOService> mIOService;
    nsCOMPtr<nsIFile> mChromeDir;
};

#endif 
