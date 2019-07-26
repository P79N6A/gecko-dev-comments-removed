




#ifndef mozilla_a11y_DocAccessible_h__
#define mozilla_a11y_DocAccessible_h__

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

class DocAccessible : public nsHyperTextAccessibleWrap,
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

  
  virtual bool Init();
  virtual void Shutdown();
  virtual nsIFrame* GetFrame() const;
  virtual nsINode* GetNode() const { return mDocument; }
  virtual nsIDocument* GetDocumentNode() const { return mDocument; }

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual void Description(nsString& aDescription);
  virtual Accessible* FocusedChild();
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual void ApplyARIAState(PRUint64* aState) const;

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
    { return (mLoadState & static_cast<PRUint32>(aState)) == 
        static_cast<PRUint32>(aState); }

  


  virtual void* GetNativeWindow() const;

  


  DocAccessible* ParentDocument() const;

  


  PRUint32 ChildDocumentCount() const
    { return mChildDocuments.Length(); }

  


  DocAccessible* GetChildDocumentAt(PRUint32 aIndex) const
    { return mChildDocuments.SafeElementAt(aIndex, nsnull); }

  






  nsresult FireDelayedAccessibleEvent(PRUint32 aEventType, nsINode *aNode,
                                      AccEvent::EEventRule aAllowDupes = AccEvent::eRemoveDupes,
                                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  




  nsresult FireDelayedAccessibleEvent(AccEvent* aEvent);

  


  void MaybeNotifyOfValueChange(Accessible* aAccessible)
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

  


  Accessible* AnchorJump()
    { return GetAccessibleOrContainer(mAnchorJumpElm); }

  void SetAnchorJump(nsIContent* aTargetNode)
    { mAnchorJumpElm = aTargetNode; }

  


  void BindChildDocument(DocAccessible* aDocument)
  {
    mNotificationController->ScheduleChildDocBinding(aDocument);
  }

  






  template<class Class, class Arg>
  void HandleNotification(Class* aInstance,
                                 typename TNotification<Class, Arg>::Callback aMethod,
                                 Arg* aArg)
  {
    if (mNotificationController) {
      mNotificationController->HandleNotification<Class, Arg>(aInstance,
                                                              aMethod, aArg);
    }
  }

  





  Accessible* GetAccessible(nsINode* aNode) const;

  


  bool HasAccessible(nsINode* aNode) const
    { return GetAccessible(aNode); }

  


  bool IsInDocument(Accessible* aAccessible) const
  {
    Accessible* acc = aAccessible;
    while (acc && !acc->IsPrimaryForNode())
      acc = acc->Parent();

    return acc ? mNodeToAccessibleMap.Get(acc->GetNode()) : false;
  }

  






  Accessible* GetAccessibleByUniqueID(void* aUniqueID)
  {
    return UniqueID() == aUniqueID ?
      this : mAccessibleCache.GetWeak(aUniqueID);
  }

  



  Accessible* GetAccessibleByUniqueIDInSubtree(void* aUniqueID);

  



  Accessible* GetAccessibleOrContainer(nsINode* aNode);

  


  Accessible* GetContainerAccessible(nsINode* aNode)
  {
    return aNode ? GetAccessibleOrContainer(aNode->GetNodeParent()) : nsnull;
  }

  






  bool IsDependentID(const nsAString& aID) const
    { return mDependentIDsHash.Get(aID, nsnull); }

  






  bool BindToDocument(Accessible* aAccessible, nsRoleMapEntry* aRoleMapEntry);

  


  void UnbindFromDocument(Accessible* aAccessible);

  


  void ContentInserted(nsIContent* aContainerNode,
                       nsIContent* aStartChildNode,
                       nsIContent* aEndChildNode);

  


  void ContentRemoved(nsIContent* aContainerNode, nsIContent* aChildNode);

  


  void UpdateText(nsIContent* aTextNode)
  {
    NS_ASSERTION(mNotificationController, "The document was shut down!");

    
    if (mNotificationController && HasLoadState(eTreeConstructed))
      mNotificationController->ScheduleTextUpdate(aTextNode);
  }

  


  void RecreateAccessible(nsIContent* aContent);

protected:

  void LastRelease();

  
  virtual void CacheChildren();

  
  virtual nsresult AddEventListeners();
  virtual nsresult RemoveEventListeners();

  


  void NotifyOfLoad(PRUint32 aLoadEventType)
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
                          nsIAtom* aRelAttr = nsnull);

  







  void RemoveDependentIDsFor(Accessible* aRelProvider,
                             nsIAtom* aRelAttr = nsnull);

  






  bool UpdateAccessibleOnAttrChange(mozilla::dom::Element* aElement,
                                    nsIAtom* aAttribute);

    






    void AttributeChangedImpl(nsIContent* aContent, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    





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

  PRUint32 UpdateTreeInternal(Accessible* aChild, bool aIsInsert);

  


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
    PRUint16 mScrollPositionChangedTicks; 

  


  PRUint32 mLoadState;

  


  PRUint32 mLoadEventType;

  


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

  friend class RelatedAccIterator;

  





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
    static_cast<DocAccessible*>(this) : nsnull;
}

#endif
