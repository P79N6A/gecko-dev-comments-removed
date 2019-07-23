












































#ifndef nsXULPopupListener_h___
#define nsXULPopupListener_h___

#include "nsCOMPtr.h"

#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsIFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMContextMenuListener.h"

class nsXULPopupListener : public nsIDOMMouseListener,
                           public nsIDOMContextMenuListener
{
public:
    
    
    
    
    nsXULPopupListener(nsIDOMElement *aElement, PRBool aIsContext);
    virtual ~nsXULPopupListener(void);

public:
    
    NS_DECL_ISUPPORTS

    
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

    
    nsresult FireFocusOnTargetContent(nsIDOMNode* aTargetNode);

    
    nsIDOMElement* mElement;               

    
    nsCOMPtr<nsIContent> mPopupContent; 

    
    PRBool mIsContext;
};




nsresult
NS_NewXULPopupListener(nsIDOMElement* aElement, PRBool aIsContext,
                       nsIDOMEventListener** aListener);

#endif 
