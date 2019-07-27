





#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "mozilla/dom/EventTarget.h"

class nsPIDOMWindow;
class nsIControllers;
class nsIController;

#define NS_IWINDOWROOT_IID \
{ 0x3f71f50c, 0xa7e0, 0x43bc, \
 { 0xac, 0x25, 0x4d, 0xbb, 0x88, 0x7b, 0x21, 0x09 } }

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

  virtual void SetParentTarget(mozilla::dom::EventTarget* aTarget) = 0;
  virtual mozilla::dom::EventTarget* GetParentTarget() = 0;
  virtual nsIDOMWindow* GetOwnerGlobal() MOZ_OVERRIDE = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
