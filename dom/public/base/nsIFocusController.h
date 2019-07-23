








































#ifndef nsIFocusController_h__
#define nsIFocusController_h__

#include "nsISupports.h"
#include "nsCOMPtr.h"

class nsIDOMElement;
class nsIDOMNode;
class nsIDOMWindowInternal;
class nsIController;
class nsIControllers;
class nsAString;


#define NS_IFOCUSCONTROLLER_IID \
{ 0xda47ea2a, 0x5e9a, 0x4281, { 0x9b, 0x5b, 0xa0, 0x8c, 0x0e, 0x6b, 0x1f, 0xa5 } }

class nsIFocusController : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFOCUSCONTROLLER_IID)

  NS_IMETHOD GetFocusedElement(nsIDOMElement** aResult)=0;
  NS_IMETHOD SetFocusedElement(nsIDOMElement* aElement)=0;

  NS_IMETHOD GetFocusedWindow(nsIDOMWindowInternal** aResult)=0;
  NS_IMETHOD SetFocusedWindow(nsIDOMWindowInternal* aResult)=0;

  NS_IMETHOD GetSuppressFocus(PRBool* aSuppressFlag)=0;
  NS_IMETHOD SetSuppressFocus(PRBool aSuppressFlag, const char* aReason)=0;

  NS_IMETHOD GetSuppressFocusScroll(PRBool* aSuppressFlag)=0;
  NS_IMETHOD SetSuppressFocusScroll(PRBool aSuppressFlag)=0;
  
  NS_IMETHOD GetActive(PRBool* aActive)=0;
  NS_IMETHOD SetActive(PRBool aActive)=0;

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode)=0;
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode)=0;

  NS_IMETHOD GetPopupEvent(nsIDOMEvent** aEvent)=0;
  NS_IMETHOD SetPopupEvent(nsIDOMEvent* aEvent)=0;

  NS_IMETHOD GetControllerForCommand(const char * aCommand, nsIController** aResult)=0;
  NS_IMETHOD GetControllers(nsIControllers** aResult)=0;

  NS_IMETHOD MoveFocus(PRBool aForward, nsIDOMElement* aElt)=0;

  NS_IMETHOD ResetElementFocus() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFocusController, NS_IFOCUSCONTROLLER_IID)

class nsFocusSuppressor {
public:
  ~nsFocusSuppressor()
  {
    Unsuppress();
  }

  
  void Suppress(nsIFocusController *aController, const char *aReason)
  {
    Unsuppress();

    mController = aController;
    mReason = aReason;
    if (aController) {
      mController->SetSuppressFocus(PR_TRUE, mReason);
    }
  }

  void Unsuppress()
  {
    if (mController) {
      mController->SetSuppressFocus(PR_FALSE, mReason);
      mController = nsnull;
      mReason = nsnull;
    }
  }

  PRBool Suppressing()
  {
    return mController != nsnull;
  }

private:
  nsCOMPtr<nsIFocusController> mController;
  const char *mReason;
};

#endif 
