












































#ifndef nsXULPopupListener_h___
#define nsXULPopupListener_h___

#include "nsCOMPtr.h"

#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsIFrame.h"
#include "nsIDOMEventListener.h"
#include "nsCycleCollectionParticipant.h"

class nsXULPopupListener : public nsIDOMEventListener
{
public:
    
    
    
    
    nsXULPopupListener(nsIDOMElement *aElement, PRBool aIsContext);
    virtual ~nsXULPopupListener(void);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsXULPopupListener)
    NS_DECL_NSIDOMEVENTLISTENER

protected:
    
    
    virtual nsresult LaunchPopup(nsIDOMEvent* aEvent, nsIContent* aTargetContent);

    
    virtual void ClosePopup();

private:
#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
    
    nsresult FireFocusOnTargetContent(nsIDOMNode* aTargetNode);
#endif

    
    nsCOMPtr<nsIDOMElement> mElement;

    
    nsCOMPtr<nsIContent> mPopupContent; 

    
    PRBool mIsContext;
};

#endif 
