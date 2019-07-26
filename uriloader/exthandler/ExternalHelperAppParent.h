





#include "mozilla/dom/PExternalHelperAppParent.h"
#include "nsIChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIResumableChannel.h"
#include "nsHashPropertyBag.h"

namespace IPC {
class URI;
}

namespace mozilla {

namespace ipc {
class OptionalURIParams;
} 

namespace dom {

class ContentParent;
class PBrowserParent;

class ExternalHelperAppParent : public PExternalHelperAppParent
                              , public nsHashPropertyBag
                              , public nsIChannel
                              , public nsIMultiPartChannel
                              , public nsIResumableChannel
{
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIMULTIPARTCHANNEL
    NS_DECL_NSIRESUMABLECHANNEL

    bool RecvOnStartRequest(const nsCString& entityID) MOZ_OVERRIDE;
    bool RecvOnDataAvailable(const nsCString& data,
                             const uint64_t& offset,
                             const uint32_t& count) MOZ_OVERRIDE;
    bool RecvOnStopRequest(const nsresult& code) MOZ_OVERRIDE;

    ExternalHelperAppParent(const OptionalURIParams& uri, const int64_t& contentLength);
    void Init(ContentParent *parent,
              const nsCString& aMimeContentType,
              const nsCString& aContentDisposition,
              const uint32_t& aContentDispositionHint,
              const nsString& aContentDispositionFilename,
              const bool& aForceSave,
              const OptionalURIParams& aReferrer,
              PBrowserParent* aBrowser);
    virtual ~ExternalHelperAppParent();

private:
  nsCOMPtr<nsIStreamListener> mListener;
  nsCOMPtr<nsIURI> mURI;
  bool mPending;
  nsLoadFlags mLoadFlags;
  nsresult mStatus;
  int64_t mContentLength;
  uint32_t mContentDisposition;
  nsString mContentDispositionFilename;
  nsCString mContentDispositionHeader;
  nsCString mEntityID;
};

} 
} 
