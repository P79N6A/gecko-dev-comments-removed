







































#ifndef nsFocusController_h__
#define nsFocusController_h__

#include "nsCOMPtr.h"
#include "nsIFocusController.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"
#include "nsCycleCollectionParticipant.h"

class nsIDOMElement;
class nsIDOMWindow;
class nsPIDOMWindow;
class nsIController;
class nsIControllers;

class nsFocusController : public nsIFocusController, 
                          public nsIDOMFocusListener,
                          public nsSupportsWeakReference
{
public:
  static NS_IMETHODIMP Create(nsIFocusController** aResult);

protected:
  nsFocusController(void);
  virtual ~nsFocusController(void);

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_IMETHOD GetFocusedElement(nsIDOMElement** aResult);
  NS_IMETHOD SetFocusedElement(nsIDOMElement* aElement);

  NS_IMETHOD GetFocusedWindow(nsIDOMWindowInternal** aResult);
  NS_IMETHOD SetFocusedWindow(nsIDOMWindowInternal* aResult);

  NS_IMETHOD GetSuppressFocus(PRBool* aSuppressFlag);
  NS_IMETHOD SetSuppressFocus(PRBool aSuppressFlag, const char* aReason);

  NS_IMETHOD GetSuppressFocusScroll(PRBool* aSuppressFlag);
  NS_IMETHOD SetSuppressFocusScroll(PRBool aSuppressFlag);
  
  NS_IMETHOD GetActive(PRBool* aActive);
  NS_IMETHOD SetActive(PRBool aActive);

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode);
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode);

  NS_IMETHOD GetPopupEvent(nsIDOMEvent** aEvent);
  NS_IMETHOD SetPopupEvent(nsIDOMEvent* aEvent);

  NS_IMETHOD GetControllerForCommand(const char *aCommand, nsIController** aResult);
  NS_IMETHOD GetControllers(nsIControllers** aResult);

  NS_IMETHOD MoveFocus(PRBool aForward, nsIDOMElement* aElt);

  NS_IMETHOD ResetElementFocus();

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* anEvent) { return NS_OK; };

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFocusController,
                                           nsIFocusController)

protected:
  void UpdateCommands();
  void UpdateWWActiveWindow();

public:
  static nsPIDOMWindow *GetWindowFromDocument(nsIDOMDocument* aElement);


protected:
  nsCOMPtr<nsIDOMElement> mCurrentElement; 
  nsCOMPtr<nsPIDOMWindow> mCurrentWindow; 
  nsCOMPtr<nsIDOMNode> mPopupNode; 
  nsCOMPtr<nsIDOMEvent> mPopupEvent;

  PRUint32 mSuppressFocus;
  PRPackedBool mSuppressFocusScroll;
  PRPackedBool mActive;
  PRPackedBool mUpdateWindowWatcher;
  PRPackedBool mNeedUpdateCommands;
};

#endif 
