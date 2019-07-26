



#ifndef __nsXULTreeAccessible_h__
#define __nsXULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "nsXULListboxAccessible.h"




const PRUint32 kMaxTreeColumns = 100;
const PRUint32 kDefaultTreeCacheSize = 256;





class nsXULTreeAccessible : public AccessibleWrap
{
public:
  using Accessible::GetChildAt;

  nsXULTreeAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeAccessible,
                                           Accessible)

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);

  virtual Accessible* GetChildAt(PRUint32 aIndex);
  virtual PRUint32 ChildCount() const;

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual PRUint32 SelectedItemCount();
  virtual Accessible* GetSelectedItem(PRUint32 aIndex);
  virtual bool IsItemSelected(PRUint32 aIndex);
  virtual bool AddItemToSelection(PRUint32 aIndex);
  virtual bool RemoveItemFromSelection(PRUint32 aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual Accessible* CurrentItem();
  virtual void SetCurrentItem(Accessible* aItem);

  virtual Accessible* ContainerWidget() const;

  

  





  Accessible* GetTreeItemAccessible(PRInt32 aRow);

  







  void InvalidateCache(PRInt32 aRow, PRInt32 aCount);

  








  void TreeViewInvalidated(PRInt32 aStartRow, PRInt32 aEndRow,
                           PRInt32 aStartCol, PRInt32 aEndCol);

  


  void TreeViewChanged(nsITreeView* aView);

protected:
  


  virtual already_AddRefed<Accessible> CreateTreeItemAccessible(PRInt32 aRow);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;
  AccessibleHashtable mAccessibleCache;
};





#define NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID         \
{  /* 1ab79ae7-766a-443c-940b-b1e6b0831dfc */         \
  0x1ab79ae7,                                         \
  0x766a,                                             \
  0x443c,                                             \
  { 0x94, 0x0b, 0xb1, 0xe6, 0xb0, 0x83, 0x1d, 0xfc }  \
}

class nsXULTreeItemAccessibleBase : public AccessibleWrap
{
public:
  using Accessible::GetParent;

  nsXULTreeItemAccessibleBase(nsIContent* aContent, DocAccessible* aDoc,
                              Accessible* aParent, nsITreeBoxObject* aTree,
                              nsITreeView* aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeItemAccessibleBase,
                                           AccessibleWrap)

  
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD SetSelected(bool aSelect); 
  NS_IMETHOD TakeFocus();

  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Shutdown();
  virtual bool IsPrimaryForNode() const;

  
  virtual mozilla::a11y::GroupPos GroupPosition();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;
  virtual PRInt32 IndexInParent() const;
  virtual Relation RelationByType(PRUint32 aType);
  virtual Accessible* FocusedChild();

  
  virtual PRUint8 ActionCount();

  
  virtual Accessible* ContainerWidget() const;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  


  PRInt32 GetRowIndex() const { return mRow; }

  



  virtual Accessible* GetCellAccessible(nsITreeColumn* aColumn)
    { return nsnull; }

  


  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx) = 0;

protected:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);
  virtual Accessible* GetSiblingAtOffset(PRInt32 aOffset,
                                         nsresult *aError = nsnull) const;

  

  


  bool IsExpandable();

  


  void GetCellName(nsITreeColumn* aColumn, nsAString& aName);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;
  PRInt32 mRow;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeItemAccessibleBase,
                              NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID)





class nsXULTreeItemAccessible : public nsXULTreeItemAccessibleBase
{
public:
  nsXULTreeItemAccessible(nsIContent* aContent, DocAccessible* aDoc,
                          Accessible* aParent, nsITreeBoxObject* aTree,
                          nsITreeView* aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeItemAccessible,
                                           nsXULTreeItemAccessibleBase)

  
  virtual bool Init();
  virtual void Shutdown();

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual mozilla::a11y::role NativeRole();

  
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:

  
  virtual void CacheChildren();

  
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};





class nsXULTreeColumAccessible : public nsXULColumAccessible
{
public:
  nsXULTreeColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

protected:

  
  virtual Accessible* GetSiblingAtOffset(PRInt32 aOffset,
                                         nsresult *aError = nsnull) const;
};




inline nsXULTreeAccessible*
Accessible::AsXULTree()
{
  return IsXULTree() ?
    static_cast<nsXULTreeAccessible*>(this) : nsnull;
}

#endif
