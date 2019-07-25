




































#ifndef nsGeolocationOOP_h
#define nsGeolocationOOP_h

#include "base/basictypes.h"

#include "nsIGeolocationProvider.h"
#include "nsIContentPermissionPrompt.h"
#include "nsString.h"
#include "nsIDOMElement.h"

#include "mozilla/dom/PContentPermissionRequestParent.h"

class nsGeolocationRequestProxy;

namespace mozilla {
namespace dom {

class GeolocationRequestParent : public PContentPermissionRequestParent
{
 public:
  GeolocationRequestParent(nsIDOMElement *element, const IPC::URI& principal);
  virtual ~GeolocationRequestParent();
  
  nsCOMPtr<nsIURI>           mURI;
  nsCOMPtr<nsIDOMElement>    mElement;
  nsCOMPtr<nsGeolocationRequestProxy> mProxy;

 private:  
  virtual bool Recvprompt();
};
  
} 
} 

class nsGeolocationRequestProxy : public nsIContentPermissionRequest
{
 public:
  nsGeolocationRequestProxy();
  virtual ~nsGeolocationRequestProxy();
  
  nsresult Init(mozilla::dom::GeolocationRequestParent* parent);
  
  NS_DECL_ISUPPORTS;
  NS_DECL_NSICONTENTPERMISSIONREQUEST;

 private:
  
  mozilla::dom::GeolocationRequestParent* mParent;
};
#endif 

