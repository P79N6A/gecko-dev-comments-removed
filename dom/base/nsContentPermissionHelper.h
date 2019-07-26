



#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "nsIContentPermissionPrompt.h"
#include "nsString.h"

#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/PContentPermissionRequestParent.h"

class nsContentPermissionRequestProxy;

namespace mozilla {
namespace dom {

class Element;

class ContentPermissionRequestParent : public PContentPermissionRequestParent
{
 public:
  ContentPermissionRequestParent(const nsACString& type,
                                 const nsACString& access,
                                 Element* element,
                                 const IPC::Principal& principal);
  virtual ~ContentPermissionRequestParent();

  bool IsBeingDestroyed();

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<Element> mElement;
  nsCOMPtr<nsContentPermissionRequestProxy> mProxy;
  nsCString mType;
  nsCString mAccess;

 private:
  virtual bool Recvprompt();
  virtual void ActorDestroy(ActorDestroyReason why);
};

} 
} 

class nsContentPermissionRequestProxy : public nsIContentPermissionRequest
{
 public:
  nsContentPermissionRequestProxy();
  virtual ~nsContentPermissionRequestProxy();

  nsresult Init(const nsACString& type, const nsACString& access, mozilla::dom::ContentPermissionRequestParent* parent);
  void OnParentDestroyed();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

 private:
  
  mozilla::dom::ContentPermissionRequestParent* mParent;
  nsCString mType;
  nsCString mAccess;
};
#endif 

