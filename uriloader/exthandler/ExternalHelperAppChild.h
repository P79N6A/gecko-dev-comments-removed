






































#include "mozilla/dom/PExternalHelperAppChild.h"
#include "nsIStreamListener.h"

namespace mozilla {
namespace dom {

class ExternalHelperAppChild : public PExternalHelperAppChild
                             , public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    ExternalHelperAppChild();
    virtual ~ExternalHelperAppChild();

    
    
    void SetHandler(nsIStreamListener *handler) { mHandler = handler; }

    virtual bool RecvCancel(const nsresult& aStatus);
private:
    nsCOMPtr<nsIStreamListener> mHandler;
    nsresult mStatus;
};

} 
} 
