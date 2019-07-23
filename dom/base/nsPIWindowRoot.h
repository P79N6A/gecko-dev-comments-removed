






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsPIDOMEventTarget.h"

class nsPIDOMWindow;
class nsIControllers;
class nsIController;


#define NS_IWINDOWROOT_IID \
{ 0x313c1d52, 0x88f1, 0x46c7, \
  { 0xb3, 0x5c, 0x4e, 0x71, 0xec, 0x1b, 0x01, 0xf3 } }

class nsPIWindowRoot : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  virtual nsPIDOMWindow* GetWindow()=0;

  virtual void GetPopupNode(nsIDOMNode** aNode) = 0;
  virtual void SetPopupNode(nsIDOMNode* aNode) = 0;

  virtual nsresult GetControllerForCommand(const char *aCommand,
                                           nsIController** aResult) = 0;
  virtual nsresult GetControllers(nsIControllers** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
