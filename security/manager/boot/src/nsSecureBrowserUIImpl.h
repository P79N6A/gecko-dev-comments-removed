








































#ifndef nsSecureBrowserUIImpl_h_
#define nsSecureBrowserUIImpl_h_

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsIObserver.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIStringBundle.h"
#include "nsISecureBrowserUI.h"
#include "nsIDocShell.h"
#include "nsIWebProgressListener.h"
#include "nsIFormSubmitObserver.h"
#include "nsIURI.h"
#include "nsISecurityEventSink.h"
#include "nsWeakReference.h"
#include "nsISSLStatusProvider.h"
#include "pldhash.h"

class nsITransportSecurityInfo;
class nsISecurityWarningDialogs;
class nsIChannel;

#define NS_SECURE_BROWSER_UI_CID \
{ 0xcc75499a, 0x1dd1, 0x11b2, {0x8a, 0x82, 0xca, 0x41, 0x0a, 0xc9, 0x07, 0xb8}}


class nsSecureBrowserUIImpl : public nsISecureBrowserUI,
                              public nsIWebProgressListener,
                              public nsIFormSubmitObserver,
                              public nsIObserver,
                              public nsSupportsWeakReference,
                              public nsISSLStatusProvider
{
public:
  
  nsSecureBrowserUIImpl();
  virtual ~nsSecureBrowserUIImpl();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSISECUREBROWSERUI
  
  
  NS_DECL_NSIOBSERVER
  NS_DECL_NSISSLSTATUSPROVIDER

  NS_IMETHOD Notify(nsIDOMHTMLFormElement* formNode, nsIDOMWindowInternal* window,
                    nsIURI *actionURL, PRBool* cancelSubmit);
  
protected:
  
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsCOMPtr<nsIStringBundle> mStringBundle;
  nsCOMPtr<nsIURI> mCurrentURI;
  nsCOMPtr<nsISecurityEventSink> mToplevelEventSink;
  
  enum lockIconState {
    lis_no_security,
    lis_broken_security,
    lis_mixed_security,
    lis_low_security,
    lis_high_security
  };

  lockIconState mPreviousSecurityState;

  void ResetStateTracking();
  PRUint32 mNewToplevelSecurityState;
  PRPackedBool mNewToplevelSecurityStateKnown;
  PRPackedBool mIsViewSource;

  nsXPIDLString mInfoTooltip;
  PRInt32 mDocumentRequestsInProgress;
  PRInt32 mSubRequestsInProgress;
  PRInt32 mSubRequestsHighSecurity;
  PRInt32 mSubRequestsLowSecurity;
  PRInt32 mSubRequestsBrokenSecurity;
  PRInt32 mSubRequestsNoSecurity;

  nsresult UpdateSecurityState(nsIRequest* aRequest);
  nsresult EvaluateAndUpdateSecurityState(nsIRequest *aRequest);
  void UpdateSubrequestMembers(nsIRequest *aRequest);

  void ObtainEventSink(nsIChannel *channel);
  
  nsCOMPtr<nsISupports> mSSLStatus;

  void GetBundleString(const PRUnichar* name, nsAString &outString);
  
  nsresult CheckPost(nsIURI *formURI, nsIURI *actionURL, PRBool *okayToPost);
  nsresult IsURLHTTPS(nsIURI* aURL, PRBool *value);
  nsresult IsURLJavaScript(nsIURI* aURL, PRBool *value);

  PRBool ConfirmEnteringSecure();
  PRBool ConfirmEnteringWeak();
  PRBool ConfirmLeavingSecure();
  PRBool ConfirmMixedMode();
  PRBool ConfirmPostToInsecure();
  PRBool ConfirmPostToInsecureFromSecure();

  
  nsresult GetNSSDialogs(nsISecurityWarningDialogs **);

  PLDHashTable mTransferringRequests;
};


#endif 
