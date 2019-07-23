





































#ifndef __nsXULTreeGridAccessible_h__
#define __nsXULTreeGridAccessible_h__

#include "nsIAccessibleTable.h"

#include "nsXULTreeAccessible.h"




class nsXULTreeGridAccessible : public nsXULTreeAccessible,
                                public nsIAccessibleTable
{
public:
  nsXULTreeGridAccessible(nsIDOMNode *aDOMNode,
                          nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLE

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:

  
  virtual void CreateTreeItemAccessible(PRInt32 aRow,
                                        nsAccessNode** aAccessNode);
};






class nsXULTreeGridRowAccessible : public nsXULTreeItemAccessibleBase
{
public:
  nsXULTreeGridRowAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                             nsIAccessible *aParent, nsITreeBoxObject *aTree,
                             nsITreeView *aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeGridRowAccessible,
                                           nsAccessible)

  
  NS_IMETHOD GetFirstChild(nsIAccessible **aFirstChild);
  NS_IMETHOD GetLastChild(nsIAccessible **aLastChild);
  NS_IMETHOD GetChildCount(PRInt32 *aChildCount);
  NS_IMETHOD GetChildAt(PRInt32 aChildIndex, nsIAccessible **aChild);

  
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  
  virtual void GetCellAccessible(nsITreeColumn *aColumn, nsIAccessible **aCell);
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:
  nsAccessNodeHashtable mAccessNodeCache;
};







#define NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID         \
{  /* 84588ad4-549c-4196-a932-4c5ca5de5dff */         \
  0x84588ad4,                                         \
  0x549c,                                             \
  0x4196,                                             \
  { 0xa9, 0x32, 0x4c, 0x5c, 0xa5, 0xde, 0x5d, 0xff }  \
}

class nsXULTreeGridCellAccessible : public nsLeafAccessible
{
public:
  nsXULTreeGridCellAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                              nsXULTreeGridRowAccessible *aRowAcc,
                              nsITreeBoxObject *aTree, nsITreeView *aTreeView,
                              PRInt32 aRow, nsITreeColumn* aColumn);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPrevSibling);

  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRBool IsDefunct();
  virtual nsresult Init();

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

  


  PRInt32 GetColumnIndex() const;

  



  void CellInvalidated();

protected:
  
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
