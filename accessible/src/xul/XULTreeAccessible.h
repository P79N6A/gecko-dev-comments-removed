




#ifndef mozilla_a11y_XULTreeAccessible_h__
#define mozilla_a11y_XULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "XULListboxAccessible.h"

namespace mozilla {
namespace a11y {




const uint32_t kMaxTreeColumns = 100;
const uint32_t kDefaultTreeCacheSize = 256;





class XULTreeAccessible : public AccessibleWrap
{
public:
  using Accessible::GetChildAt;

  XULTreeAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeAccessible, Accessible)

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);

  virtual Accessible* GetChildAt(uint32_t aIndex);
  virtual uint32_t ChildCount() const;
  virtual Relation RelationByType(uint32_t aType);

  
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

  

  





  Accessible* GetTreeItemAccessible(int32_t aRow);

  







  void InvalidateCache(int32_t aRow, int32_t aCount);

  








  void TreeViewInvalidated(int32_t aStartRow, int32_t aEndRow,
                           int32_t aStartCol, int32_t aEndCol);

  


  void TreeViewChanged(nsITreeView* aView);

protected:
  


  virtual already_AddRefed<Accessible> CreateTreeItemAccessible(int32_t aRow);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;
  AccessibleHashtable mAccessibleCache;
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
  using Accessible::GetParent;

  XULTreeItemAccessibleBase(nsIContent* aContent, DocAccessible* aDoc,
                            Accessible* aParent, nsITreeBoxObject* aTree,
                            nsITreeView* aTreeView, int32_t aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeItemAccessibleBase,
                                           AccessibleWrap)

  
  NS_IMETHOD GetBounds(int32_t *aX, int32_t *aY,
                       int32_t *aWidth, int32_t *aHeight);

  NS_IMETHOD SetSelected(bool aSelect);
  NS_IMETHOD TakeFocus();

  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Shutdown();

  
  virtual GroupPos GroupPosition();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual int32_t IndexInParent() const;
  virtual Relation RelationByType(uint32_t aType);
  virtual Accessible* FocusedChild();

  
  virtual uint8_t ActionCount();

  
  virtual Accessible* ContainerWidget() const;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  


  int32_t GetRowIndex() const { return mRow; }

  



  virtual Accessible* GetCellAccessible(nsITreeColumn* aColumn)
    { return nullptr; }

  


  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) = 0;

protected:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, uint32_t aActionIndex);
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;

  

  


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

  
  virtual void Shutdown();

  
  virtual ENameValueFlag Name(nsString& aName);
  virtual a11y::role NativeRole();

  
  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx);

protected:

  
  virtual void CacheChildren();

  
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
