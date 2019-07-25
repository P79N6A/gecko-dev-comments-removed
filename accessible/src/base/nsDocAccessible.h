





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsIAccessibleDocument.h"

#include "nsHyperTextAccessibleWrap.h"
#include "nsEventShell.h"

#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
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
  using nsAccessible::GetParent;

  nsDocAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                  nsIWeakReference* aShell);
  virtual ~nsDocAccessible();

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);
  NS_IMETHOD TakeFocus(void);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual PRBool Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame();
  virtual PRBool IsDefunct();
  virtual nsINode* GetNode() const { return mDocument; }
  virtual nsIDocument* GetDocumentNode() const { return mDocument; }

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);

  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);

#ifdef DEBUG_ACCDOCMGR
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);
#endif

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  

  


  PRBool IsContentLoaded() const
  {
    return mDocument && mDocument->IsVisible() &&
      (mDocument->IsShowing() || mIsLoaded);
  }

  






  void MarkAsLoaded() { mIsLoaded = PR_TRUE; }
  void MarkAsLoading() { mIsLoaded = PR_FALSE; }

  


  virtual void* GetNativeWindow() const;

  


  nsDocAccessible* ParentDocument() const
    { return mParent ? mParent->GetDocAccessible() : nsnull; }

  


  PRUint32 ChildDocumentCount() const
    { return mChildDocuments.Length(); }

  


  nsDocAccessible* GetChildDocumentAt(PRUint32 aIndex) const
    { return mChildDocuments.SafeElementAt(aIndex, nsnull); }

  






  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsINode *aNode,
                                      AccEvent::EEventRule aAllowDupes = AccEvent::eRemoveDupes,
                                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  




  nsresult FireDelayedAccessibleEvent(AccEvent* aEvent);

  





  nsAccessible* GetCachedAccessible(nsINode* aNode);

  






  nsAccessible* GetCachedAccessibleByUniqueID(void* aUniqueID)
  {
    return UniqueID() == aUniqueID ?
      this : mAccessibleCache.GetWeak(aUniqueID);
  }

  



  nsAccessible* GetCachedAccessibleByUniqueIDInSubtree(void* aUniqueID);

  






  PRBool IsDependentID(const nsAString& aID) const
    { return mDependentIDsHash.Get(aID, nsnull); }

  






  bool BindToDocument(nsAccessible* aAccessible, nsRoleMapEntry* aRoleMapEntry);

  


  void UnbindFromDocument(nsAccessible* aAccessible);

  



  void ProcessPendingEvent(AccEvent* aEvent);

  


  void UpdateTree(nsIContent* aContainerNode, nsIContent* aStartChildNode,
                  nsIContent* aEndChildNode, PRBool aIsInsert);

  


  void RecreateAccessible(nsINode* aNode);

  







  void NotifyOfCachingStart(nsAccessible* aAccessible);
  void NotifyOfCachingEnd(nsAccessible* aAccessible);

protected:

  
  virtual void CacheChildren();

  
    virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
    virtual nsresult AddEventListeners();
    virtual nsresult RemoveEventListeners();
    void AddScrollListener();
    void RemoveScrollListener();

  



  bool AppendChildDocument(nsDocAccessible* aChildDocument)
  {
    return mChildDocuments.AppendElement(aChildDocument);
  }

  



  void RemoveChildDocument(nsDocAccessible* aChildDocument)
  {
    mChildDocuments.RemoveElement(aChildDocument);
  }

  







  void AddDependentIDsFor(nsAccessible* aRelProvider,
                          nsIAtom* aRelAttr = nsnull);

  







  void RemoveDependentIDsFor(nsAccessible* aRelProvider,
                             nsIAtom* aRelAttr = nsnull);

    static void ScrollTimerCallback(nsITimer *aTimer, void *aClosure);

    






    void AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    





    void ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute);

    








    void FireTextChangeEventForText(nsIContent *aContent,
                                    CharacterDataChangeInfo* aInfo,
                                    PRBool aIsInserted);

  



  void FireValueChangeForTextFields(nsAccessible *aAccessible);

  



  enum EUpdateTreeFlags {
    eNoAccessible = 0,
    eAccessible = 1,
    eAlertAccessible = 2
  };

  PRUint32 UpdateTreeInternal(nsAccessible* aContainer,
                              nsIContent* aStartNode,
                              nsIContent* aEndNode,
                              PRBool aIsInsert,
                              PRBool aFireEvents,
                              EIsFromUserInput aFromUserInput);

  


  void UncacheChildrenInSubtree(nsAccessible* aRoot);

  





  void ShutdownChildrenInSubtree(nsAccessible *aAccessible);

  


  nsAccessibleHashtable mAccessibleCache;
  nsDataHashtable<nsPtrHashKey<const nsINode>, nsAccessible*>
    mNodeToAccessibleMap;

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    PRUint16 mScrollPositionChangedTicks; 

protected:

  nsRefPtr<nsAccEventQueue> mEventQueue;

  


  PRPackedBool mIsLoaded;

    static PRUint32 gLastFocusedAccessiblesState;
    static nsIAtom *gLastFocusedFrameType;

  nsTArray<nsRefPtr<nsDocAccessible> > mChildDocuments;

  


  class AttrRelProvider
  {
  public:
    AttrRelProvider(nsIAtom* aRelAttr, nsIContent* aContent) :
      mRelAttr(aRelAttr), mContent(aContent) { }

    nsIAtom* mRelAttr;
    nsIContent* mContent;

  private:
    AttrRelProvider();
    AttrRelProvider(const AttrRelProvider&);
    AttrRelProvider& operator =(const AttrRelProvider&);
  };

  


  typedef nsTArray<nsAutoPtr<AttrRelProvider> > AttrRelProviderArray;
  nsClassHashtable<nsStringHashKey, AttrRelProviderArray> mDependentIDsHash;

  friend class RelatedAccIterator;

  






  nsAccessible* mCacheRoot;
  nsTArray<nsIContent*> mInvalidationList;
  PRBool mIsPostCacheProcessing;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocAccessible,
                              NS_DOCACCESSIBLE_IMPL_CID)

#endif
