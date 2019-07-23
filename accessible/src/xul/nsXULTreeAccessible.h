




































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

class nsXULTreeAccessible : public nsXULSelectableAccessible
{
public:
  nsXULTreeAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeAccessible,
                                           nsAccessible)

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetFirstChild(nsIAccessible **aFirstChild);
  NS_IMETHOD GetLastChild(nsIAccessible **aLastChild);
  NS_IMETHOD GetChildCount(PRInt32 *aChildCount);
  NS_IMETHOD GetChildAt(PRInt32 aChildIndex, nsIAccessible **aChild);

  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  
  NS_DECL_NSIACCESSIBLESELECTABLE

  
  virtual PRBool IsDefunct();
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEACCESSIBLE_IMPL_CID)

  






  void GetTreeItemAccessible(PRInt32 aRow, nsIAccessible **aAccessible);

  







  void InvalidateCache(PRInt32 aRow, PRInt32 aCount);

  








  void TreeViewInvalidated(PRInt32 aStartRow, PRInt32 aEndRow,
                           PRInt32 aStartCol, PRInt32 aEndCol);

  


  void TreeViewChanged();

protected:
  


  virtual void CreateTreeItemAccessible(PRInt32 aRowIndex,
                                        nsAccessNode** aAccessNode);

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  nsAccessNodeHashtable mAccessNodeCache;

  NS_IMETHOD ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);
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
  nsXULTreeItemAccessibleBase(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                              nsIAccessible *aParent, nsITreeBoxObject *aTree,
                              nsITreeView *aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);

  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD SetSelected(PRBool aSelect); 
  NS_IMETHOD TakeFocus();

  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRBool IsDefunct();
  virtual nsresult Shutdown();

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEITEMBASEACCESSIBLE_IMPL_CID)

  



  virtual void GetCellAccessible(nsITreeColumn *aColumn,
                                 nsIAccessible **aCellAcc)
    { *aCellAcc = nsnull; }

  


  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx) = 0;

protected:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  
  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  

  


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
  nsXULTreeItemAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                          nsIAccessible *aParent, nsITreeBoxObject *aTree,
                          nsITreeView *aTreeView, PRInt32 aRow);

  
  NS_IMETHOD GetFirstChild(nsIAccessible **aFirstChild);
  NS_IMETHOD GetLastChild(nsIAccessible **aLastChild);
  NS_IMETHOD GetChildCount(PRInt32 *aChildCount);

  NS_IMETHOD GetName(nsAString& aName);

  
  virtual PRBool IsDefunct();
  virtual nsresult Init();
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};





class nsXULTreeColumnsAccessible : public nsXULColumnsAccessible
{
public:
  nsXULTreeColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
};

#endif
