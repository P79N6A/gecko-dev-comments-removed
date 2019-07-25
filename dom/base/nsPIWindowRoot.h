






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsIDOMEventTarget.h"

class nsPIDOMWindow;
class nsIControllers;
class nsIController;
struct JSContext;


#define NS_IWINDOWROOT_IID \
{ 0x426c1b56, 0xe38a, 0x435e, \
  { 0xb2, 0x91, 0xbe, 0x15, 0x57, 0xf2, 0xa0, 0xa2 } }

class nsPIWindowRoot : public nsIDOMEventTarget
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  virtual nsPIDOMWindow* GetWindow()=0;

  
  virtual nsIDOMNode* GetPopupNode() = 0;
  virtual void SetPopupNode(nsIDOMNode* aNode) = 0;

  virtual nsresult GetControllerForCommand(const char *aCommand,
                                           nsIController** aResult) = 0;
  virtual nsresult GetControllers(nsIControllers** aResult) = 0;

  virtual void SetParentTarget(nsIDOMEventTarget* aTarget) = 0;
  virtual nsIDOMEventTarget* GetParentTarget() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
