




#ifndef mozilla_a11y_RootAccessible_h__
#define mozilla_a11y_RootAccessible_h__

#include "HyperTextAccessible.h"
#include "DocAccessibleWrap.h"

#include "nsIDOMEventListener.h"

class nsIDocument;

namespace mozilla {
namespace a11y {

class RootAccessible : public DocAccessibleWrap,
                       public nsIDOMEventListener
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  RootAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                 nsIPresShell* aPresShell);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE;

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName) MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  

  


  virtual void DocumentActivated(DocAccessible* aDocument);

protected:
  virtual ~RootAccessible();

  


  virtual nsresult AddEventListeners() MOZ_OVERRIDE;
  virtual nsresult RemoveEventListeners() MOZ_OVERRIDE;

  


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
