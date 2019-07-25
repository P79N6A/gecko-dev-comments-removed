




#ifndef _Accessible_H_
#define _Accessible_H_

#include "mozilla/a11y/Role.h"
#include "mozilla/a11y/States.h"
#include "nsAccessNodeWrap.h"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperLink.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleValue.h"
#include "nsIAccessibleStates.h"
#include "nsIContent.h"

#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsRefPtrHashtable.h"

class AccEvent;
class AccGroupInfo;
class EmbeddedObjCollector;
class KeyBinding;
class Accessible;
class HyperTextAccessible;
struct nsRoleMapEntry;

namespace mozilla {
namespace a11y {

class HTMLImageMapAccessible;
class HTMLLIAccessible;
class ImageAccessible;
class Relation;
class TableAccessible;
class TextLeafAccessible;
class XULTreeAccessible;




enum ENameValueFlag {
  





 eNameOK,
 eNameFromTooltip 
};




struct GroupPos
{
  GroupPos() : level(0), posInSet(0), setSize(0) { }

  int32_t level;
  int32_t posInSet;
  int32_t setSize;
};

} 
} 

struct nsRect;
class nsIContent;
class nsIFrame;
class nsIAtom;
class nsIView;

typedef nsRefPtrHashtable<nsPtrHashKey<const void>, Accessible>
  AccessibleHashtable;


#define NS_ACCESSIBLE_IMPL_IID                          \
{  /* 133c8bf4-4913-4355-bd50-426bd1d6e1ad */           \
  0x133c8bf4,                                           \
  0x4913,                                               \
  0x4355,                                               \
  { 0xbd, 0x50, 0x42, 0x6b, 0xd1, 0xd6, 0xe1, 0xad }    \
}

class Accessible : public nsAccessNodeWrap,
                   public nsIAccessible,
                   public nsIAccessibleHyperLink,
                   public nsIAccessibleSelectable,
                   public nsIAccessibleValue
{
public:
  Accessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~Accessible();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Accessible, nsAccessNode)

  NS_DECL_NSIACCESSIBLE
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_IID)

  
  

  virtual void Shutdown();

  
  

  


  virtual void Init();

  


  virtual void Description(nsString& aDescription);

  


  virtual void Value(nsString& aValue);

  


  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);

  


  inline already_AddRefed<nsIDOMNode> DOMNode() const
  {
    nsIDOMNode *DOMNode = nullptr;
    if (GetNode())
      CallQueryInterface(GetNode(), &DOMNode);
    return DOMNode;
  }

  


  nsresult GetARIAName(nsAString& aName);

  






  virtual void ApplyARIAState(uint64_t* aState) const;

  









  virtual nsresult GetNameInternal(nsAString& aName);

  


  mozilla::a11y::role Role();

  


  bool HasARIARole() const
    { return mRoleMapEntry; }

  



  mozilla::a11y::role ARIARole();

  



  virtual mozilla::a11y::role NativeRole();

  


  virtual uint64_t State();

  



  uint64_t InteractiveState() const
  {
    uint64_t state = NativeInteractiveState();
    ApplyARIAState(&state);
    return state;
  }

  


  uint64_t LinkState() const
  {
    uint64_t state = NativeLinkState();
    ApplyARIAState(&state);
    return state;
  }

  



  virtual uint64_t NativeState();

  


  virtual uint64_t NativeInteractiveState() const;

  


  virtual uint64_t NativeLinkState() const;

  


  uint64_t VisibilityState();

  


  virtual bool NativelyUnavailable() const;

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  


  virtual mozilla::a11y::GroupPos GroupPosition();

  


  enum EWhichChildAtPoint {
    eDirectChild,
    eDeepestChild
  };

  







  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);

  


  virtual Accessible* FocusedChild();

  


  virtual int32_t GetLevelInternal();

  






  virtual void GetPositionAndSizeInternal(int32_t *aPosInSet,
                                          int32_t *aSetSize);

  


  virtual mozilla::a11y::Relation RelationByType(uint32_t aType);

  
  

  






  virtual void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry);

  


  inline bool UpdateChildren()
  {
    InvalidateChildren();
    return EnsureChildren();
  }

  


  bool EnsureChildren();

  





  virtual void InvalidateChildren();

  


  virtual bool AppendChild(Accessible* aChild);
  virtual bool InsertChildAt(uint32_t aIndex, Accessible* aChild);
  virtual bool RemoveChild(Accessible* aChild);

  
  

  


  Accessible* Parent() const { return mParent; }

  


  virtual Accessible* GetChildAt(uint32_t aIndex);

  


  virtual uint32_t ChildCount() const;

  


  virtual int32_t GetIndexOf(Accessible* aChild);

  


  virtual int32_t IndexInParent() const;

  


  bool HasChildren() { return !!GetChildAt(0); }

  


  inline Accessible* NextSibling() const
    {  return GetSiblingAtOffset(1); }
  inline Accessible* PrevSibling() const
    { return GetSiblingAtOffset(-1); }
  inline Accessible* FirstChild()
    { return GetChildAt(0); }
  inline Accessible* LastChild()
  {
    uint32_t childCount = ChildCount();
    return childCount != 0 ? GetChildAt(childCount - 1) : nullptr;
  }


  


  uint32_t EmbeddedChildCount();

  


  Accessible* GetEmbeddedChildAt(uint32_t aIndex);

  


  int32_t GetIndexOfEmbeddedChild(Accessible* aChild);

  




  uint32_t ContentChildCount() const { return mChildren.Length(); }
  Accessible* ContentChildAt(uint32_t aIndex) const
    { return mChildren.ElementAt(aIndex); }

  


  inline bool AreChildrenCached() const
    { return !IsChildrenFlag(eChildrenUninitialized); }

  


  bool IsBoundToParent() const { return !!mParent; }

  
  

  



  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);

  


  virtual bool CanHaveAnonChildren();

  









  virtual void AppendTextTo(nsAString& aText, uint32_t aStartOffset = 0,
                            uint32_t aLength = PR_UINT32_MAX);

  



  void TestChildCache(Accessible* aCachedChild) const;

  


  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);

  
  

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
  DocAccessible* AsDoc();

  inline bool IsHyperText() const { return mFlags & eHyperTextAccessible; }
  HyperTextAccessible* AsHyperText();

  inline bool IsHTMLFileInput() const { return mFlags & eHTMLFileInputAccessible; }

  inline bool IsHTMLListItem() const { return mFlags & eHTMLListItemAccessible; }
  mozilla::a11y::HTMLLIAccessible* AsHTMLListItem();

  inline bool IsImage() const { return mFlags & eImageAccessible; }
  mozilla::a11y::ImageAccessible* AsImage();

  bool IsImageMapAccessible() const { return mFlags & eImageMapAccessible; }
  mozilla::a11y::HTMLImageMapAccessible* AsImageMap();

  inline bool IsXULTree() const { return mFlags & eXULTreeAccessible; }
  mozilla::a11y::XULTreeAccessible* AsXULTree();

  inline bool IsXULDeck() const { return mFlags & eXULDeckAccessible; }

  inline bool IsListControl() const { return mFlags & eListControlAccessible; }

  inline bool IsMenuButton() const { return mFlags & eMenuButtonAccessible; }

  inline bool IsMenuPopup() const { return mFlags & eMenuPopupAccessible; }

  inline bool IsRoot() const { return mFlags & eRootAccessible; }
  mozilla::a11y::RootAccessible* AsRoot();

  virtual mozilla::a11y::TableAccessible* AsTable() { return nullptr; }

  inline bool IsTextLeaf() const { return mFlags & eTextLeafAccessible; }
  mozilla::a11y::TextLeafAccessible* AsTextLeaf();

  
  

  


  virtual uint8_t ActionCount();

  


  virtual KeyBinding AccessKey() const;

  



  virtual KeyBinding KeyboardShortcut() const;

  
  

  


  virtual bool IsLink();

  


  virtual uint32_t StartOffset();

  


  virtual uint32_t EndOffset();

  


  inline bool IsLinkValid()
  {
    NS_PRECONDITION(IsLink(), "IsLinkValid is called on not hyper link!");

    
    
    
    
    return (0 == (State() & mozilla::a11y::states::INVALID));
  }

  


  bool IsLinkSelected();

  


  virtual uint32_t AnchorCount();

  


  virtual Accessible* AnchorAt(uint32_t aAnchorIndex);

  


  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

  
  

  



  virtual bool IsSelect();

  


  virtual already_AddRefed<nsIArray> SelectedItems();

  


  virtual uint32_t SelectedItemCount();

  


  virtual Accessible* GetSelectedItem(uint32_t aIndex);

  


  virtual bool IsItemSelected(uint32_t aIndex);

  


  virtual bool AddItemToSelection(uint32_t aIndex);

  


  virtual bool RemoveItemFromSelection(uint32_t aIndex);

  


  virtual bool SelectAll();

  


  virtual bool UnselectAll();

  
  

  




  virtual bool IsWidget() const;

  


  virtual bool IsActiveWidget() const;

  



  virtual bool AreItemsOperable() const;

  



  virtual Accessible* CurrentItem();

  


  virtual void SetCurrentItem(Accessible* aItem);

  


  virtual Accessible* ContainerWidget() const;

  


  static void TranslateString(const nsString& aKey, nsAString& aStringOut);

  


  bool IsDefunct() const { return mFlags & eIsDefunct; }

  


  bool IsInDocument() const { return !(mFlags & eIsNotInDocument); }

protected:

  
  

  


  virtual void CacheChildren();

  


  virtual void BindToParent(Accessible* aParent, uint32_t aIndexInParent);
  virtual void UnbindFromParent();

  


  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;

  


  enum ChildrenFlags {
    eChildrenUninitialized = 0, 
    eMixedChildren = 1 << 0, 
    eEmbeddedChildren = 1 << 1 
  };

  


  inline bool IsChildrenFlag(ChildrenFlags aFlag) const
    { return static_cast<ChildrenFlags> (mFlags & kChildrenFlagsMask) == aFlag; }

  


  inline void SetChildrenFlag(ChildrenFlags aFlag)
    { mFlags = (mFlags & ~kChildrenFlagsMask) | aFlag; }

  



  enum StateFlags {
    eIsDefunct = 1 << 2, 
    eIsNotInDocument = 1 << 3 
  };

  



  enum AccessibleTypes {
    eApplicationAccessible = 1 << 4,
    eAutoCompleteAccessible = 1 << 5,
    eAutoCompletePopupAccessible = 1 << 6,
    eComboboxAccessible = 1 << 7,
    eDocAccessible = 1 << 8,
    eHyperTextAccessible = 1 << 9,
    eHTMLFileInputAccessible = 1 << 10,
    eHTMLListItemAccessible = 1 << 11,
    eImageAccessible = 1 << 12,
    eImageMapAccessible = 1 << 13,
    eListControlAccessible = 1 << 14,
    eMenuButtonAccessible = 1 << 15,
    eMenuPopupAccessible = 1 << 16,
    eRootAccessible = 1 << 17,
    eTextLeafAccessible = 1 << 18,
    eXULDeckAccessible = 1 << 19,
    eXULTreeAccessible = 1 << 20
  };

  
  

  


  mozilla::a11y::role ARIATransformRole(mozilla::a11y::role aRole);

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);

  







  Accessible* GetFirstAvailableAccessible(nsINode* aStartNode) const;

  
  

  











  void DoCommand(nsIContent *aContent = nullptr, uint32_t aActionIndex = 0);

  


  virtual void DispatchClickEvent(nsIContent *aContent, uint32_t aActionIndex);

  NS_DECL_RUNNABLEMETHOD_ARG2(Accessible, DispatchClickEvent,
                              nsCOMPtr<nsIContent>, uint32_t)

  
  

  



  nsIContent* GetAtomicRegion() const;

  







  nsresult GetAttrValue(nsIAtom *aAriaProperty, double *aValue);

  



  uint32_t GetActionRule();

  


  AccGroupInfo* GetGroupInfo();

  







  virtual nsresult FirePlatformEvent(AccEvent* aEvent) = 0;

  
  nsRefPtr<Accessible> mParent;
  nsTArray<nsRefPtr<Accessible> > mChildren;
  int32_t mIndexInParent;

  static const uint32_t kChildrenFlagsMask =
    eChildrenUninitialized | eMixedChildren | eEmbeddedChildren;

  uint32_t mFlags;
  friend class DocAccessible;

  nsAutoPtr<EmbeddedObjCollector> mEmbeddedObjCollector;
  int32_t mIndexOfEmbeddedChild;
  friend class EmbeddedObjCollector;

  nsAutoPtr<AccGroupInfo> mGroupInfo;
  friend class AccGroupInfo;

  


  nsRoleMapEntry* mRoleMapEntry;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Accessible,
                              NS_ACCESSIBLE_IMPL_IID)






class KeyBinding
{
public:
  


  static const uint32_t kShift = 1;
  static const uint32_t kControl = 2;
  static const uint32_t kAlt = 4;
  static const uint32_t kMeta = 8;
  static const uint32_t kOS = 16;

  KeyBinding() : mKey(0), mModifierMask(0) {}
  KeyBinding(uint32_t aKey, uint32_t aModifierMask) :
    mKey(aKey), mModifierMask(aModifierMask) {};

  inline bool IsEmpty() const { return !mKey; }
  inline uint32_t Key() const { return mKey; }
  inline uint32_t ModifierMask() const { return mModifierMask; }

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

  uint32_t mKey;
  uint32_t mModifierMask;
};

#endif
