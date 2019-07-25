




































#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "base/basictypes.h"

#include "nsIContentPermissionPrompt.h"
#include "nsString.h"
#include "nsIDOMElement.h"

#include "mozilla/dom/PContentPermissionRequestParent.h"

class nsContentPermissionRequestProxy;

namespace mozilla {
namespace dom {

class ContentPermissionRequestParent : public PContentPermissionRequestParent
{
 public:
  ContentPermissionRequestParent(const nsACString& type, nsIDOMElement *element, const IPC::URI& principal);
  virtual ~ContentPermissionRequestParent();
  
  nsCOMPtr<nsIURI>           mURI;
  nsCOMPtr<nsIDOMElement>    mElement;
  nsCOMPtr<nsContentPermissionRequestProxy> mProxy;
  nsCString mType;

 private:  
  virtual bool Recvprompt();
};
  
} 
} 

class nsContentPermissionRequestProxy : public nsIContentPermissionRequest
{
 public:
  nsContentPermissionRequestProxy();
  virtual ~nsContentPermissionRequestProxy();
  
  nsresult Init(const nsACString& type, mozilla::dom::ContentPermissionRequestParent* parent);
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

 private:
  
  mozilla::dom::ContentPermissionRequestParent* mParent;
  nsCString mType;
};
#endif 

