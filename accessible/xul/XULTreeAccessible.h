




#ifndef mozilla_a11y_XULTreeAccessible_h__
#define mozilla_a11y_XULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "XULListboxAccessible.h"

class nsTreeBodyFrame;

namespace mozilla {
namespace a11y {

class XULTreeGridCellAccessible;




const uint32_t kMaxTreeColumns = 100;
const uint32_t kDefaultTreeCacheLength = 128;





class XULTreeAccessible : public AccessibleWrap
{
public:
  XULTreeAccessible(nsIContent* aContent, DocAccessible* aDoc,
                    nsTreeBodyFrame* aTreeframe);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeAccessible, Accessible)

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual void Value(nsString& aValue) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) MOZ_OVERRIDE;

  virtual Accessible* GetChildAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual uint32_t ChildCount() const MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;

  
  virtual void SelectedItems(nsTArray<Accessible*>* aItems) MOZ_OVERRIDE;
  virtual uint32_t SelectedItemCount() MOZ_OVERRIDE;
  virtual Accessible* GetSelectedItem(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool IsItemSelected(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool AddItemToSelection(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool RemoveItemFromSelection(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool SelectAll() MOZ_OVERRIDE;
  virtual bool UnselectAll() MOZ_OVERRIDE;

  
  virtual bool IsWidget() const MOZ_OVERRIDE;
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;
  virtual Accessible* CurrentItem() MOZ_OVERRIDE;
  virtual void SetCurrentItem(Accessible* aItem) MOZ_OVERRIDE;

  virtual Accessible* ContainerWidget() const MOZ_OVERRIDE;

  

  





  Accessible* GetTreeItemAccessible(int32_t aRow) const;

  







  void InvalidateCache(int32_t aRow, int32_t aCount);

  








  void TreeViewInvalidated(int32_t aStartRow, int32_t aEndRow,
                           int32_t aStartCol, int32_t aEndCol);

  


  void TreeViewChanged(nsITreeView* aView);

protected:
  virtual ~XULTreeAccessible();

  


  virtual already_AddRefed<Accessible>
    CreateTreeItemAccessible(int32_t aRow) const;

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;
  mutable AccessibleHashtable mAccessibleCache;
};





#define XULTREEITEMBASEACCESSIBLE_IMPL_CID            \
{  /* 1ab79ae7-766a-443c-940b-b1e6b0831dfc */         \
  0x1ab79ae7,                                         \
  0x766a,                                             \
  0x443c,                                             \
  { 0x94, 0x0b, 0xb1, 0xe6, 0xb0, 0x83, 0x1d, 0xfc }  \
}

class XULTreeItemAccessibleBase : public AccessibleWrap
{
public:
  XULTreeItemAccessibleBase(nsIContent* aContent, DocAccessible* aDoc,
                            Accessible* aParent, nsITreeBoxObject* aTree,
                            nsITreeView* aTreeView, int32_t aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeItemAccessibleBase,
                                           AccessibleWrap)

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual nsIntRect Bounds() const MOZ_OVERRIDE;
  virtual GroupPos GroupPosition() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;
  virtual int32_t IndexInParent() const MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;
  virtual Accessible* FocusedChild() MOZ_OVERRIDE;
  virtual void SetSelected(bool aSelect) MOZ_OVERRIDE;
  virtual void TakeFocus() MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual Accessible* ContainerWidget() const MOZ_OVERRIDE;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  


  int32_t GetRowIndex() const { return mRow; }

  



  virtual XULTreeGridCellAccessible* GetCellAccessible(nsITreeColumn* aColumn) const
    { return nullptr; }

  


  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) = 0;

protected:
  virtual ~XULTreeItemAccessibleBase();

  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, uint32_t aActionIndex) MOZ_OVERRIDE;
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const MOZ_OVERRIDE;

  

  


  bool IsExpandable();

  


  void GetCellName(nsITreeColumn* aColumn, nsAString& aName);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;
  int32_t mRow;
};

NS_DEFINE_STATIC_IID_ACCESSOR(XULTreeItemAccessibleBase,
                              XULTREEITEMBASEACCESSIBLE_IMPL_CID)





class XULTreeItemAccessible : public XULTreeItemAccessibleBase
{
public:
  XULTreeItemAccessible(nsIContent* aContent, DocAccessible* aDoc,
                        Accessible* aParent, nsITreeBoxObject* aTree,
                        nsITreeView* aTreeView, int32_t aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeItemAccessible,
                                           XULTreeItemAccessibleBase)

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual ENameValueFlag Name(nsString& aName) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;

  
  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) MOZ_OVERRIDE;

protected:
  virtual ~XULTreeItemAccessible();

  
  virtual void CacheChildren() MOZ_OVERRIDE;

  
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};





class XULTreeColumAccessible : public XULColumAccessible
{
public:
  XULTreeColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

protected:

  
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;
};





inline XULTreeAccessible*
Accessible::AsXULTree()
{
  return IsXULTree() ? static_cast<XULTreeAccessible*>(this) : nullptr;
}

} 
} 

#endif
