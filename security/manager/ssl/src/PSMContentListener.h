





#ifndef mozilla_psm_PSMCOntentListener_h_
#define mozilla_psm_PSMCOntentListener_h_

#include "nsCOMPtr.h"
#include "nsIURIContentListener.h"
#include "nsWeakReference.h"

#define NS_PSMCONTENTLISTEN_CID {0xc94f4a30, 0x64d7, 0x11d4, {0x99, 0x60, 0x00, 0xb0, 0xd0, 0x23, 0x54, 0xa0}}
#define NS_PSMCONTENTLISTEN_CONTRACTID "@mozilla.org/security/psmdownload;1"

namespace mozilla { namespace psm {

class PSMContentListener : public nsIURIContentListener,
                           public nsSupportsWeakReference
{
public:
  PSMContentListener();
  nsresult init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURICONTENTLISTENER

protected:
  virtual ~PSMContentListener();

private:
  nsCOMPtr<nsISupports> mLoadCookie;
  nsCOMPtr<nsIURIContentListener> mParentContentListener;
};

} } 

#endif 
