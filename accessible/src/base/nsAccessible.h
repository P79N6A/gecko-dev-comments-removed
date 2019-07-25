





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"
#include "States.h"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperLink.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleValue.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"

#include "nsARIAMap.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsRefPtrHashtable.h"

class AccEvent;
class AccGroupInfo;
class EmbeddedObjCollector;
class nsAccessible;
class nsHyperTextAccessible;
class nsHTMLLIAccessible;
struct nsRoleMapEntry;
class nsTextAccessible;

struct nsRect;
class nsIContent;
class nsIFrame;
class nsIAtom;
class nsIView;

typedef nsRefPtrHashtable<nsVoidPtrHashKey, nsAccessible>
  nsAccessibleHashtable;


#define NS_OK_NO_ARIA_VALUE \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x21)


#define NS_OK_EMPTY_NAME \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x23)


#define NS_OK_NAME_FROM_TOOLTIP \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x25)


#define NS_ACCESSIBLE_IMPL_IID                          \
{  /* 133c8bf4-4913-4355-bd50-426bd1d6e1ad */           \
  0x133c8bf4,                                           \
  0x4913,                                               \
  0x4355,                                               \
  { 0xbd, 0x50, 0x42, 0x6b, 0xd1, 0xd6, 0xe1, 0xad }    \
}

class nsAccessible : public nsAccessNodeWrap, 
                     public nsIAccessible, 
                     public nsIAccessibleHyperLink,
                     public nsIAccessibleSelectable,
                     public nsIAccessibleValue
{
public:
  nsAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsAccessible();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsAccessible, nsAccessNode)

  NS_DECL_NSIACCESSIBLE
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_IID)

  
  

  virtual void Shutdown();

  
  

  


  virtual void Description(nsString& aDescription);

  


  nsresult GetARIAName(nsAString& aName);

  






  virtual void ApplyARIAState(PRUint64* aState);

  









  virtual nsresult GetNameInternal(nsAString& aName);

  


  inline PRUint32 Role()
  {
    if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
      return NativeRole();

    return ARIARoleInternal();
  }

  



  inline PRUint32 ARIARole()
  {
    if (!mRoleMapEntry || mRoleMapEntry->roleRule != kUseMapRole)
      return nsIAccessibleRole::ROLE_NOTHING;

    return ARIARoleInternal();
  }

  



  virtual PRUint32 NativeRole();

  


  virtual PRUint64 State();

  



  virtual PRUint64 NativeState();

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  


  enum EWhichChildAtPoint {
    eDirectChild,
    eDeepestChild
  };

  







  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);

  


  virtual PRInt32 GetLevelInternal();

  






  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  

  






  virtual void SetRoleMapEntry(nsRoleMapEntry *aRoleMapEntry);

  


  inline bool UpdateChildren()
  {
    InvalidateChildren();
    return EnsureChildren();
  }

  


  bool EnsureChildren();

  





  virtual void InvalidateChildren();

  


  virtual PRBool AppendChild(nsAccessible* aChild);
  virtual PRBool InsertChildAt(PRUint32 aIndex, nsAccessible* aChild);
  virtual PRBool RemoveChild(nsAccessible* aChild);

  
  

  


  nsAccessible* GetParent() const { return mParent; }

  


  virtual nsAccessible* GetChildAt(PRUint32 aIndex);

  


  virtual PRInt32 GetChildCount();

  


  virtual PRInt32 GetIndexOf(nsAccessible* aChild);

  


  virtual PRInt32 IndexInParent() const;

  


  PRBool HasChildren() { return !!GetChildAt(0); }

  


  inline nsAccessible* NextSibling() const
    {  return GetSiblingAtOffset(1); }
  inline nsAccessible* PrevSibling() const
    { return GetSiblingAtOffset(-1); }

  


  PRInt32 GetEmbeddedChildCount();

  


  nsAccessible* GetEmbeddedChildAt(PRUint32 aIndex);

  


  PRInt32 GetIndexOfEmbeddedChild(nsAccessible* aChild);

  




  PRUint32 ContentChildCount() const { return mChildren.Length(); }
  nsAccessible* ContentChildAt(PRUint32 aIndex) const
    { return mChildren.ElementAt(aIndex); }

  


  inline bool AreChildrenCached() const
    { return !IsChildrenFlag(eChildrenUninitialized); }

  


  bool IsBoundToParent() const { return !!mParent; }

  
  

  



  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);

  


  virtual PRBool GetAllowsAnonChildAccessibles();

  









  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);

  



  void TestChildCache(nsAccessible* aCachedChild) const;

  
  

  inline bool IsApplication() const { return mFlags & eApplicationAccessible; }

  inline bool IsDoc() const { return mFlags & eDocAccessible; }
  nsDocAccessible* AsDoc();

  inline bool IsHyperText() const { return mFlags & eHyperTextAccessible; }
  nsHyperTextAccessible* AsHyperText();

  inline bool IsHTMLListItem() const { return mFlags & eHTMLListItemAccessible; }
  nsHTMLLIAccessible* AsHTMLListItem();

  inline bool IsRoot() const { return mFlags & eRootAccessible; }
  nsRootAccessible* AsRoot();

  inline bool IsTextLeaf() const { return mFlags & eTextLeafAccessible; }
  nsTextAccessible* AsTextLeaf();

  
  

  


  virtual bool IsLink();

  


  virtual PRUint32 StartOffset();

  


  virtual PRUint32 EndOffset();

  


  inline bool IsLinkValid()
  {
    NS_PRECONDITION(IsLink(), "IsLinkValid is called on not hyper link!");

    
    
    
    
    return (0 == (State() & states::INVALID));
  }

  


  inline bool IsLinkSelected()
  {
    NS_PRECONDITION(IsLink(),
                    "IsLinkSelected() called on something that is not a hyper link!");
    return gLastFocusedNode == GetNode();
  }

  


  virtual PRUint32 AnchorCount();

  


  virtual nsAccessible* AnchorAt(PRUint32 aAnchorIndex);

  


  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

  
  

  



  virtual bool IsSelect();

  


  virtual already_AddRefed<nsIArray> SelectedItems();

  


  virtual PRUint32 SelectedItemCount();

  


  virtual nsAccessible* GetSelectedItem(PRUint32 aIndex);

  


  virtual bool IsItemSelected(PRUint32 aIndex);

  


  virtual bool AddItemToSelection(PRUint32 aIndex);

  


  virtual bool RemoveItemFromSelection(PRUint32 aIndex);

  


  virtual bool SelectAll();

  


  virtual bool UnselectAll();

protected:

  
  

  


  virtual void CacheChildren();

  


  virtual void BindToParent(nsAccessible* aParent, PRUint32 aIndexInParent);
  void UnbindFromParent();

  


  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull) const;

  


  enum ChildrenFlags {
    eChildrenUninitialized = 0, 
    eMixedChildren = 1 << 0, 
    eEmbeddedChildren = 1 << 1 
  };

  


  inline bool IsChildrenFlag(ChildrenFlags aFlag) const
    { return static_cast<ChildrenFlags> (mFlags & kChildrenFlagsMask) == aFlag; }

  


  inline void SetChildrenFlag(ChildrenFlags aFlag)
    { mFlags = (mFlags & ~kChildrenFlagsMask) | aFlag; }

  



  enum AccessibleTypes {
    eApplicationAccessible = 1 << 2,
    eDocAccessible = 1 << 3,
    eHyperTextAccessible = 1 << 4,
    eHTMLListItemAccessible = 1 << 5,
    eRootAccessible = 1 << 6,
    eTextLeafAccessible = 1 << 7
  };

  
  

  


  PRUint32 ARIARoleInternal();

  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
  PRBool IsVisible(PRBool *aIsOffscreen); 

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);

  







  nsAccessible *GetFirstAvailableAccessible(nsINode *aStartNode) const;

  
  

  











  void DoCommand(nsIContent *aContent = nsnull, PRUint32 aActionIndex = 0);

  


  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  NS_DECL_RUNNABLEMETHOD_ARG2(nsAccessible, DispatchClickEvent,
                              nsCOMPtr<nsIContent>, PRUint32)

  
  

  



  nsIDOMNode* GetAtomicRegion();

  







  nsresult GetAttrValue(nsIAtom *aAriaProperty, double *aValue);

  





  PRUint32 GetActionRule(PRUint64 aStates);

  


  AccGroupInfo* GetGroupInfo();

  







  virtual nsresult FirePlatformEvent(AccEvent* aEvent) = 0;

  
  nsRefPtr<nsAccessible> mParent;
  nsTArray<nsRefPtr<nsAccessible> > mChildren;
  PRInt32 mIndexInParent;

  static const PRUint32 kChildrenFlagsMask =
    eChildrenUninitialized | eMixedChildren | eEmbeddedChildren;

  PRUint32 mFlags;

  nsAutoPtr<EmbeddedObjCollector> mEmbeddedObjCollector;
  PRInt32 mIndexOfEmbeddedChild;
  friend class EmbeddedObjCollector;

  nsAutoPtr<AccGroupInfo> mGroupInfo;
  friend class AccGroupInfo;

  nsRoleMapEntry *mRoleMapEntry; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessible,
                              NS_ACCESSIBLE_IMPL_IID)

#endif
