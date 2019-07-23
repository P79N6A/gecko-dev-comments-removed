







































#ifndef nsFocusController_h__
#define nsFocusController_h__

#include "nsCOMPtr.h"
#include "nsIFocusController.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"
#include "nsCycleCollectionParticipant.h"

class nsPIDOMWindow;
class nsIController;
class nsIControllers;

class nsFocusController : public nsIFocusController,
                          public nsSupportsWeakReference
{
public:
  static NS_IMETHODIMP Create(nsIFocusController** aResult);

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode);
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode);

  NS_IMETHOD GetControllerForCommand(nsPIDOMWindow* aContextWindow,
                                     const char *aCommand,
                                     nsIController** aResult);
  NS_IMETHOD GetControllers(nsPIDOMWindow* aContextWindow, nsIControllers** aResult);

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFocusController,
                                           nsIFocusController)


protected:
  nsCOMPtr<nsIDOMNode> mPopupNode; 
};

#endif 
