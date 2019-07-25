




































#ifndef __nsXULTreeAccessible_h__
#define __nsXULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "nsXULListboxAccessible.h"




const PRUint32 kMaxTreeColumns = 100;
const PRUint32 kDefaultTreeCacheSize = 256;





#define NS_XULTREEACCESSIBLE_IMPL_CID                   \
{  /* 2692e149-6176-42ee-b8e1-2c44b04185e3 */           \
  0x2692e149,                                           \
  0x6176,                                               \
  0x42ee,                                               \
  { 0xb8, 0xe1, 0x2c, 0x44, 0xb0, 0x41, 0x85, 0xe3 }    \
}

class nsXULTreeAccessible : public nsAccessibleWrap
{
public:
  using nsAccessible::GetChildCount;
  using nsAccessible::GetChildAt;
  using nsAccessible::GetChildAtPoint;

  nsXULTreeAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeAccessible,
                                           nsAccessible)

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  
  virtual bool IsDefunct() const;
  virtual void Shutdown();

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual nsAccessible* GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                        EWhichChildAtPoint aWhichChild);

  virtual nsAccessible* GetChildAt(PRUint32 aIndex);
  virtual PRInt32 GetChildCount();

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual PRUint32 SelectedItemCount();
  virtual nsAccessible* GetSelectedItem(PRUint32 aIndex);
  virtual bool IsItemSelected(PRUint32 aIndex);
  virtual bool AddItemToSelection(PRUint32 aIndex);
  virtual bool RemoveItemFromSelection(PRUint32 aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

  

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEACCESSIBLE_IMPL_CID)

  





  nsAccessible* GetTreeItemAccessible(PRInt32 aRow);

  







  void InvalidateCache(PRInt32 aRow, PRInt32 aCount);

  








  void TreeViewInvalidated(PRInt32 aStartRow, PRInt32 aEndRow,
                           PRInt32 aStartCol, PRInt32 aEndCol);

  


  void TreeViewChanged();

protected:
  


  virtual already_AddRefed<nsAccessible> CreateTreeItemAccessible(PRInt32 aRow);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  nsAccessibleHashtable mAccessibleCache;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeAccessible,
                              NS_XULTREEACCESSIBLE_IMPL_CID)





#define NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID         \
{  /* 1ab79ae7-766a-443c-940b-b1e6b0831dfc */         \
  0x1ab79ae7,                                         \
  0x766a,                                             \
  0x443c,                                             \
  { 0x94, 0x0b, 0xb1, 0xe6, 0xb0, 0x83, 0x1d, 0xfc }  \
}

class nsXULTreeItemAccessibleBase : public nsAccessibleWrap
{
public:
  using nsAccessible::GetParent;

  nsXULTreeItemAccessibleBase(nsIContent *aContent, nsIWeakReference *aShell,
                              nsAccessible *aParent, nsITreeBoxObject *aTree,
                              nsITreeView *aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD SetSelected(PRBool aSelect); 
  NS_IMETHOD TakeFocus();

  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  NS_IMETHOD GroupPosition(PRInt32 *aGroupLevel,
                           PRInt32 *aSimilarItemsInGroup,
                           PRInt32 *aPositionInGroup);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual bool IsDefunct() const;
  virtual void Shutdown();
  virtual bool IsPrimaryForNode() const;

  
  virtual PRUint64 NativeState();
  virtual PRInt32 GetIndexInParent() const;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  


  PRInt32 GetRowIndex() const { return mRow; }

  



  virtual nsAccessible* GetCellAccessible(nsITreeColumn *aColumn)
    { return nsnull; }

  


  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx) = 0;

protected:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);

  

  


  PRBool IsExpandable();

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  PRInt32 mRow;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeItemAccessibleBase,
                              NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID)





class nsXULTreeItemAccessible : public nsXULTreeItemAccessibleBase
{
public:
  nsXULTreeItemAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                          nsAccessible *aParent, nsITreeBoxObject *aTree,
                          nsITreeView *aTreeView, PRInt32 aRow);

  NS_IMETHOD GetName(nsAString& aName);

  
  virtual bool IsDefunct() const;
  virtual PRBool Init();
  virtual void Shutdown();

  
  virtual PRUint32 NativeRole();

  
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:

  
  virtual void CacheChildren();

  
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};





class nsXULTreeColumnsAccessible : public nsXULColumnsAccessible
{
public:
  nsXULTreeColumnsAccessible(nsIContent *aContent, nsIWeakReference *aShell);

protected:

  
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);
};

#endif
