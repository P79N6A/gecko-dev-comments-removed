








































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
{ 0x58be9aa6, 0xedec, 0x46be, \
  { 0xa9, 0xf5, 0x6d, 0x8b, 0x57, 0x24, 0x18, 0xd5 } }

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

  NS_IMETHOD GetControllerForCommand(const char * aCommand, nsIController** aResult)=0;
  NS_IMETHOD GetControllers(nsIControllers** aResult)=0;

  NS_IMETHOD MoveFocus(PRBool aForward, nsIDOMElement* aElt)=0;
  NS_IMETHOD RewindFocusState()=0;

  NS_IMETHOD ResetElementFocus() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFocusController, NS_IFOCUSCONTROLLER_IID)

class NS_STACK_CLASS nsFocusSuppressor {
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

class NS_STACK_CLASS nsFocusScrollSuppressor
{
public:
  nsFocusScrollSuppressor(nsIFocusController* aController = nsnull)
  : mWasSuppressed(PR_FALSE)
  {
    Init(aController);
  }

  ~nsFocusScrollSuppressor()
  {
    Init(nsnull);
  }

  void Init(nsIFocusController* aController)
  {
    if (mController) {
      mController->SetSuppressFocusScroll(mWasSuppressed);
    }

    mController = aController;
    if (mController) {
      mController->GetSuppressFocusScroll(&mWasSuppressed);
      if (!mWasSuppressed) {
        mController->SetSuppressFocusScroll(PR_TRUE);
      }
    }
  }
private:
  nsCOMPtr<nsIFocusController> mController;
  PRBool                       mWasSuppressed;
};

#endif 
