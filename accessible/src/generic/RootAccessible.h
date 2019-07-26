




#ifndef mozilla_a11y_RootAccessible_h__
#define mozilla_a11y_RootAccessible_h__

#include "HyperTextAccessible.h"
#include "DocAccessibleWrap.h"

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"

namespace mozilla {
namespace a11y {

class RootAccessible : public DocAccessibleWrap,
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
  virtual Relation RelationByType(uint32_t aType);
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  

  


  virtual void DocumentActivated(DocAccessible* aDocument);

protected:

  


  virtual nsresult AddEventListeners();
  virtual nsresult RemoveEventListeners();

  


  void ProcessDOMEvent(nsIDOMEvent* aEvent);

  


  void HandlePopupShownEvent(Accessible* aAccessible);

  


  void HandlePopupHidingEvent(nsINode* aNode);

#ifdef MOZ_XUL
    void HandleTreeRowCountChangedEvent(nsIDOMEvent* aEvent,
                                        XULTreeAccessible* aAccessible);
    void HandleTreeInvalidatedEvent(nsIDOMEvent* aEvent,
                                    XULTreeAccessible* aAccessible);

    uint32_t GetChromeFlags();
#endif
};

inline RootAccessible*
Accessible::AsRoot()
{
  return IsRoot() ? static_cast<mozilla::a11y::RootAccessible*>(this) : nullptr;
}

} 
} 

#endif
