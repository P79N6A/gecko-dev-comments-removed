




































#ifndef __nsXULTreeAccessible_h__
#define __nsXULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "nsXULSelectAccessible.h"




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
  virtual ~nsXULTreeAccessible() {}

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLESELECTABLE

  
  NS_IMETHOD GetValue(nsAString& _retval);

  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  
  virtual PRBool IsDefunct();
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEACCESSIBLE_IMPL_CID)

  








  void GetCachedTreeitemAccessible(PRInt32 aRow, nsITreeColumn *aColumn,
                                   nsIAccessible **aAccessible);

  







  void InvalidateCache(PRInt32 aRow, PRInt32 aCount);

  








  void TreeViewInvalidated(PRInt32 aStartRow, PRInt32 aEndRow,
                           PRInt32 aStartCol, PRInt32 aEndCol);

  


  void TreeViewChanged();

  static void GetTreeBoxObject(nsIDOMNode* aDOMNode, nsITreeBoxObject** aBoxObject);
  static nsresult GetColumnCount(nsITreeBoxObject* aBoxObject, PRInt32 *aCount);

  static PRBool IsColumnHidden(nsITreeColumn *aColumn);
  static already_AddRefed<nsITreeColumn> GetNextVisibleColumn(nsITreeColumn *aColumn);
  static already_AddRefed<nsITreeColumn> GetFirstVisibleColumn(nsITreeBoxObject *aTree);
  static already_AddRefed<nsITreeColumn> GetLastVisibleColumn(nsITreeBoxObject *aTree);

protected:
  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  nsAccessNodeHashtable *mAccessNodeCache;

  NS_IMETHOD ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeAccessible,
                              NS_XULTREEACCESSIBLE_IMPL_CID)





#define NS_XULTREEITEMACCESSIBLE_IMPL_CID             \
{  /* 7b1aa039-7270-4523-aeb3-61063a13ac3f */         \
  0x7b1aa039,                                         \
  0x7270,                                             \
  0x4523,                                             \
  { 0xae, 0xb3, 0x61, 0x06, 0x3a, 0x13, 0xac, 0x3f }  \
}

class nsXULTreeitemAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  nsXULTreeitemAccessible(nsIAccessible *aParent, nsIDOMNode *aDOMNode, nsIWeakReference *aShell, PRInt32 aRow, nsITreeColumn* aColumn = nsnull);
  virtual ~nsXULTreeitemAccessible() {}

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  NS_IMETHOD GetParent(nsIAccessible **_retval);
  NS_IMETHOD GetNextSibling(nsIAccessible **_retval);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **_retval);

  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  NS_IMETHOD SetSelected(PRBool aSelect); 
  NS_IMETHOD TakeFocus(void); 

  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  virtual PRBool IsDefunct();
  virtual nsresult Init();
  virtual nsresult Shutdown();

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEITEMACCESSIBLE_IMPL_CID)

  


  void GetCachedName(nsAString& aName);
  void SetCachedName(const nsAString& aName);

protected:
  PRBool IsExpandable();
  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  PRInt32 mRow;
  nsCOMPtr<nsITreeColumn> mColumn;
  nsString mCachedName;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeitemAccessible,
                              NS_XULTREEITEMACCESSIBLE_IMPL_CID)




class nsXULTreeColumnsAccessible : public nsXULColumnsAccessible
{
public:
  nsXULTreeColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
};

#endif
