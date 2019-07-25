





































#ifndef __nsXULTreeGridAccessible_h__
#define __nsXULTreeGridAccessible_h__

#include "nsIAccessibleTable.h"

#include "nsXULTreeAccessible.h"




class nsXULTreeGridAccessible : public nsXULTreeAccessible,
                                public nsIAccessibleTable
{
public:
  nsXULTreeGridAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLE

  
  virtual PRUint32 NativeRole();

protected:

  
  virtual already_AddRefed<nsAccessible> CreateTreeItemAccessible(PRInt32 aRow);
};






class nsXULTreeGridRowAccessible : public nsXULTreeItemAccessibleBase
{
public:
  using nsAccessible::GetChildCount;
  using nsAccessible::GetChildAt;
  using nsAccessible::GetChildAtPoint;

  nsXULTreeGridRowAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                             nsAccessible *aParent, nsITreeBoxObject *aTree,
                             nsITreeView *aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeGridRowAccessible,
                                           nsAccessible)

  
  virtual void Shutdown();

  
  virtual PRUint32 NativeRole();
  virtual nsAccessible* GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                        EWhichChildAtPoint aWhichChild);

  virtual nsAccessible* GetChildAt(PRUint32 aIndex);
  virtual PRInt32 GetChildCount();

  
  virtual nsAccessible* GetCellAccessible(nsITreeColumn *aColumn);
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:

  
  virtual void CacheChildren();

  
  nsAccessibleHashtable mAccessibleCache;
};







#define NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID         \
{  /* 84588ad4-549c-4196-a932-4c5ca5de5dff */         \
  0x84588ad4,                                         \
  0x549c,                                             \
  0x4196,                                             \
  { 0xa9, 0x32, 0x4c, 0x5c, 0xa5, 0xde, 0x5d, 0xff }  \
}

class nsXULTreeGridCellAccessible : public nsLeafAccessible,
                                    public nsIAccessibleTableCell
{
public:
  using nsAccessible::GetParent;

  nsXULTreeGridCellAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                              nsXULTreeGridRowAccessible *aRowAcc,
                              nsITreeBoxObject *aTree, nsITreeView *aTreeView,
                              PRInt32 aRow, nsITreeColumn* aColumn);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual bool IsDefunct() const;
  virtual PRBool Init();
  virtual bool IsPrimaryForNode() const;

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual PRInt32 GetIndexInParent() const;

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

  


  PRInt32 GetColumnIndex() const;

  



  void CellInvalidated();

protected:
  
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);
  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  

  


  PRBool IsEditable() const;

  enum { eAction_Click = 0 };

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;

  PRInt32 mRow;
  nsCOMPtr<nsITreeColumn> mColumn;

  nsString mCachedTextEquiv;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeGridCellAccessible,
                              NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

#endif
