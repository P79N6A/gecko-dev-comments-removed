








































#ifndef nsIFocusController_h__
#define nsIFocusController_h__

#include "nsISupports.h"
#include "nsCOMPtr.h"

class nsPIDOMWindow;
class nsIDOMNode;
class nsIController;
class nsIControllers;


#define NS_IFOCUSCONTROLLER_IID \
{ 0x6d733829, 0x8ae4, 0x43bd, \
  { 0xbe, 0xee, 0x35, 0x42, 0x0f, 0xe3, 0xe9, 0x32 } }

class nsIFocusController : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFOCUSCONTROLLER_IID)

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode)=0;
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode)=0;

  NS_IMETHOD GetControllerForCommand(nsPIDOMWindow* aContextWindow,
                                     const char * aCommand,
                                     nsIController** aResult)=0;
  NS_IMETHOD GetControllers(nsPIDOMWindow* aContextWindow, nsIControllers** aResult)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFocusController, NS_IFOCUSCONTROLLER_IID)

#endif 
