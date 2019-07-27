








#ifndef nsXULPopupListener_h___
#define nsXULPopupListener_h___

#include "nsCOMPtr.h"

#include "mozilla/dom/Element.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventListener.h"
#include "nsCycleCollectionParticipant.h"

class nsXULPopupListener : public nsIDOMEventListener
{
public:
    
    
    
    
    nsXULPopupListener(mozilla::dom::Element* aElement, bool aIsContext);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS(nsXULPopupListener)
    NS_DECL_NSIDOMEVENTLISTENER

protected:
    virtual ~nsXULPopupListener(void);

    
    
    virtual nsresult LaunchPopup(nsIDOMEvent* aEvent, nsIContent* aTargetContent);

    
    virtual void ClosePopup();

private:
#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
    
    nsresult FireFocusOnTargetContent(nsIDOMNode* aTargetNode);
#endif

    
    nsCOMPtr<mozilla::dom::Element> mElement;

    
    nsCOMPtr<nsIContent> mPopupContent; 

    
    bool mIsContext;
};

#endif 
