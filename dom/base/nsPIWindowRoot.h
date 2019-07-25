






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsPIDOMEventTarget.h"

class nsPIDOMWindow;
class nsIControllers;
class nsIController;
struct JSContext;


#define NS_IWINDOWROOT_IID \
{ 0x2e26a297, 0x6e40, 0x41c1, \
  { 0x81, 0xc9, 0x73, 0x06, 0x57, 0x1f, 0x95, 0x5e } }

class nsPIWindowRoot : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  virtual nsPIDOMWindow* GetWindow()=0;

  virtual void GetPopupNode(nsIDOMNode** aNode) = 0;
  virtual void SetPopupNode(nsIDOMNode* aNode) = 0;

  virtual nsresult GetControllerForCommand(const char *aCommand,
                                           nsIController** aResult) = 0;
  virtual nsresult GetControllers(nsIControllers** aResult) = 0;

  virtual void SetParentTarget(nsPIDOMEventTarget* aTarget) = 0;
  virtual nsPIDOMEventTarget* GetParentTarget() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
