





































#include "mozilla/dom/PExternalHelperAppParent.h"
#include "nsIChannel.h"
#include "nsICancelable.h"
#include "nsIResumableChannel.h"
#include "nsHashPropertyBag.h"

namespace IPC {
class URI;
}

namespace mozilla {
namespace dom {

class TabParent;

class ExternalHelperAppParent : public PExternalHelperAppParent
                              , public nsHashPropertyBag
                              , public nsIChannel
                              , public nsIResumableChannel
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIRESUMABLECHANNEL

    bool RecvOnStartRequest(const nsCString& entityID);
    bool RecvOnDataAvailable(const nsCString& data, const PRUint32& offset, const PRUint32& count);
    bool RecvOnStopRequest(const nsresult& code);
    
    ExternalHelperAppParent(const IPC::URI& uri, const PRInt64& contentLength);
    void Init(TabParent *parent,
              const nsCString& aMimeContentType,
              const nsCString& aContentDisposition,
              const PRBool& aForceSave);
    virtual ~ExternalHelperAppParent();

private:
  nsCOMPtr<nsIStreamListener> mListener;
  nsCOMPtr<nsIURI> mURI;
  PRBool mPending;
  nsLoadFlags mLoadFlags;
  nsresult mStatus;
  PRInt64 mContentLength;
  nsCString mContentDisposition;
  nsCString mEntityID;
};

} 
} 
