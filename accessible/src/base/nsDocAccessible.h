





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsEventShell.h"
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
{  /* 5641921c-a093-4292-9dca-0b51813db57d */           \
  0x5641921c,                                           \
  0xa093,                                               \
  0x4292,                                               \
  { 0x9d, 0xca, 0x0b, 0x51, 0x81, 0x3d, 0xb5, 0x7d }    \
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

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual nsresult Init();
  virtual nsresult Shutdown();
  virtual nsIFrame* GetFrame();
  virtual PRBool IsDefunct();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);

  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);
  virtual nsAccessible* GetParent();

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  

  








  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                                      nsAccEvent::EEventRule aAllowDupes = nsAccEvent::eRemoveDupes,
                                      PRBool aIsAsynch = PR_FALSE,
                                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  




  nsresult FireDelayedAccessibleEvent(nsAccEvent *aEvent);

  











  void InvalidateCacheSubtree(nsIContent *aContent, PRUint32 aEvent);

  









  nsAccessNode* GetCachedAccessNode(void *aUniqueID);

  







  PRBool CacheAccessNode(void *aUniqueID, nsAccessNode *aAccessNode);

  


  void RemoveAccessNodeFromCache(nsIAccessNode *aAccessNode);

  




  virtual void FireDocLoadEvents(PRUint32 aEventType);

  



  void ProcessPendingEvent(nsAccEvent* aEvent);

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

  












  already_AddRefed<nsAccEvent>
    CreateTextChangeEventForNode(nsIAccessible *aContainerAccessible,
                                 nsIDOMNode *aNode,
                                 nsIAccessible *aAccessible,
                                 PRBool aIsInserting,
                                 PRBool aIsAsynch,
                                 EIsFromUserInput aIsFromUserInput = eAutoDetect);

  


  enum EEventFiringType {
    eNormalEvent,
    eDelayedEvent
  };

  











  nsresult FireShowHideEvents(nsIDOMNode *aDOMNode, PRBool aAvoidOnThisNode,
                              PRUint32 aEventType,
                              EEventFiringType aDelayedOrNormal,
                              PRBool aIsAsyncChange,
                              EIsFromUserInput aIsFromUserInput = eAutoDetect);

    


    void FireValueChangeForTextFields(nsIAccessible *aPossibleTextFieldAccessible);

    nsAccessNodeHashtable mAccessNodeCache;
    void *mWnd;
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    PRUint16 mScrollPositionChangedTicks; 
    PRPackedBool mIsContentLoaded;
    PRPackedBool mIsLoadCompleteFired;

protected:

  nsRefPtr<nsAccEventQueue> mEventQueue;

    static PRUint32 gLastFocusedAccessiblesState;
    static nsIAtom *gLastFocusedFrameType;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocAccessible,
                              NS_DOCACCESSIBLE_IMPL_CID)

#endif  
