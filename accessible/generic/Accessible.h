




#ifndef _Accessible_H_
#define _Accessible_H_

#include "mozilla/a11y/AccTypes.h"
#include "mozilla/a11y/RelationType.h"
#include "mozilla/a11y/Role.h"
#include "mozilla/a11y/States.h"

#include "nsIContent.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsRefPtrHashtable.h"
#include "nsRect.h"

struct nsRoleMapEntry;

struct nsRect;
class nsIFrame;
class nsIAtom;
class nsIPersistentProperties;

namespace mozilla {
namespace a11y {

class Accessible;
class AccEvent;
class AccGroupInfo;
class ApplicationAccessible;
class DocAccessible;
class EmbeddedObjCollector;
class HTMLImageMapAccessible;
class HTMLLIAccessible;
class HyperTextAccessible;
class ImageAccessible;
class KeyBinding;
class ProxyAccessible;
class Relation;
class RootAccessible;
class TableAccessible;
class TableCellAccessible;
class TextLeafAccessible;
class XULLabelAccessible;
class XULTreeAccessible;




enum ENameValueFlag {
  




 eNameOK,

 



 eNoNameOnPurpose,

 


 eNameFromSubtree,

 


 eNameFromTooltip
};




struct GroupPos
{
  GroupPos() : level(0), posInSet(0), setSize(0) { }

  int32_t level;
  int32_t posInSet;
  int32_t setSize;
};




class index_t
{
public:
  MOZ_IMPLICIT index_t(int32_t aVal) : mVal(aVal) {}

  operator uint32_t() const
  {
    MOZ_ASSERT(mVal >= 0, "Attempt to use wrong index!");
    return mVal;
  }

  bool IsValid() const { return mVal >= 0; }

private:
  int32_t mVal;
};

typedef nsRefPtrHashtable<nsPtrHashKey<const void>, Accessible>
  AccessibleHashtable;


#define NS_ACCESSIBLE_IMPL_IID                          \
{  /* 133c8bf4-4913-4355-bd50-426bd1d6e1ad */           \
  0x133c8bf4,                                           \
  0x4913,                                               \
  0x4355,                                               \
  { 0xbd, 0x50, 0x42, 0x6b, 0xd1, 0xd6, 0xe1, 0xad }    \
}

class Accessible : public nsISupports
{
public:
  Accessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(Accessible)

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_IID)

  
  

  


  DocAccessible* Document() const { return mDoc; }

  


  a11y::RootAccessible* RootAccessible() const;

  


  virtual nsIFrame* GetFrame() const;

  


  virtual nsINode* GetNode() const;
  inline already_AddRefed<nsIDOMNode> DOMNode() const
  {
    nsCOMPtr<nsIDOMNode> DOMNode = do_QueryInterface(GetNode());
    return DOMNode.forget();
  }
  nsIContent* GetContent() const { return mContent; }

  


  bool IsContent() const
    { return GetNode() && GetNode()->IsNodeOfType(nsINode::eCONTENT); }

  


  void* UniqueID() { return static_cast<void*>(this); }

  


  void Language(nsAString& aLocale);

  


  virtual void Description(nsString& aDescription);

  


  virtual void Value(nsString& aValue);

  


  void Help(nsString& aHelp) const { aHelp.Truncate(); }

  





  virtual ENameValueFlag Name(nsString& aName);

  






  virtual void ApplyARIAState(uint64_t* aState) const;

  


  mozilla::a11y::role Role();

  


  bool HasARIARole() const { return mRoleMapEntry; }
  bool IsARIARole(nsIAtom* aARIARole) const;
  bool HasStrongARIARole() const;

  


  nsRoleMapEntry* ARIARoleMap() const { return mRoleMapEntry; }

  



  mozilla::a11y::role ARIARole();

  


  virtual nsIAtom* LandmarkRole() const;

  



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

  


  bool Unavailable() const
  {
    uint64_t state = NativelyUnavailable() ? states::UNAVAILABLE : 0;
    ApplyARIAState(&state);
    return state & states::UNAVAILABLE;
  }

  



  virtual uint64_t NativeState();

  


  virtual uint64_t NativeInteractiveState() const;

  


  virtual uint64_t NativeLinkState() const;

  


  uint64_t VisibilityState();

  


  virtual bool NativelyUnavailable() const;

  


  virtual already_AddRefed<nsIPersistentProperties> Attributes();

  


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

  


  virtual Relation RelationByType(RelationType aType);

  
  

  


  virtual void Shutdown();

  


  void SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
    { mRoleMapEntry = aRoleMapEntry; }

  


  void EnsureChildren();

  





  virtual void InvalidateChildren();

  


  bool AppendChild(Accessible* aChild)
    { return InsertChildAt(mChildren.Length(), aChild); }
  virtual bool InsertChildAt(uint32_t aIndex, Accessible* aChild);
  virtual bool RemoveChild(Accessible* aChild);

  
  

  


  Accessible* Parent() const { return mParent; }

  


  virtual Accessible* GetChildAt(uint32_t aIndex) const;

  


  virtual uint32_t ChildCount() const;

  


  int32_t GetIndexOf(const Accessible* aChild) const
    { return (aChild->mParent != this) ? -1 : aChild->IndexInParent(); }

  


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

  


  virtual bool IsAcceptableChild(Accessible* aPossibleChild) const { return true; }

  









  virtual void AppendTextTo(nsAString& aText, uint32_t aStartOffset = 0,
                            uint32_t aLength = UINT32_MAX);

  



  void TestChildCache(Accessible* aCachedChild) const;

  


  virtual nsIntRect Bounds() const;

  


  virtual nsRect RelativeBounds(nsIFrame** aRelativeFrame) const;

  


  virtual void SetSelected(bool aSelect);

  


  void ExtendSelection() { };

  


  void TakeSelection();

  


  virtual void TakeFocus();

  


  void ScrollTo(uint32_t aHow) const;

  


  void ScrollToPoint(uint32_t aCoordinateType, int32_t aX, int32_t aY);

  



  virtual void GetNativeInterface(void** aNativeAccessible);

  
  

  inline bool IsAbbreviation() const
  {
    return mContent->IsAnyOfHTMLElements(nsGkAtoms::abbr, nsGkAtoms::acronym);
  }

  bool IsApplication() const { return mType == eApplicationType; }
  ApplicationAccessible* AsApplication();

  bool IsAutoComplete() const { return HasGenericType(eAutoComplete); }

  bool IsAutoCompletePopup() const
    { return HasGenericType(eAutoCompletePopup); }

  bool IsButton() const { return HasGenericType(eButton); }

  bool IsCombobox() const { return HasGenericType(eCombobox); }

  bool IsDoc() const { return HasGenericType(eDocument); }
  DocAccessible* AsDoc();

  bool IsGenericHyperText() const { return mType == eHyperTextType; }
  bool IsHyperText() const { return HasGenericType(eHyperText); }
  HyperTextAccessible* AsHyperText();

  bool IsHTMLBr() const { return mType == eHTMLBRType; }
  bool IsHTMLCombobox() const { return mType == eHTMLComboboxType; }
  bool IsHTMLFileInput() const { return mType == eHTMLFileInputType; }

  bool IsHTMLListItem() const { return mType == eHTMLLiType; }
  HTMLLIAccessible* AsHTMLListItem();

  bool IsHTMLOptGroup() const { return mType == eHTMLOptGroupType; }

  bool IsHTMLTable() const { return mType == eHTMLTableType; }
  bool IsHTMLTableRow() const { return mType == eHTMLTableRowType; }

  bool IsImage() const { return mType == eImageType; }
  ImageAccessible* AsImage();

  bool IsImageMap() const { return mType == eImageMapType; }
  HTMLImageMapAccessible* AsImageMap();

  bool IsList() const { return HasGenericType(eList); }

  bool IsListControl() const { return HasGenericType(eListControl); }

  bool IsMenuButton() const { return HasGenericType(eMenuButton); }

  bool IsMenuPopup() const { return mType == eMenuPopupType; }

  bool IsProxy() const { return mType == eProxyType; }
  ProxyAccessible* Proxy() const
  {
    MOZ_ASSERT(IsProxy());
    return mBits.proxy;
  }

  bool IsProgress() const { return mType == eProgressType; }

  bool IsRoot() const { return mType == eRootType; }
  a11y::RootAccessible* AsRoot();

  bool IsSearchbox() const;

  bool IsSelect() const { return HasGenericType(eSelect); }

  bool IsTable() const { return HasGenericType(eTable); }
  virtual TableAccessible* AsTable() { return nullptr; }

  bool IsTableCell() const { return HasGenericType(eTableCell); }
  virtual TableCellAccessible* AsTableCell() { return nullptr; }
  const TableCellAccessible* AsTableCell() const
    { return const_cast<Accessible*>(this)->AsTableCell(); }

  bool IsTableRow() const { return HasGenericType(eTableRow); }

  bool IsTextField() const { return mType == eHTMLTextFieldType; }

  bool IsTextLeaf() const { return mType == eTextLeafType; }
  TextLeafAccessible* AsTextLeaf();

  bool IsXULLabel() const { return mType == eXULLabelType; }
  XULLabelAccessible* AsXULLabel();

  bool IsXULListItem() const { return mType == eXULListItemType; }

  bool IsXULTabpanels() const { return mType == eXULTabpanelsType; }

  bool IsXULTree() const { return mType == eXULTreeType; }
  XULTreeAccessible* AsXULTree();

  


  bool HasGenericType(AccGenericType aType) const;

  
  

  


  virtual uint8_t ActionCount();

  


  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName);

  


  void ActionDescriptionAt(uint8_t aIndex, nsAString& aDescription)
  {
    nsAutoString name;
    ActionNameAt(aIndex, name);
    TranslateString(name, aDescription);
  }

  


  virtual bool DoAction(uint8_t aIndex);

  


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

  


  virtual uint32_t AnchorCount();

  


  virtual Accessible* AnchorAt(uint32_t aAnchorIndex);

  


  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

  
  

  


  virtual void SelectedItems(nsTArray<Accessible*>* aItems);

  


  virtual uint32_t SelectedItemCount();

  


  virtual Accessible* GetSelectedItem(uint32_t aIndex);

  


  virtual bool IsItemSelected(uint32_t aIndex);

  


  virtual bool AddItemToSelection(uint32_t aIndex);

  


  virtual bool RemoveItemFromSelection(uint32_t aIndex);

  


  virtual bool SelectAll();

  


  virtual bool UnselectAll();

  
  

  virtual double MaxValue() const;
  virtual double MinValue() const;
  virtual double CurValue() const;
  virtual double Step() const;
  virtual bool SetCurValue(double aValue);

  
  

  




  virtual bool IsWidget() const;

  


  virtual bool IsActiveWidget() const;

  



  virtual bool AreItemsOperable() const;

  



  virtual Accessible* CurrentItem();

  


  virtual void SetCurrentItem(Accessible* aItem);

  


  virtual Accessible* ContainerWidget() const;

  


  static void TranslateString(const nsString& aKey, nsAString& aStringOut);

  


  bool IsDefunct() const { return mStateFlags & eIsDefunct; }

  


  bool IsInDocument() const { return !(mStateFlags & eIsNotInDocument); }

  


  bool IsNodeMapEntry() const
    { return HasOwnContent() && !(mStateFlags & eNotNodeMapEntry); }

  


  inline bool HasDirtyGroupInfo() const { return mStateFlags & eGroupInfoDirty; }

  


  bool HasOwnContent() const
    { return mContent && !(mStateFlags & eSharedNode); }

  


  bool HasNumericValue() const;

  





  bool NeedsDOMUIEvent() const
    { return !(mStateFlags & eIgnoreDOMUIEvent); }

  



  bool IsSurvivingInUpdate() const { return mStateFlags & eSurvivingInUpdate; }
  void SetSurvivingInUpdate(bool aIsSurviving)
  {
    if (aIsSurviving)
      mStateFlags |= eSurvivingInUpdate;
    else
      mStateFlags &= ~eSurvivingInUpdate;
  }

  



  bool HasNameDependentParent() const
    { return mContextFlags & eHasNameDependentParent; }

  



  bool IsARIAHidden() const { return mContextFlags & eARIAHidden; }
  void SetARIAHidden(bool aIsDefined);

protected:

  virtual ~Accessible();

  



  virtual mozilla::a11y::ENameValueFlag NativeName(nsString& aName);

  



  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes();

  
  

  


  void LastRelease();

  


  virtual void CacheChildren();

  


  virtual void BindToParent(Accessible* aParent, uint32_t aIndexInParent);
  virtual void UnbindFromParent();

  


  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;

  


  enum ChildrenFlags {
    eChildrenUninitialized = 0, 
    eMixedChildren = 1 << 0, 
    eEmbeddedChildren = 1 << 1, 

    eLastChildrenFlag = eEmbeddedChildren
  };

  


  bool IsChildrenFlag(ChildrenFlags aFlag) const
    { return static_cast<ChildrenFlags>(mChildrenFlags) == aFlag; }

  


  void SetChildrenFlag(ChildrenFlags aFlag) { mChildrenFlags = aFlag; }

  



  enum StateFlags {
    eIsDefunct = 1 << 0, 
    eIsNotInDocument = 1 << 1, 
    eSharedNode = 1 << 2, 
    eNotNodeMapEntry = 1 << 3, 
    eHasNumericValue = 1 << 4, 
    eGroupInfoDirty = 1 << 5, 
    eSubtreeMutating = 1 << 6, 
    eIgnoreDOMUIEvent = 1 << 7, 
    eSurvivingInUpdate = 1 << 8, 

    eLastStateFlag = eSurvivingInUpdate
  };

  


  enum ContextFlags {
    eHasNameDependentParent = 1 << 0, 
    eARIAHidden = 1 << 1,

    eLastContextFlag = eARIAHidden
  };

protected:

  
  

  


  mozilla::a11y::role ARIATransformRole(mozilla::a11y::role aRole);

  
  

  


  void ARIAName(nsString& aName);

  


  static void XULElmName(DocAccessible* aDocument,
                         nsIContent* aElm, nsString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);

  
  

  











  void DoCommand(nsIContent *aContent = nullptr, uint32_t aActionIndex = 0);

  


  virtual void DispatchClickEvent(nsIContent *aContent, uint32_t aActionIndex);

  
  

  



  nsIContent* GetAtomicRegion() const;

  





  double AttrNumericValue(nsIAtom* aARIAAttr) const;

  



  uint32_t GetActionRule() const;

  


  AccGroupInfo* GetGroupInfo();

  


  inline void SetDirtyGroupInfo(bool aIsDirty)
  {
    if (aIsDirty)
      mStateFlags |= eGroupInfoDirty;
    else
      mStateFlags &= ~eGroupInfoDirty;
  }

  


  void InvalidateChildrenGroupInfo();

  
  nsCOMPtr<nsIContent> mContent;
  DocAccessible* mDoc;

  nsRefPtr<Accessible> mParent;
  nsTArray<nsRefPtr<Accessible> > mChildren;
  int32_t mIndexInParent;

  static const uint8_t kChildrenFlagsBits = 2;
  static const uint8_t kStateFlagsBits = 9;
  static const uint8_t kContextFlagsBits = 2;
  static const uint8_t kTypeBits = 6;
  static const uint8_t kGenericTypesBits = 14;

  


  uint32_t mChildrenFlags : kChildrenFlagsBits;
  uint32_t mStateFlags : kStateFlagsBits;
  uint32_t mContextFlags : kContextFlagsBits;
  uint32_t mType : kTypeBits;
  uint32_t mGenericTypes : kGenericTypesBits;

  void StaticAsserts() const;
  void AssertInMutatingSubtree() const;

  friend class DocAccessible;
  friend class xpcAccessible;
  friend class AutoTreeMutation;

  nsAutoPtr<mozilla::a11y::EmbeddedObjCollector> mEmbeddedObjCollector;
  int32_t mIndexOfEmbeddedChild;
  friend class EmbeddedObjCollector;

  union
  {
    AccGroupInfo* groupInfo;
    ProxyAccessible* proxy;
  } mBits;
  friend class AccGroupInfo;

  


  nsRoleMapEntry* mRoleMapEntry;

private:
  Accessible() = delete;
  Accessible(const Accessible&) = delete;
  Accessible& operator =(const Accessible&) = delete;

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

  static uint32_t AccelModifier();

  KeyBinding() : mKey(0), mModifierMask(0) {}
  KeyBinding(uint32_t aKey, uint32_t aModifierMask) :
    mKey(aKey), mModifierMask(aModifierMask) {}

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







class AutoTreeMutation
{
public:
  explicit AutoTreeMutation(Accessible* aRoot, bool aInvalidationRequired = true) :
    mInvalidationRequired(aInvalidationRequired), mRoot(aRoot)
  {
    MOZ_ASSERT(!(mRoot->mStateFlags & Accessible::eSubtreeMutating));
    mRoot->mStateFlags |= Accessible::eSubtreeMutating;
  }
  ~AutoTreeMutation()
  {
    if (mInvalidationRequired)
      mRoot->InvalidateChildrenGroupInfo();

    MOZ_ASSERT(mRoot->mStateFlags & Accessible::eSubtreeMutating);
    mRoot->mStateFlags &= ~Accessible::eSubtreeMutating;
  }

  bool mInvalidationRequired;
private:
  Accessible* mRoot;
};

} 
} 

#endif
