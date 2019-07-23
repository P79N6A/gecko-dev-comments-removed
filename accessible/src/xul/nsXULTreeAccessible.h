




































#ifndef __nsXULTreeAccessible_h__
#define __nsXULTreeAccessible_h__

#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "nsXULSelectAccessible.h"
#include "nsIAccessibleTreeCache.h"




const PRUint32 kMaxTreeColumns = 100;
const PRUint32 kDefaultTreeCacheSize = 256;

class nsXULTreeAccessible : public nsXULSelectableAccessible,
                            public nsIAccessibleTreeCache
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLETREECACHE

  nsXULTreeAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeAccessible() {}

  
  NS_IMETHOD GetValue(nsAString& _retval);

  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  NS_IMETHOD GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY,
                                    nsIAccessible **aAccessible);

  
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
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




class nsXULTreeitemAccessible : public nsLeafAccessible,
                                public nsPIAccessibleTreeItem
{
public:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSPIACCESSIBLETREEITEM

  nsXULTreeitemAccessible(nsIAccessible *aParent, nsIDOMNode *aDOMNode, nsIWeakReference *aShell, PRInt32 aRow, nsITreeColumn* aColumn = nsnull);
  virtual ~nsXULTreeitemAccessible() {}

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

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

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  PRBool IsExpandable();
  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  PRInt32 mRow;
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
