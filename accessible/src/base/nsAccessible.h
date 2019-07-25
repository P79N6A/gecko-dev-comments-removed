





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"
#include "mozilla/a11y/States.h"

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
class KeyBinding;
class nsAccessible;
class nsHyperTextAccessible;
class nsHTMLLIAccessible;
struct nsRoleMapEntry;
class Relation;
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

  


  virtual nsAccessible* FocusedChild();

  


  virtual PRInt32 GetLevelInternal();

  






  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  


  virtual Relation RelationByType(PRUint32 aType);

  
  

  






  virtual void SetRoleMapEntry(nsRoleMapEntry *aRoleMapEntry);

  


  inline bool UpdateChildren()
  {
    InvalidateChildren();
    return EnsureChildren();
  }

  


  bool EnsureChildren();

  





  virtual void InvalidateChildren();

  


  virtual bool AppendChild(nsAccessible* aChild);
  virtual bool InsertChildAt(PRUint32 aIndex, nsAccessible* aChild);
  virtual bool RemoveChild(nsAccessible* aChild);

  
  

  


  nsAccessible* Parent() const { return mParent; }

  


  virtual nsAccessible* GetChildAt(PRUint32 aIndex);

  


  virtual PRInt32 GetChildCount();

  


  virtual PRInt32 GetIndexOf(nsAccessible* aChild);

  


  virtual PRInt32 IndexInParent() const;

  


  bool HasChildren() { return !!GetChildAt(0); }

  


  inline nsAccessible* NextSibling() const
    {  return GetSiblingAtOffset(1); }
  inline nsAccessible* PrevSibling() const
    { return GetSiblingAtOffset(-1); }
  inline nsAccessible* FirstChild()
    { return GetChildCount() != 0 ? GetChildAt(0) : nsnull; }
  inline nsAccessible* LastChild()
  {
    PRUint32 childCount = GetChildCount();
    return childCount != 0 ? GetChildAt(childCount - 1) : nsnull;
  }


  


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

  


  virtual bool GetAllowsAnonChildAccessibles();

  









  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);

  



  void TestChildCache(nsAccessible* aCachedChild) const;

  
  

  inline bool IsAbbreviation() const
  {
    return mContent->IsHTML() &&
      (mContent->Tag() == nsGkAtoms::abbr || mContent->Tag() == nsGkAtoms::acronym);
  }

  inline bool IsApplication() const { return mFlags & eApplicationAccessible; }

  bool IsAutoComplete() const { return mFlags & eAutoCompleteAccessible; }

  inline bool IsAutoCompletePopup() const { return mFlags & eAutoCompletePopupAccessible; }

  inline bool IsCombobox() const { return mFlags & eComboboxAccessible; }

  inline bool IsDoc() const { return mFlags & eDocAccessible; }
  nsDocAccessible* AsDoc();

  inline bool IsHyperText() const { return mFlags & eHyperTextAccessible; }
  nsHyperTextAccessible* AsHyperText();

  inline bool IsHTMLFileInput() const { return mFlags & eHTMLFileInputAccessible; }

  inline bool IsHTMLListItem() const { return mFlags & eHTMLListItemAccessible; }
  nsHTMLLIAccessible* AsHTMLListItem();

  inline bool IsListControl() const { return mFlags & eListControlAccessible; }

  inline bool IsMenuButton() const { return mFlags & eMenuButtonAccessible; }

  inline bool IsMenuPopup() const { return mFlags & eMenuPopupAccessible; }

  inline bool IsRoot() const { return mFlags & eRootAccessible; }
  nsRootAccessible* AsRoot();

  inline bool IsTextLeaf() const { return mFlags & eTextLeafAccessible; }
  nsTextAccessible* AsTextLeaf();

  
  

  


  virtual PRUint8 ActionCount();

  


  virtual KeyBinding AccessKey() const;

  



  virtual KeyBinding KeyboardShortcut() const;

  
  

  


  virtual bool IsLink();

  


  virtual PRUint32 StartOffset();

  


  virtual PRUint32 EndOffset();

  


  inline bool IsLinkValid()
  {
    NS_PRECONDITION(IsLink(), "IsLinkValid is called on not hyper link!");

    
    
    
    
    return (0 == (State() & mozilla::a11y::states::INVALID));
  }

  


  bool IsLinkSelected();

  


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

  
  

  




  virtual bool IsWidget() const;

  


  virtual bool IsActiveWidget() const;

  



  virtual bool AreItemsOperable() const;

  



  virtual nsAccessible* CurrentItem();

  


  virtual void SetCurrentItem(nsAccessible* aItem);

  


  virtual nsAccessible* ContainerWidget() const;

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
    eAutoCompleteAccessible = 1 << 3,
    eAutoCompletePopupAccessible = 1 << 4,
    eComboboxAccessible = 1 << 5,
    eDocAccessible = 1 << 6,
    eHyperTextAccessible = 1 << 7,
    eHTMLFileInputAccessible = 1 << 8,
    eHTMLListItemAccessible = 1 << 9,
    eListControlAccessible = 1 << 10,
    eMenuButtonAccessible = 1 << 11,
    eMenuPopupAccessible = 1 << 12,
    eRootAccessible = 1 << 13,
    eTextLeafAccessible = 1 << 14
  };

  
  

  


  PRUint32 ARIARoleInternal();

  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);

  PRUint64 VisibilityState(); 

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);

  







  nsAccessible *GetFirstAvailableAccessible(nsINode *aStartNode) const;

  
  

  











  void DoCommand(nsIContent *aContent = nsnull, PRUint32 aActionIndex = 0);

  


  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  NS_DECL_RUNNABLEMETHOD_ARG2(nsAccessible, DispatchClickEvent,
                              nsCOMPtr<nsIContent>, PRUint32)

  
  

  



  nsIContent* GetAtomicRegion() const;

  







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






class KeyBinding
{
public:
  


  static const PRUint32 kShift = 1;
  static const PRUint32 kControl = 2;
  static const PRUint32 kAlt = 4;
  static const PRUint32 kMeta = 8;

  KeyBinding() : mKey(0), mModifierMask(0) {}
  KeyBinding(PRUint32 aKey, PRUint32 aModifierMask) :
    mKey(aKey), mModifierMask(aModifierMask) {};

  inline bool IsEmpty() const { return !mKey; }
  inline PRUint32 Key() const { return mKey; }
  inline PRUint32 ModifierMask() const { return mModifierMask; }

  enum Format {
    ePlatformFormat,
    eAtkFormat
  };

  


  inline void ToString(nsAString& aValue,
                       Format aFormat = ePlatformFormat) const
  {
    aValue.Truncate();
    AppendToString(aValue, aFormat);
  }
  inline void AppendToString(nsAString& aValue,
                             Format aFormat = ePlatformFormat) const
  {
    if (mKey) {
      if (aFormat == ePlatformFormat)
        ToPlatformFormat(aValue);
      else
        ToAtkFormat(aValue);
    }
  }

private:
  void ToPlatformFormat(nsAString& aValue) const;
  void ToAtkFormat(nsAString& aValue) const;

  PRUint32 mKey;
  PRUint32 mModifierMask;
};

#endif
