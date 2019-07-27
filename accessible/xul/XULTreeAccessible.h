




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

  
  virtual void Shutdown() override;
  virtual void Value(nsString& aValue) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) override;

  virtual Accessible* GetChildAt(uint32_t aIndex) const override;
  virtual uint32_t ChildCount() const override;
  virtual Relation RelationByType(RelationType aType) override;

  
  virtual void SelectedItems(nsTArray<Accessible*>* aItems) override;
  virtual uint32_t SelectedItemCount() override;
  virtual Accessible* GetSelectedItem(uint32_t aIndex) override;
  virtual bool IsItemSelected(uint32_t aIndex) override;
  virtual bool AddItemToSelection(uint32_t aIndex) override;
  virtual bool RemoveItemFromSelection(uint32_t aIndex) override;
  virtual bool SelectAll() override;
  virtual bool UnselectAll() override;

  
  virtual bool IsWidget() const override;
  virtual bool IsActiveWidget() const override;
  virtual bool AreItemsOperable() const override;
  virtual Accessible* CurrentItem() override;
  virtual void SetCurrentItem(Accessible* aItem) override;

  virtual Accessible* ContainerWidget() const override;

  

  





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

  
  virtual void Shutdown() override;
  virtual nsIntRect Bounds() const override;
  virtual GroupPos GroupPosition() override;
  virtual uint64_t NativeState() override;
  virtual uint64_t NativeInteractiveState() const override;
  virtual int32_t IndexInParent() const override;
  virtual Relation RelationByType(RelationType aType) override;
  virtual Accessible* FocusedChild() override;
  virtual void SetSelected(bool aSelect) override;
  virtual void TakeFocus() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual Accessible* ContainerWidget() const override;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  


  int32_t GetRowIndex() const { return mRow; }

  



  virtual XULTreeGridCellAccessible* GetCellAccessible(nsITreeColumn* aColumn) const
    { return nullptr; }

  


  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) = 0;

protected:
  virtual ~XULTreeItemAccessibleBase();

  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, uint32_t aActionIndex) override;
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const override;

  

  


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

  
  virtual void Shutdown() override;
  virtual ENameValueFlag Name(nsString& aName) override;
  virtual a11y::role NativeRole() override;

  
  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) override;

protected:
  virtual ~XULTreeItemAccessible();

  
  virtual void CacheChildren() override;

  
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};





class XULTreeColumAccessible : public XULColumAccessible
{
public:
  XULTreeColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

protected:

  
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult* aError = nullptr) const
    override;
};





inline XULTreeAccessible*
Accessible::AsXULTree()
{
  return IsXULTree() ? static_cast<XULTreeAccessible*>(this) : nullptr;
}

} 
} 

#endif
