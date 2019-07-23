




































#ifndef _nsRootAccessible_H_
#define _nsRootAccessible_H_

#include "nsCaretAccessible.h"
#include "nsDocAccessibleWrap.h"

#include "nsIAccessibleDocument.h"
#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#endif

#include "nsHashtable.h"
#include "nsCaretAccessible.h"
#include "nsIDocument.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
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
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  virtual nsresult Init();
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsIAccessible* GetParent();

  
  virtual void FireDocLoadEvents(PRUint32 aEventType);

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ROOTACCESSIBLE_IMPL_CID)

    








    PRBool FireAccessibleFocusEvent(nsIAccessible *aFocusAccessible,
                                    nsIDOMNode *aFocusNode,
                                    nsIDOMEvent *aFocusEvent,
                                    PRBool aForceEvent = PR_FALSE,
                                    PRBool aIsAsynch = PR_FALSE);
    



    void FireCurrentFocusEvent();

    nsCaretAccessible *GetCaretAccessible();

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
    void GetChromeEventHandler(nsIDOMEventTarget **aChromeTarget);

    


    nsresult HandlePopupShownEvent(nsIAccessible *aAccessible);
    nsresult HandlePopupHidingEvent(nsIDOMNode *aNode,
                                    nsIAccessible *aAccessible);

#ifdef MOZ_XUL
    nsresult HandleTreeRowCountChangedEvent(nsIDOMEvent *aEvent,
                                            nsXULTreeAccessible *aAccessible);
    nsresult HandleTreeInvalidatedEvent(nsIDOMEvent *aEvent,
                                        nsXULTreeAccessible *aAccessible);

    PRUint32 GetChromeFlags();
#endif
    already_AddRefed<nsIDocShellTreeItem>
           GetContentDocShell(nsIDocShellTreeItem *aStart);
    nsRefPtr<nsCaretAccessible> mCaretAccessible;
    nsCOMPtr<nsIDOMNode> mCurrentARIAMenubar;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsRootAccessible, NS_ROOTACCESSIBLE_IMPL_CID)

#endif  
