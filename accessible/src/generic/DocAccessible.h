




#ifndef mozilla_a11y_DocAccessible_h__
#define mozilla_a11y_DocAccessible_h__

#include "nsIAccessibleCursorable.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessiblePivot.h"

#include "AccEvent.h"
#include "HyperTextAccessibleWrap.h"

#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIEditor.h"
#include "nsIObserver.h"
#include "nsIScrollPositionListener.h"
#include "nsITimer.h"
#include "nsIWeakReference.h"
#include "nsIDocShellTreeNode.h"

template<class Class, class Arg>
class TNotification;
class NotificationController;

class nsIScrollableView;
class nsAccessiblePivot;

const uint32_t kDefaultCacheSize = 256;

namespace mozilla {
namespace a11y {

class RelatedAccIterator;

} 
} 

class DocAccessible : public HyperTextAccessibleWrap,
                      public nsIAccessibleDocument,
                      public nsIDocumentObserver,
                      public nsIObserver,
                      public nsIScrollPositionListener,
                      public nsSupportsWeakReference,
                      public nsIAccessibleCursorable,
                      public nsIAccessiblePivotObserver
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DocAccessible, Accessible)

  NS_DECL_NSIACCESSIBLEDOCUMENT

  NS_DECL_NSIOBSERVER

  NS_DECL_NSIACCESSIBLECURSORABLE

  NS_DECL_NSIACCESSIBLEPIVOTOBSERVER

public:

  DocAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                nsIPresShell* aPresShell);
  virtual ~DocAccessible();

  
  NS_IMETHOD GetAttributes(nsIPersistentProperties** aAttributes);
  NS_IMETHOD TakeFocus(void);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual void Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame() const;
  virtual nsINode* GetNode() const { return mDocument; }
  virtual nsIDocument* GetDocumentNode() const { return mDocument; }

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual void Description(nsString& aDescription);
  virtual Accessible* FocusedChild();
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual bool NativelyUnavailable() const;
  virtual void ApplyARIAState(uint64_t* aState) const;

  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);

#ifdef DEBUG
  virtual nsresult HandleAccEvent(AccEvent* aEvent);
#endif

  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);

  
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
    { return (mLoadState & static_cast<uint32_t>(aState)) == 
        static_cast<uint32_t>(aState); }

  


  virtual void* GetNativeWindow() const;

  


  DocAccessible* ParentDocument() const
    { return mParent ? mParent->Document() : nullptr; }

  


  uint32_t ChildDocumentCount() const
    { return mChildDocuments.Length(); }

  


  DocAccessible* GetChildDocumentAt(uint32_t aIndex) const
    { return mChildDocuments.SafeElementAt(aIndex, nullptr); }

  






  nsresult FireDelayedAccessibleEvent(uint32_t aEventType, nsINode *aNode,
                                      AccEvent::EEventRule aAllowDupes = AccEvent::eRemoveDupes,
                                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  




  nsresult FireDelayedAccessibleEvent(AccEvent* aEvent);

  


  void MaybeNotifyOfValueChange(Accessible* aAccessible);

  


  Accessible* AnchorJump()
    { return GetAccessibleOrContainer(mAnchorJumpElm); }

  void SetAnchorJump(nsIContent* aTargetNode)
    { mAnchorJumpElm = aTargetNode; }

  


  void BindChildDocument(DocAccessible* aDocument);

  






  template<class Class, class Arg>
  void HandleNotification(Class* aInstance,
                          typename TNotification<Class, Arg>::Callback aMethod,
                          Arg* aArg);

  





  Accessible* GetAccessible(nsINode* aNode) const;

  


  bool HasAccessible(nsINode* aNode) const
    { return GetAccessible(aNode); }

  






  Accessible* GetAccessibleByUniqueID(void* aUniqueID)
  {
    return UniqueID() == aUniqueID ?
      this : mAccessibleCache.GetWeak(aUniqueID);
  }

  



  Accessible* GetAccessibleByUniqueIDInSubtree(void* aUniqueID);

  



  Accessible* GetAccessibleOrContainer(nsINode* aNode);

  


  Accessible* GetContainerAccessible(nsINode* aNode)
  {
    return aNode ? GetAccessibleOrContainer(aNode->GetNodeParent()) : nullptr;
  }

  






  bool IsDependentID(const nsAString& aID) const
    { return mDependentIDsHash.Get(aID, nullptr); }

  






  bool BindToDocument(Accessible* aAccessible, nsRoleMapEntry* aRoleMapEntry);

  


  void UnbindFromDocument(Accessible* aAccessible);

  


  void ContentInserted(nsIContent* aContainerNode,
                       nsIContent* aStartChildNode,
                       nsIContent* aEndChildNode);

  


  void ContentRemoved(nsIContent* aContainerNode, nsIContent* aChildNode);

  


  void UpdateText(nsIContent* aTextNode);

  


  void RecreateAccessible(nsIContent* aContent);

protected:

  void LastRelease();

  
  virtual void CacheChildren();

  
  virtual nsresult AddEventListeners();
  virtual nsresult RemoveEventListeners();

  


  void NotifyOfLoad(uint32_t aLoadEventType)
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

  



  bool AppendChildDocument(DocAccessible* aChildDocument)
  {
    return mChildDocuments.AppendElement(aChildDocument);
  }

  



  void RemoveChildDocument(DocAccessible* aChildDocument)
  {
    mChildDocuments.RemoveElement(aChildDocument);
  }

  







  void AddDependentIDsFor(Accessible* aRelProvider,
                          nsIAtom* aRelAttr = nullptr);

  







  void RemoveDependentIDsFor(Accessible* aRelProvider,
                             nsIAtom* aRelAttr = nullptr);

  






  bool UpdateAccessibleOnAttrChange(mozilla::dom::Element* aElement,
                                    nsIAtom* aAttribute);

    






    void AttributeChangedImpl(nsIContent* aContent, int32_t aNameSpaceID, nsIAtom* aAttribute);

    





    void ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute);

  


  void ARIAActiveDescendantChanged(nsIContent* aElm);

  



  void ProcessPendingEvent(AccEvent* aEvent);

  


  void ProcessContentInserted(Accessible* aContainer,
                              const nsTArray<nsCOMPtr<nsIContent> >* aInsertedContent);

  






  void ProcessInvalidationList();

  


  void UpdateTree(Accessible* aContainer, nsIContent* aChildNode,
                  bool aIsInsert);

  



  enum EUpdateTreeFlags {
    eNoAccessible = 0,
    eAccessible = 1,
    eAlertAccessible = 2
  };

  uint32_t UpdateTreeInternal(Accessible* aChild, bool aIsInsert);

  


  void CacheChildrenInSubtree(Accessible* aRoot);

  


  void UncacheChildrenInSubtree(Accessible* aRoot);

  





  void ShutdownChildrenInSubtree(Accessible* aAccessible);

  








  bool IsLoadEventTarget() const;

  





  static void ScrollTimerCallback(nsITimer* aTimer, void* aClosure);

protected:

  


  AccessibleHashtable mAccessibleCache;
  nsDataHashtable<nsPtrHashKey<const nsINode>, Accessible*>
    mNodeToAccessibleMap;

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    uint16_t mScrollPositionChangedTicks; 

  


  uint32_t mLoadState;

  


  uint32_t mLoadEventType;

  


  nsCOMPtr<nsIContent> mAnchorJumpElm;

  



  nsIAtom* mARIAAttrOldValue;

  nsTArray<nsRefPtr<DocAccessible> > mChildDocuments;

  


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

  friend class mozilla::a11y::RelatedAccIterator;

  





  nsTArray<nsIContent*> mInvalidationList;

  


  nsRefPtr<NotificationController> mNotificationController;
  friend class NotificationController;

private:

  nsIPresShell* mPresShell;
};

inline DocAccessible*
Accessible::AsDoc()
{
  return mFlags & eDocAccessible ?
    static_cast<DocAccessible*>(this) : nullptr;
}

#endif
