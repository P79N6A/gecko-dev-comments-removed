





































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
{  /* 9e97d7af-b20a-4a5a-a8d9-bcae0de0b7a2 */           \
  0x9e97d7af,                                           \
  0xb20a,                                               \
  0x4a5a,                                               \
  { 0xa8, 0xd9, 0xbc, 0xae, 0x0d, 0xe0, 0xb7, 0xa2 }    \
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
  virtual nsIAccessible* GetParent();

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  

  








  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                                      nsAccEvent::EEventRule aAllowDupes = nsAccEvent::eRemoveDupes,
                                      PRBool aIsAsynch = PR_FALSE);

  




  nsresult FireDelayedAccessibleEvent(nsIAccessibleEvent *aEvent);

  











  void InvalidateCacheSubtree(nsIContent *aContent, PRUint32 aEvent);

  





  void CacheAccessNode(void *aUniqueID, nsIAccessNode *aAccessNode);

  


  void RemoveAccessNodeFromCache(nsIAccessNode *aAccessNode);

  




  virtual void FireDocLoadEvents(PRUint32 aEventType);

  


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

  


  void FlushPendingEvents();

  


  nsresult PreparePendingEventsFlush();

    








    void FireTextChangeEventForText(nsIContent *aContent,
                                    CharacterDataChangeInfo* aInfo,
                                    PRBool aIsInserted);

    






    already_AddRefed<nsIAccessibleEvent>
    CreateTextChangeEventForNode(nsIAccessible *aContainerAccessible,
                                 nsIDOMNode *aChangeNode,
                                 nsIAccessible *aAccessibleForNode,
                                 PRBool aIsInserting,
                                 PRBool aIsAsynch);

  


  enum EEventFiringType {
    eNormalEvent,
    eDelayedEvent
  };

  











  nsresult FireShowHideEvents(nsIDOMNode *aDOMNode, PRBool aAvoidOnThisNode,
                              PRUint32 aEventType,
                              EEventFiringType aDelayedOrNormal,
                              PRBool aIsAsyncChange,
                              PRBool aForceIsFromUserInput);

    


    void FireValueChangeForTextFields(nsIAccessible *aPossibleTextFieldAccessible);

    nsAccessNodeHashtable mAccessNodeCache;
    void *mWnd;
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    nsCOMPtr<nsITimer> mFireEventTimer;
    PRUint16 mScrollPositionChangedTicks; 
    PRPackedBool mIsContentLoaded;
    PRPackedBool mIsLoadCompleteFired;

protected:

  PRBool mInFlushPendingEvents;
  PRBool mFireEventTimerStarted;
  nsTArray<nsRefPtr<nsAccEvent> > mEventsToFire;

    static PRUint32 gLastFocusedAccessiblesState;
    static nsIAtom *gLastFocusedFrameType;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocAccessible,
                              NS_DOCACCESSIBLE_IMPL_CID)

#endif  
