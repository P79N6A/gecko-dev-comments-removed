



#ifndef mozilla_dom_indexeddb_permissionrequestbase_h__
#define mozilla_dom_indexeddb_permissionrequestbase_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIPermissionManager.h"
#include "nsISupportsImpl.h"
#include "nsString.h"

class nsIPrincipal;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Element;

namespace indexedDB {

class PermissionRequestBase
  : public nsIObserver
  , public nsIInterfaceRequestor
{
  nsCOMPtr<Element> mOwnerElement;
  nsCOMPtr<nsIPrincipal> mPrincipal;

public:
  enum PermissionValue {
    kPermissionAllowed = nsIPermissionManager::ALLOW_ACTION,
    kPermissionDenied = nsIPermissionManager::DENY_ACTION,
    kPermissionPrompt = nsIPermissionManager::PROMPT_ACTION
  };

  NS_DECL_ISUPPORTS

  
  
  
  static nsresult
  GetCurrentPermission(nsIPrincipal* aPrincipal,
                       PermissionValue* aCurrentValue);

  static PermissionValue
  PermissionValueForIntPermission(uint32_t aIntPermission);

  
  nsresult
  PromptIfNeeded(PermissionValue* aCurrentValue);

protected:
  PermissionRequestBase(Element* aOwnerElement,
                        nsIPrincipal* aPrincipal);

  
  virtual
  ~PermissionRequestBase();

  virtual void
  OnPromptComplete(PermissionValue aPermissionValue) = 0;

private:
  void
  SetExplicitPermission(nsIPrincipal* aPrincipal,
                        uint32_t aIntPermission);

  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
};

} 
} 
} 

#endif
