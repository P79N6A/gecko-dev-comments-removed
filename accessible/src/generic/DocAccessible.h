




#ifndef mozilla_a11y_DocAccessible_h__
#define mozilla_a11y_DocAccessible_h__

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

class nsAccessiblePivot;

class nsIScrollableView;

const uint32_t kDefaultCacheSize = 256;

namespace mozilla {
namespace a11y {

class DocManager;
class NotificationController;
class RelatedAccIterator;
template<class Class, class Arg>
class TNotification;

class DocAccessible : public HyperTextAccessibleWrap,
                      public nsIAccessibleDocument,
                      public nsIDocumentObserver,
                      public nsIObserver,
                      public nsIScrollPositionListener,
                      public nsSupportsWeakReference,
                      public nsIAccessiblePivotObserver
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DocAccessible, Accessible)

  NS_DECL_NSIACCESSIBLEDOCUMENT

  NS_DECL_NSIOBSERVER

  NS_DECL_NSIACCESSIBLEPIVOTOBSERVER

public:

  DocAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                nsIPresShell* aPresShell);
  virtual ~DocAccessible();

  
  NS_IMETHOD TakeFocus(void);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) {}
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);

  
  NS_DECL_NSIDOCUMENTOBSERVER

  
  virtual void Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame() const;
  virtual nsINode* GetNode() const { return mDocumentNode; }
  nsIDocument* DocumentNode() const { return mDocumentNode; }

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual void Description(nsString& aDescription);
  virtual Accessible* FocusedChild();
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual bool NativelyUnavailable() const;
  virtual void ApplyARIAState(uint64_t* aState) const;
  virtual already_AddRefed<nsIPersistentProperties> Attributes();

#ifdef A11Y_LOG
  virtual nsresult HandleAccEvent(AccEvent* aEvent);
#endif

  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  

  


  nsIPresShell* PresShell() const { return mPresShell; }

  


  nsPresContext* PresContext() const { return mPresShell->GetPresContext(); }
    
  


  bool IsContentLoaded() const
  {
    
    
    
    return mDocumentNode && mDocumentNode->IsVisible() &&
      (mDocumentNode->IsShowing() || HasLoadState(eDOMLoaded));
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

  


  void FireDelayedEvent(AccEvent* aEvent);
  void FireDelayedEvent(uint32_t aEventType, Accessible* aTarget);

  


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

  



  Accessible* GetAccessibleOrContainer(nsINode* aNode) const;

  


  Accessible* GetContainerAccessible(nsINode* aNode) const
  {
    return aNode ? GetAccessibleOrContainer(aNode->GetParentNode()) : nullptr;
  }

  


  Accessible* GetAccessibleOrDescendant(nsINode* aNode) const;

  






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

  


  void NotifyOfLoad(uint32_t aLoadEventType);
  void NotifyOfLoading(bool aIsReloading);

  friend class DocManager;

  



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

  







  void AddDependentIDsFor(dom::Element* aRelProviderElm,
                          nsIAtom* aRelAttr = nullptr);

  







  void RemoveDependentIDsFor(dom::Element* aRelProviderElm,
                             nsIAtom* aRelAttr = nullptr);

  






  bool UpdateAccessibleOnAttrChange(mozilla::dom::Element* aElement,
                                    nsIAtom* aAttribute);

  






  void AttributeChangedImpl(Accessible* aAccessible,
                            int32_t aNameSpaceID, nsIAtom* aAttribute);

  





  void ARIAAttributeChanged(Accessible* aAccessible, nsIAtom* aAttribute);

  


  void ARIAActiveDescendantChanged(Accessible* aAccessible);

  


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

  uint32_t UpdateTreeInternal(Accessible* aChild, bool aIsInsert,
                              AccReorderEvent* aReorderEvent);

  






  void CacheChildrenInSubtree(Accessible* aRoot,
                              Accessible** aFocusedAcc = nullptr);

  


  void UncacheChildrenInSubtree(Accessible* aRoot);

  





  void ShutdownChildrenInSubtree(Accessible* aAccessible);

  








  bool IsLoadEventTarget() const;

  





  static void ScrollTimerCallback(nsITimer* aTimer, void* aClosure);

protected:

  


  enum {
    
    eScrollInitialized = 1 << 0,

    
    eTabDocument = 1 << 1
  };

  


  AccessibleHashtable mAccessibleCache;
  nsDataHashtable<nsPtrHashKey<const nsINode>, Accessible*>
    mNodeToAccessibleMap;

  nsIDocument* mDocumentNode;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    uint16_t mScrollPositionChangedTicks; 

  


  uint32_t mLoadState : 3;

  


  uint32_t mDocFlags : 28;

  


  uint32_t mLoadEventType;

  


  nsCOMPtr<nsIContent> mAnchorJumpElm;

  



  nsIAtom* mARIAAttrOldValue;

  nsTArray<nsRefPtr<DocAccessible> > mChildDocuments;

  


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
  typedef nsClassHashtable<nsStringHashKey, AttrRelProviderArray>
    DependentIDsHashtable;

  


  DependentIDsHashtable mDependentIDsHash;

  static PLDHashOperator
    CycleCollectorTraverseDepIDsEntry(const nsAString& aKey,
                                      AttrRelProviderArray* aProviders,
                                      void* aUserArg);

  friend class RelatedAccIterator;

  





  nsTArray<nsIContent*> mInvalidationList;

  


  nsRefPtr<NotificationController> mNotificationController;
  friend class EventQueue;
  friend class NotificationController;

private:

  nsIPresShell* mPresShell;
};

inline DocAccessible*
Accessible::AsDoc()
{
  return IsDoc() ? static_cast<DocAccessible*>(this) : nullptr;
}

} 
} 

#endif
