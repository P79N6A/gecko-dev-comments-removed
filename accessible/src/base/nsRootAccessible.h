




































#ifndef _nsRootAccessible_H_
#define _nsRootAccessible_H_

#include "nsDocAccessibleWrap.h"
#include "nsHashtable.h"
#include "nsIAccessibleDocument.h"
#include "nsIDocument.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMXULListener.h"
#include "nsIAccessibleCaret.h"
#include "nsITimer.h"

#define NS_ROOTACCESSIBLE_IMPL_CID                      \
{  /* 7565f0d1-1465-4b71-906c-a623ac279f5d */           \
  0x7565f0d1,                                           \
  0x1465,                                               \
  0x4b71,                                               \
  { 0x90, 0x6c, 0xa6, 0x23, 0xac, 0x27, 0x9f, 0x5d }    \
}

const PRInt32 SCROLL_HASH_START_SIZE = 6;

class nsRootAccessible : public nsDocAccessibleWrap,
                         public nsIDOMEventListener
{
  NS_DECL_ISUPPORTS_INHERITED

  public:
    nsRootAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell);
    virtual ~nsRootAccessible();

    
    NS_IMETHOD GetName(nsAString& aName);
    NS_IMETHOD GetParent(nsIAccessible * *aParent);
    NS_IMETHOD GetRole(PRUint32 *aRole);
    NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
    NS_IMETHOD GetAccessibleRelated(PRUint32 aRelationType,
                                    nsIAccessible **aRelated);

    
    NS_IMETHOD FireDocLoadEvents(PRUint32 aEventType);

    
    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

    
    NS_IMETHOD GetCaretAccessible(nsIAccessible **aAccessibleCaret);

    
    NS_IMETHOD Init();
    NS_IMETHOD Shutdown();

    void ShutdownAll();
    
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ROOTACCESSIBLE_IMPL_CID)

    








    PRBool FireAccessibleFocusEvent(nsIAccessible *aFocusAccessible,
                                    nsIDOMNode *aFocusNode,
                                    nsIDOMEvent *aFocusEvent,
                                    PRBool aForceEvent = PR_FALSE);

  private:
    nsCOMPtr<nsITimer> mFireFocusTimer;
    static void FireFocusCallback(nsITimer *aTimer, void *aClosure);
    
  protected:
    nsresult AddEventListeners();
    nsresult RemoveEventListeners();
    nsresult HandleEventWithTarget(nsIDOMEvent* aEvent,
                                   nsIDOMNode* aTargetNode);
    static void GetTargetNode(nsIDOMEvent *aEvent, nsIDOMNode **aTargetNode);
    void TryFireEarlyLoadEvent(nsIDOMNode *aDocNode);
    void FireCurrentFocusEvent();
    void GetChromeEventHandler(nsIDOMEventTarget **aChromeTarget);
#ifdef MOZ_XUL
    PRUint32 GetChromeFlags();
#endif
    already_AddRefed<nsIDocShellTreeItem>
           GetContentDocShell(nsIDocShellTreeItem *aStart);
    nsCOMPtr<nsIAccessibleCaret> mCaretAccessible;
    PRPackedBool mIsInDHTMLMenu;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsRootAccessible, NS_ROOTACCESSIBLE_IMPL_CID)

#endif  
