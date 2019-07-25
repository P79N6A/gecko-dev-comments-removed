




































#ifndef mozilla_a11y_RootAccessible_h__
#define mozilla_a11y_RootAccessible_h__

#include "nsCaretAccessible.h"
#include "nsDocAccessibleWrap.h"


#include "nsHashtable.h"
#include "nsCaretAccessible.h"
#include "nsIDocument.h"
#include "nsIDOMEventListener.h"

class nsXULTreeAccessible;
class Relation;

namespace mozilla {
namespace a11y {

class RootAccessible : public nsDocAccessibleWrap,
                       public nsIDOMEventListener
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  RootAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                 nsIPresShell* aPresShell);
  virtual ~RootAccessible();

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual Relation RelationByType(PRUint32 aType);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  nsCaretAccessible* GetCaretAccessible();

  


  virtual void DocumentActivated(nsDocAccessible* aDocument);

protected:

  


  virtual nsresult AddEventListeners();
  virtual nsresult RemoveEventListeners();

  


  void ProcessDOMEvent(nsIDOMEvent* aEvent);

  


  void HandlePopupShownEvent(nsAccessible* aAccessible);

  


  void HandlePopupHidingEvent(nsINode* aNode);

#ifdef MOZ_XUL
    void HandleTreeRowCountChangedEvent(nsIDOMEvent* aEvent,
                                        nsXULTreeAccessible* aAccessible);
    void HandleTreeInvalidatedEvent(nsIDOMEvent* aEvent,
                                    nsXULTreeAccessible* aAccessible);

    PRUint32 GetChromeFlags();
#endif

    nsRefPtr<nsCaretAccessible> mCaretAccessible;
};

} 
} 

inline mozilla::a11y::RootAccessible*
nsAccessible::AsRoot()
{
  return mFlags & eRootAccessible ?
    static_cast<mozilla::a11y::RootAccessible*>(this) : nsnull;
}

#endif
