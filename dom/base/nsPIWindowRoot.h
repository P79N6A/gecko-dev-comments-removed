





#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "mozilla/dom/EventTarget.h"
#include "nsWeakReference.h"

class nsPIDOMWindow;
class nsIControllers;
class nsIController;

namespace mozilla {
namespace dom {
class TabParent;
}
}

#define NS_IWINDOWROOT_IID \
{ 0x238edca0, 0xb30d, 0x46d3, \
 { 0xb2, 0x6a, 0x17, 0xb6, 0x21, 0x28, 0x89, 0x7e } }

class nsPIWindowRoot : public mozilla::dom::EventTarget
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  virtual nsPIDOMWindow* GetWindow()=0;

  
  virtual nsIDOMNode* GetPopupNode() = 0;
  virtual void SetPopupNode(nsIDOMNode* aNode) = 0;

  virtual nsresult GetControllerForCommand(const char *aCommand,
                                           nsIController** aResult) = 0;
  virtual nsresult GetControllers(nsIControllers** aResult) = 0;

  virtual void GetEnabledDisabledCommands(nsTArray<nsCString>& aEnabledCommands,
                                          nsTArray<nsCString>& aDisabledCommands) = 0;

  virtual void SetParentTarget(mozilla::dom::EventTarget* aTarget) = 0;
  virtual mozilla::dom::EventTarget* GetParentTarget() = 0;

  
  virtual void AddBrowser(mozilla::dom::TabParent* aBrowser) = 0;
  virtual void RemoveBrowser(mozilla::dom::TabParent* aBrowser) = 0;

  typedef void (*BrowserEnumerator)(mozilla::dom::TabParent* aTab, void* aArg);

  
  virtual void EnumerateBrowsers(BrowserEnumerator aEnumFunc, void* aArg) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
