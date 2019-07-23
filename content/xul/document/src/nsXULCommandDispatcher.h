












































#ifndef nsXULCommandDispatcher_h__
#define nsXULCommandDispatcher_h__

#include "nsCOMPtr.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMFocusListener.h"
#include "nsWeakReference.h"
#include "nsIDOMNode.h"
#include "nsString.h"

class nsIDOMElement;
class nsIFocusController;

class nsXULCommandDispatcher : public nsIDOMXULCommandDispatcher,
                               public nsSupportsWeakReference
{
protected:
    nsXULCommandDispatcher(nsIDocument* aDocument);
    virtual ~nsXULCommandDispatcher();

    void EnsureFocusController();

public:

    static NS_IMETHODIMP
    Create(nsIDocument* aDocument, nsIDOMXULCommandDispatcher** aResult);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIDOMXULCOMMANDDISPATCHER

protected:
    nsIFocusController* mFocusController; 
    nsIDocument* mDocument; 

    class Updater {
    public:
      Updater(nsIDOMElement* aElement,
              const nsAString& aEvents,
              const nsAString& aTargets)
          : mElement(aElement),
            mEvents(aEvents),
            mTargets(aTargets),
            mNext(nsnull)
      {}

      nsIDOMElement* mElement; 
      nsString       mEvents;
      nsString       mTargets;
      Updater*       mNext;
    };

    Updater* mUpdaters;

    PRBool Matches(const nsString& aList, 
                   const nsAString& aElement);
};

#endif 
