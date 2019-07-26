





#ifndef mozilla_dom_ExternalHelperAppChild_h
#define mozilla_dom_ExternalHelperAppChild_h

#include "mozilla/dom/PExternalHelperAppChild.h"
#include "nsExternalHelperAppService.h"
#include "nsIStreamListener.h"

class nsIDivertableChannel;

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

    
    
    void SetHandler(nsExternalAppHandler *handler) { mHandler = handler; }

    virtual bool RecvCancel(const nsresult& aStatus) MOZ_OVERRIDE;
private:
    virtual ~ExternalHelperAppChild();
    nsresult DivertToParent(nsIDivertableChannel *divertable, nsIRequest *request);

    nsRefPtr<nsExternalAppHandler> mHandler;
    nsresult mStatus;
};

} 
} 

#endif 
