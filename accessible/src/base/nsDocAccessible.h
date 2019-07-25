





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsIAccessibleCursorable.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessiblePivot.h"

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
class nsAccessiblePivot;

const PRUint32 kDefaultCacheSize = 256;

class nsDocAccessible : public nsHyperTextAccessibleWrap,
                        public nsIAccessibleDocument,
                        public nsIDocumentObserver,
                        public nsIObserver,
                        public nsIScrollPositionListener,
                        public nsSupportsWeakReference,
                        public nsIAccessibleCursorable,
                        public nsIAccessiblePivotObserver
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDocAccessible, nsAccessible)

  NS_DECL_NSIACCESSIBLEDOCUMENT

  NS_DECL_NSIOBSERVER

  NS_DECL_NSIACCESSIBLECURSORABLE

  NS_DECL_NSIACCESSIBLEPIVOTOBSERVER

public:
  using nsAccessible::GetParent;

  nsDocAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                  nsIPresShell* aPresShell);
  virtual ~nsDocAccessible();

  
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD TakeFocus(void);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual bool Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame() const;
  virtual nsINode* GetNode() const { return mDocument; }
  virtual nsIDocument* GetDocumentNode() const { return mDocument; }

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual void Description(nsString& aDescription);
  virtual nsAccessible* FocusedChild();
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual void ApplyARIAState(PRUint64* aState);

  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);

#ifdef DEBUG_ACCDOCMGR
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);
#endif

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  

  


  nsIPresShell* PresShell() const { return mPresShell; }

  


  nsPresContext* PresContext() const { return mPresShell->GetPresContext(); }
    
  


  bool IsContentLoaded() const
  {
    
    
    
    return mDocument && mDocument->IsVisible() &&
      (mDocument->IsShowing() || HasLoadState(eDOMLoaded));
  }

  


  enum LoadState {
    
    eTreeConstructionPending = 0,
    
    eTreeConstructed = 1,
    
    eDOMLoaded = 1 << 1,
    
    eReady = eTreeConstructed | eDOMLoaded,
    
    eCompletelyLoaded = eReady | 1 << 2
  };

  


  bool HasLoadState(LoadState aState) const
    { return (mLoadState & static_cast<PRUint32>(aState)) == 
        static_cast<PRUint32>(aState); }

  


  virtual void* GetNativeWindow() const;

  


  nsDocAccessible* ParentDocument() const
    { return mParent ? mParent->Document() : nsnull; }

  


  PRUint32 ChildDocumentCount() const
    { return mChildDocuments.Length(); }

  


  nsDocAccessible* GetChildDocumentAt(PRUint32 aIndex) const
    { return mChildDocuments.SafeElementAt(aIndex, nsnull); }

  






  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsINode *aNode,
                                      AccEvent::EEventRule aAllowDupes = AccEvent::eRemoveDupes,
                                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  




  nsresult FireDelayedAccessibleEvent(AccEvent* aEvent);

  


  inline void MaybeNotifyOfValueChange(nsAccessible* aAccessible)
  {
    mozilla::a11y::role role = aAccessible->Role();
    if (role == mozilla::a11y::roles::ENTRY ||
        role == mozilla::a11y::roles::COMBOBOX) {
      nsRefPtr<AccEvent> valueChangeEvent =
        new AccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible,
                     eAutoDetect, AccEvent::eRemoveDupes);
      FireDelayedAccessibleEvent(valueChangeEvent);
    }
  }

  


  inline nsAccessible* AnchorJump()
    { return GetAccessibleOrContainer(mAnchorJumpElm); }

  inline void SetAnchorJump(nsIContent* aTargetNode)
    { mAnchorJumpElm = aTargetNode; }

  


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

  


  inline bool HasAccessible(nsINode* aNode) const
    { return GetAccessible(aNode); }

  


  inline bool IsInDocument(nsAccessible* aAccessible) const
  {
    nsAccessible* acc = aAccessible;
    while (acc && !acc->IsPrimaryForNode())
      acc = acc->Parent();

    return acc ? mNodeToAccessibleMap.Get(acc->GetNode()) : false;
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

  






  bool IsDependentID(const nsAString& aID) const
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

    
    if (mNotificationController && HasLoadState(eTreeConstructed))
      mNotificationController->ScheduleTextUpdate(aTextNode);
  }

  


  void RecreateAccessible(nsIContent* aContent);

protected:

  void LastRelease();

  
  virtual void CacheChildren();

  
    virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
    virtual nsresult AddEventListeners();
    virtual nsresult RemoveEventListeners();

  


  inline void NotifyOfLoad(PRUint32 aLoadEventType)
  {
    mLoadState |= eDOMLoaded;
    mLoadEventType = aLoadEventType;
  }

  void NotifyOfLoading(bool aIsReloading);

  friend class nsAccDocManager;

  



  virtual void DoInitialUpdate();

  



  void ProcessLoad();

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

  


  void ARIAActiveDescendantChanged(nsIContent* aElm);

  



  void ProcessPendingEvent(AccEvent* aEvent);

  


  void ProcessContentInserted(nsAccessible* aContainer,
                              const nsTArray<nsCOMPtr<nsIContent> >* aInsertedContent);

  






  void ProcessInvalidationList();

  


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

  















  bool IsLoadEventTarget() const;

  





  static void ScrollTimerCallback(nsITimer* aTimer, void* aClosure);

protected:

  


  nsAccessibleHashtable mAccessibleCache;
  nsDataHashtable<nsPtrHashKey<const nsINode>, nsAccessible*>
    mNodeToAccessibleMap;

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    PRUint16 mScrollPositionChangedTicks; 

  


  PRUint32 mLoadState;

  


  PRUint32 mLoadEventType;

  


  nsCOMPtr<nsIContent> mAnchorJumpElm;

  



  nsIAtom* mARIAAttrOldValue;

  nsTArray<nsRefPtr<nsDocAccessible> > mChildDocuments;

  


  bool mIsCursorable;

  


  nsRefPtr<nsAccessiblePivot> mVirtualCursor;

  


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

  





  nsTArray<nsIContent*> mInvalidationList;

  


  nsRefPtr<NotificationController> mNotificationController;
  friend class NotificationController;

private:

  nsIPresShell* mPresShell;
};

inline nsDocAccessible*
nsAccessible::AsDoc()
{
  return mFlags & eDocAccessible ?
    static_cast<nsDocAccessible*>(this) : nsnull;
}

#endif
