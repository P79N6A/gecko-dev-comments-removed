




































#ifndef __nsXULTreeAccessible_h__
#define __nsXULTreeAccessible_h__

#include "nsBaseWidgetAccessible.h"
#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeColumns.h"
#include "nsXULSelectAccessible.h"
#include "nsIAccessibleTreeCache.h"





const PRUint32 kDefaultTreeCacheSize = 256;

class nsXULTreeAccessible : public nsXULSelectableAccessible,
                            public nsIAccessibleTreeCache
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLETREECACHE

  nsXULTreeAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeAccessible() {};

  
  NS_IMETHOD Shutdown();
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetValue(nsAString& _retval);

  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);

  static void GetTreeBoxObject(nsIDOMNode* aDOMNode, nsITreeBoxObject** aBoxObject);
  static nsresult GetColumnCount(nsITreeBoxObject* aBoxObject, PRInt32 *aCount);

  static PRBool IsColumnHidden(nsITreeColumn *aColumn);
  static already_AddRefed<nsITreeColumn> GetNextVisibleColumn(nsITreeColumn *aColumn);
  static already_AddRefed<nsITreeColumn> GetFirstVisibleColumn(nsITreeBoxObject *aTree);
  static already_AddRefed<nsITreeColumn> GetLastVisibleColumn(nsITreeBoxObject *aTree);

protected:
  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> *mAccessNodeCache;

  NS_IMETHOD ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);
};




class nsXULTreeitemAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0, eAction_Expand = 1 };

  NS_DECL_ISUPPORTS_INHERITED

  nsXULTreeitemAccessible(nsIAccessible *aParent, nsIDOMNode *aDOMNode, nsIWeakReference *aShell, PRInt32 aRow, nsITreeColumn* aColumn = nsnull);
  virtual ~nsXULTreeitemAccessible() {}

  NS_IMETHOD Shutdown();

  
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);

  NS_IMETHOD GetParent(nsIAccessible **_retval);
  NS_IMETHOD GetNextSibling(nsIAccessible **_retval);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **_retval);

  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  NS_IMETHOD SetSelected(PRBool aSelect); 
  NS_IMETHOD TakeFocus(void); 

  NS_IMETHOD GetAccessibleRelated(PRUint32 aRelationType, nsIAccessible **aRelated);
  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

protected:
  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeView> mTreeView;
  PRInt32 mRow;
  nsCOMPtr<nsITreeColumn> mColumn;
};

class nsXULTreeColumnsAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  nsXULTreeColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeColumnsAccessible() {}

  
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  NS_IMETHOD GetNextSibling(nsIAccessible **_retval); 
  NS_IMETHOD GetPreviousSibling(nsIAccessible **_retval); 

  NS_IMETHOD DoAction(PRUint8 index);
};

class nsXULTreeColumnitemAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  nsXULTreeColumnitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeColumnitemAccessible() {}

  
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  NS_IMETHOD DoAction(PRUint8 index);
};

#endif
