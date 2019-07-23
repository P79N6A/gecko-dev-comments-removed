







































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
                          public nsSupportsWeakReference
{
public:
  static NS_IMETHODIMP Create(nsIFocusController** aResult);

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_IMETHOD GetPopupNode(nsIDOMNode** aNode);
  NS_IMETHOD SetPopupNode(nsIDOMNode* aNode);

  NS_IMETHOD GetControllerForCommand(const char *aCommand, nsIController** aResult);
  NS_IMETHOD GetControllers(nsIControllers** aResult);

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFocusController,
                                           nsIFocusController)

public:
  static nsPIDOMWindow *GetWindowFromDocument(nsIDOMDocument* aElement);


protected:
  nsCOMPtr<nsIDOMNode> mPopupNode; 
};

#endif 
