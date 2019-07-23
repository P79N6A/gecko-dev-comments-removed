





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsIAccessibleDocument.h"
#include "nsPIAccessibleDocument.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIEditor.h"
#include "nsIObserver.h"
#include "nsIScrollPositionListener.h"
#include "nsITimer.h"
#include "nsIWeakReference.h"
#include "nsCOMArray.h"
#include "nsIDocShellTreeNode.h"

class nsIScrollableView;

const PRUint32 kDefaultCacheSize = 256;

class nsDocAccessible : public nsHyperTextAccessibleWrap,
                        public nsIAccessibleDocument,
                        public nsPIAccessibleDocument,
                        public nsIDocumentObserver,
                        public nsIObserver,
                        public nsIScrollPositionListener,
                        public nsSupportsWeakReference
{  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEDOCUMENT
  NS_DECL_NSPIACCESSIBLEDOCUMENT
  NS_DECL_NSIOBSERVER

  public:
    nsDocAccessible(nsIDOMNode *aNode, nsIWeakReference* aShell);
    virtual ~nsDocAccessible();

    NS_IMETHOD GetRole(PRUint32 *aRole);
    NS_IMETHOD SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);
    NS_IMETHOD GetName(nsAString& aName);
    NS_IMETHOD GetDescription(nsAString& aDescription);
    NS_IMETHOD GetARIAState(PRUint32 *aState);
    NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
    NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
    NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);
    NS_IMETHOD GetParent(nsIAccessible **aParent);
    NS_IMETHOD TakeFocus(void);

    
    NS_IMETHOD ScrollPositionWillChange(nsIScrollableView *aView, nscoord aX, nscoord aY);
    NS_IMETHOD ScrollPositionDidChange(nsIScrollableView *aView, nscoord aX, nscoord aY);

    
    NS_DECL_NSIDOCUMENTOBSERVER

    static void FlushEventsCallback(nsITimer *aTimer, void *aClosure);

    
    NS_IMETHOD Shutdown();
    NS_IMETHOD Init();

    
    NS_IMETHOD_(nsIFrame *) GetFrame(void);

    
    NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

    













    nsresult FireDelayedToolkitEvent(PRUint32 aEvent, nsIDOMNode *aDOMNode,
                                     nsAccEvent::EEventRule aAllowDupes = nsAccEvent::eRemoveDupes,
                                     PRBool aIsAsynch = PR_FALSE);

    




    nsresult FireDelayedAccessibleEvent(nsIAccessibleEvent *aEvent);

    void ShutdownChildDocuments(nsIDocShellTreeItem *aStart);

  protected:
    virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
    virtual nsresult AddEventListeners();
    virtual nsresult RemoveEventListeners();
    void AddScrollListener();
    void RemoveScrollListener();

    




    void InvalidateChildrenInSubtree(nsIDOMNode *aStartNode);
    void RefreshNodes(nsIDOMNode *aStartNode);
    static void ScrollTimerCallback(nsITimer *aTimer, void *aClosure);

    






    void AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    





    void ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute);

    








    void FireTextChangeEventForText(nsIContent *aContent,
                                    CharacterDataChangeInfo* aInfo,
                                    PRBool aIsInserted);

    






    already_AddRefed<nsIAccessibleTextChangeEvent>
    CreateTextChangeEventForNode(nsIAccessible *aContainerAccessible,
                                 nsIDOMNode *aChangeNode,
                                 nsIAccessible *aAccessibleForNode,
                                 PRBool aIsInserting,
                                 PRBool aIsAsynch);

    









    nsresult FireShowHideEvents(nsIDOMNode *aDOMNode, PRBool aAvoidOnThisNode, PRUint32 aEventType,
                                PRBool aDelay, PRBool aForceIsFromUserInput);

    


    void FireValueChangeForTextFields(nsIAccessible *aPossibleTextFieldAccessible);

    nsAccessNodeHashtable mAccessNodeCache;
    void *mWnd;
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    nsCOMPtr<nsITimer> mFireEventTimer;
    PRUint16 mScrollPositionChangedTicks; 
    PRPackedBool mIsContentLoaded;
    PRPackedBool mIsLoadCompleteFired;
    nsCOMArray<nsIAccessibleEvent> mEventsToFire;

protected:
    PRBool mIsAnchor;
    PRBool mIsAnchorJumped;
    PRBool mInFlushPendingEvents;
    static PRUint32 gLastFocusedAccessiblesState;
    static nsIAtom *gLastFocusedFrameType;
};

#endif  
