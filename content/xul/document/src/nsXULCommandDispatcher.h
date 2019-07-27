











#ifndef nsXULCommandDispatcher_h__
#define nsXULCommandDispatcher_h__

#include "nsCOMPtr.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsWeakReference.h"
#include "nsIDOMNode.h"
#include "nsString.h"
#include "nsCycleCollectionParticipant.h"

class nsIDOMElement;
class nsPIWindowRoot;

class nsXULCommandDispatcher : public nsIDOMXULCommandDispatcher,
                               public nsSupportsWeakReference
{
public:
    explicit nsXULCommandDispatcher(nsIDocument* aDocument);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULCommandDispatcher,
                                             nsIDOMXULCommandDispatcher)

    
    NS_DECL_NSIDOMXULCOMMANDDISPATCHER

    void Disconnect();
protected:
    virtual ~nsXULCommandDispatcher();

    already_AddRefed<nsPIWindowRoot> GetWindowRoot();

    nsIContent* GetRootFocusedContentAndWindow(nsPIDOMWindow** aWindow);

    nsCOMPtr<nsIDocument> mDocument;

    class Updater {
    public:
      Updater(nsIDOMElement* aElement,
              const nsAString& aEvents,
              const nsAString& aTargets)
          : mElement(aElement),
            mEvents(aEvents),
            mTargets(aTargets),
            mNext(nullptr)
      {}

      nsCOMPtr<nsIDOMElement> mElement;
      nsString                mEvents;
      nsString                mTargets;
      Updater*                mNext;
    };

    Updater* mUpdaters;

    bool Matches(const nsString& aList, 
                   const nsAString& aElement);
};

#endif 
