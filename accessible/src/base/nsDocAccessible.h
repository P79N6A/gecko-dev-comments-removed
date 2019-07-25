





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsIAccessibleDocument.h"

#include "nsEventShell.h"
#include "nsHyperTextAccessibleWrap.h"
#include "NotificationController.h"

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
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);
  NS_IMETHOD TakeFocus(void);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual PRBool Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame() const;
  virtual PRBool IsDefunct();
  virtual nsINode* GetNode() const { return mDocument; }
  virtual nsIDocument* GetDocumentNode() const { return mDocument; }

  
  virtual void Description(nsString& aDescription);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual void ApplyARIAState(PRUint64* aState);

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

  


  inline void HandleAnchorJump(nsIContent* aTargetNode)
  {
    HandleNotification<nsDocAccessible, nsIContent>
      (this, &nsDocAccessible::ProcessAnchorJump, aTargetNode);
  }

  


  inline void BindChildDocument(nsDocAccessible* aDocument)
  {
    mNotificationController->ScheduleChildDocBinding(aDocument);
  }

  






  template<class Class, class Arg>
  inline void HandleNotification(Class* aInstance,
                                 typename TNotification<Class, Arg>::Callback aMethod,
                                 Arg* aArg)
  {
    if (mNotificationController) {
      mNotificationController->HandleNotification<Class, Arg>(aInstance,
                                                              aMethod, aArg);
    }
  }

  





  nsAccessible* GetAccessible(nsINode* aNode) const;

  


  inline bool HasAccessible(nsINode* aNode)
  {
    return GetAccessible(aNode);
  }

  






  inline nsAccessible* GetAccessibleByUniqueID(void* aUniqueID)
  {
    return UniqueID() == aUniqueID ?
      this : mAccessibleCache.GetWeak(aUniqueID);
  }

  



  nsAccessible* GetAccessibleByUniqueIDInSubtree(void* aUniqueID);

  



  nsAccessible* GetAccessibleOrContainer(nsINode* aNode);

  


  inline nsAccessible* GetContainerAccessible(nsINode* aNode)
  {
    return aNode ? GetAccessibleOrContainer(aNode->GetNodeParent()) : nsnull;
  }

  






  PRBool IsDependentID(const nsAString& aID) const
    { return mDependentIDsHash.Get(aID, nsnull); }

  






  bool BindToDocument(nsAccessible* aAccessible, nsRoleMapEntry* aRoleMapEntry);

  


  void UnbindFromDocument(nsAccessible* aAccessible);

  


  void ContentInserted(nsIContent* aContainerNode,
                       nsIContent* aStartChildNode,
                       nsIContent* aEndChildNode);

  


  void ContentRemoved(nsIContent* aContainerNode, nsIContent* aChildNode);

  


  inline void UpdateText(nsIContent* aTextNode)
  {
    NS_ASSERTION(mNotificationController, "The document was shut down!");

    if (mNotificationController)
      mNotificationController->ScheduleTextUpdate(aTextNode);
  }

  


  void RecreateAccessible(nsIContent* aContent);

  







  void NotifyOfCachingStart(nsAccessible* aAccessible);
  void NotifyOfCachingEnd(nsAccessible* aAccessible);

protected:

  
  virtual void CacheChildren();

  
    virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
    virtual nsresult AddEventListeners();
    virtual nsresult RemoveEventListeners();

  



  virtual void NotifyOfInitialUpdate();

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

  






  bool UpdateAccessibleOnAttrChange(mozilla::dom::Element* aElement,
                                    nsIAtom* aAttribute);

    






    void AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    





    void ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute);

  



  void ProcessPendingEvent(AccEvent* aEvent);

  


  void ProcessAnchorJump(nsIContent* aTargetNode);

  


  void ProcessContentInserted(nsAccessible* aContainer,
                              const nsTArray<nsCOMPtr<nsIContent> >* aInsertedContent);

  


  void UpdateTree(nsAccessible* aContainer, nsIContent* aChildNode,
                  bool aIsInsert);

  



  enum EUpdateTreeFlags {
    eNoAccessible = 0,
    eAccessible = 1,
    eAlertAccessible = 2
  };

  PRUint32 UpdateTreeInternal(nsAccessible* aChild, bool aIsInsert);

  


  void CacheChildrenInSubtree(nsAccessible* aRoot);

  


  void UncacheChildrenInSubtree(nsAccessible* aRoot);

  





  void ShutdownChildrenInSubtree(nsAccessible *aAccessible);

  





  static void ScrollTimerCallback(nsITimer* aTimer, void* aClosure);

  


  nsAccessibleHashtable mAccessibleCache;
  nsDataHashtable<nsPtrHashKey<const nsINode>, nsAccessible*>
    mNodeToAccessibleMap;

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    PRUint16 mScrollPositionChangedTicks; 

protected:

  


  PRPackedBool mIsLoaded;

  static PRUint64 gLastFocusedAccessiblesState;

  nsTArray<nsRefPtr<nsDocAccessible> > mChildDocuments;

  


  class AttrRelProvider
  {
  public:
    AttrRelProvider(nsIAtom* aRelAttr, nsIContent* aContent) :
      mRelAttr(aRelAttr), mContent(aContent) { }

    nsIAtom* mRelAttr;
    nsCOMPtr<nsIContent> mContent;

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

  


  nsRefPtr<NotificationController> mNotificationController;
  friend class NotificationController;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocAccessible,
                              NS_DOCACCESSIBLE_IMPL_CID)

inline nsDocAccessible*
nsAccessible::AsDoc()
{
  return mFlags & eDocAccessible ?
    static_cast<nsDocAccessible*>(this) : nsnull;
}

#endif
