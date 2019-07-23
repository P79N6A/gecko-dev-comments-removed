





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsIAccessibleDocument.h"
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

#define NS_DOCACCESSIBLE_IMPL_CID                       \
{  /* 9735bc5f-a4b6-4668-ab73-6f8434c8e750 */           \
  0x9735bc5f,                                           \
  0xa4b6,                                               \
  0x4668,                                               \
  { 0xab, 0x73, 0x6f, 0x84, 0x34, 0xc8, 0xe7, 0x50 }    \
}

class nsDocAccessible : public nsHyperTextAccessibleWrap,
                        public nsIAccessibleDocument,
                        public nsIDocumentObserver,
                        public nsIObserver,
                        public nsIScrollPositionListener,
                        public nsSupportsWeakReference
{  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDocAccessible, nsAccessible)

  NS_DECL_NSIACCESSIBLEDOCUMENT
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOCACCESSIBLE_IMPL_CID)

  NS_DECL_NSIOBSERVER

public:
  nsDocAccessible(nsIDOMNode *aNode, nsIWeakReference* aShell);
  virtual ~nsDocAccessible();

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD TakeFocus(void);

  
  NS_IMETHOD ScrollPositionWillChange(nsIScrollableView *aView,
                                      nscoord aX, nscoord aY);
  virtual void ViewPositionDidChange(nsIScrollableView* aScrollable,
                                     nsTArray<nsIWidget::Configuration>* aConfigurations) {}
  NS_IMETHOD ScrollPositionDidChange(nsIScrollableView *aView,
                                     nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual nsresult Init();
  virtual nsresult Shutdown();
  virtual nsIFrame* GetFrame();
  virtual PRBool IsDefunct();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);
  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  

  








  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                                      nsAccEvent::EEventRule aAllowDupes = nsAccEvent::eRemoveDupes,
                                      PRBool aIsAsynch = PR_FALSE);

  




  nsresult FireDelayedAccessibleEvent(nsIAccessibleEvent *aEvent);

  











  void InvalidateCacheSubtree(nsIContent *aContent, PRUint32 aEvent);

  





  void CacheAccessNode(void *aUniqueID, nsIAccessNode *aAccessNode);

  


  void RemoveAccessNodeFromCache(nsIAccessNode *aAccessNode);

  


  void FlushPendingEvents();

  




  virtual void FireDocLoadEvents(PRUint32 aEventType);

  


  virtual void FireAnchorJumpEvent();

  


  static void FlushEventsCallback(nsITimer *aTimer, void *aClosure);

protected:
  


  void ShutdownChildDocuments(nsIDocShellTreeItem *aStart);

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

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocAccessible,
                              NS_DOCACCESSIBLE_IMPL_CID)

#endif  
