












































#ifndef nsXULPopupListener_h___
#define nsXULPopupListener_h___

#include "nsCOMPtr.h"

#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsIFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsCycleCollectionParticipant.h"

class nsXULPopupListener : public nsIDOMMouseListener,
                           public nsIDOMContextMenuListener
{
public:
    
    
    
    
    nsXULPopupListener(nsIDOMElement *aElement, PRBool aIsContext);
    virtual ~nsXULPopupListener(void);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULPopupListener,
                                             nsIDOMMouseListener)

    
    NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
    NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
    NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }

    
    NS_IMETHOD ContextMenu(nsIDOMEvent* aContextMenuEvent);

    
    NS_IMETHOD HandleEvent(nsIDOMEvent* anEvent) { return NS_OK; }

protected:

    
    
    virtual nsresult LaunchPopup(nsIDOMEvent* aEvent, nsIContent* aTargetContent);

    
    virtual void ClosePopup();

private:

    
    
    
    nsresult PreLaunchPopup(nsIDOMEvent* aMouseEvent);

#ifndef NS_CONTEXT_MENU_IS_MOUSEUP
    
    nsresult FireFocusOnTargetContent(nsIDOMNode* aTargetNode);
#endif

    
    nsCOMPtr<nsIDOMElement> mElement;

    
    nsCOMPtr<nsIContent> mPopupContent; 

    
    PRBool mIsContext;
};




nsresult
NS_NewXULPopupListener(nsIDOMElement* aElement, PRBool aIsContext,
                       nsIDOMEventListener** aListener);

#endif 
