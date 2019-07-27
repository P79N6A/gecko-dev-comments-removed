




#ifndef nsCSPContext_h___
#define nsCSPContext_h___

#include "nsCSPUtils.h"
#include "nsDataHashtable.h"
#include "nsIChannel.h"
#include "nsIChannelEventSink.h"
#include "nsIClassInfo.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIInterfaceRequestor.h"
#include "nsISerializable.h"
#include "nsIStreamListener.h"
#include "nsWeakPtr.h"
#include "nsXPCOM.h"

#define NS_CSPCONTEXT_CONTRACTID "@mozilla.org/cspcontext;1"
 
#define NS_CSPCONTEXT_CID \
{ 0x09d9ed1a, 0xe5d4, 0x4004, \
  { 0xbf, 0xe0, 0x27, 0xce, 0xb9, 0x23, 0xd9, 0xac } }

class nsCSPContext : public nsIContentSecurityPolicy
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTSECURITYPOLICY
    NS_DECL_NSISERIALIZABLE

  protected:
    virtual ~nsCSPContext();

  public:
    nsCSPContext();

    nsresult SendReports(nsISupports* aBlockedContentSource,
                         nsIURI* aOriginalURI,
                         nsAString& aViolatedDirective,
                         uint32_t aViolatedPolicyIndex,
                         nsAString& aSourceFile,
                         nsAString& aScriptSample,
                         uint32_t aLineNum);

    nsresult AsyncReportViolation(nsISupports* aBlockedContentSource,
                                  nsIURI* aOriginalURI,
                                  const nsAString& aViolatedDirective,
                                  uint32_t aViolatedPolicyIndex,
                                  const nsAString& aObserverSubject,
                                  const nsAString& aSourceFile,
                                  const nsAString& aScriptSample,
                                  uint32_t aLineNum);

  private:
    NS_IMETHODIMP getAllowsInternal(nsContentPolicyType aContentType,
                                    enum CSPKeyword aKeyword,
                                    const nsAString& aNonceOrContent,
                                    bool* outShouldReportViolations,
                                    bool* outIsAllowed) const;

    nsCOMPtr<nsIURI>                           mReferrer;
    uint64_t                                   mInnerWindowID; 
    nsTArray<nsCSPPolicy*>                     mPolicies;
    nsCOMPtr<nsIURI>                           mSelfURI;
    nsDataHashtable<nsCStringHashKey, int16_t> mShouldLoadCache;
    nsCOMPtr<nsILoadGroup>                     mCallingChannelLoadGroup;
    nsWeakPtr                                  mLoadingContext;
};


class CSPViolationReportListener : public nsIStreamListener
{
  public:
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_ISUPPORTS

  public:
    CSPViolationReportListener();

  protected:
    virtual ~CSPViolationReportListener();
};




class CSPReportRedirectSink MOZ_FINAL : public nsIChannelEventSink,
                                        public nsIInterfaceRequestor
{
  public:
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_ISUPPORTS

  public:
    CSPReportRedirectSink();

  protected:
    virtual ~CSPReportRedirectSink();
};

#endif 
