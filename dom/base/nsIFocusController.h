








































#ifndef nsIFocusController_h__
#define nsIFocusController_h__

#include "nsISupports.h"
#include "nsCOMPtr.h"

class nsIDOMNode;
class nsIController;
class nsIControllers;


#define NS_IFOCUSCONTROLLER_IID \
{ 0x2879db1c, 0x47aa, 0x46c4, \
  { 0xb1, 0x84, 0x25, 0x90, 0xcc, 0x39, 0xf2, 0x62 } }

class nsIFocusController : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFOCUSCONTROLLER_IID)

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode)=0;
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode)=0;

  NS_IMETHOD GetControllerForCommand(const char * aCommand, nsIController** aResult)=0;
  NS_IMETHOD GetControllers(nsIControllers** aResult)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFocusController, NS_IFOCUSCONTROLLER_IID)

#endif 
