





#include "mozilla/dom/PExternalHelperAppParent.h"
#include "nsIChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIResumableChannel.h"
#include "nsIStreamListener.h"
#include "nsHashPropertyBag.h"

namespace IPC {
class URI;
} 

namespace mozilla {

namespace ipc {
class OptionalURIParams;
} 

namespace net {
class PChannelDiverterParent;
} 

namespace dom {

class ContentParent;
class PBrowserParent;

class ExternalHelperAppParent : public PExternalHelperAppParent
                              , public nsHashPropertyBag
                              , public nsIChannel
                              , public nsIMultiPartChannel
                              , public nsIResumableChannel
                              , public nsIStreamListener
{
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;

public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIMULTIPARTCHANNEL
    NS_DECL_NSIRESUMABLECHANNEL
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    bool RecvOnStartRequest(const nsCString& entityID) override;
    bool RecvOnDataAvailable(const nsCString& data,
                             const uint64_t& offset,
                             const uint32_t& count) override;
    bool RecvOnStopRequest(const nsresult& code) override;

    bool RecvDivertToParentUsing(PChannelDiverterParent* diverter) override;

    ExternalHelperAppParent(const OptionalURIParams& uri, const int64_t& contentLength);
    void Init(ContentParent *parent,
              const nsCString& aMimeContentType,
              const nsCString& aContentDisposition,
              const uint32_t& aContentDispositionHint,
              const nsString& aContentDispositionFilename,
              const bool& aForceSave,
              const OptionalURIParams& aReferrer,
              PBrowserParent* aBrowser);

protected:
  virtual ~ExternalHelperAppParent();

  virtual void ActorDestroy(ActorDestroyReason why) override;
  void Delete();

private:
  nsCOMPtr<nsIStreamListener> mListener;
  nsCOMPtr<nsIURI> mURI;
  bool mPending;
  DebugOnly<bool> mDiverted;
  bool mIPCClosed;
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
