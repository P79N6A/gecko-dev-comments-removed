





































#ifndef _nsHTMLTableAccessible_H_
#define _nsHTMLTableAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleTable.h"

class nsHTMLTableCellAccessible : public nsHyperTextAccessible
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsHTMLTableCellAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aResult); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsITableLayout;




#define SHOW_LAYOUT_HEURISTIC

class nsHTMLTableAccessible : public nsAccessibleWrap,
                              public nsIAccessibleTable
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  nsHTMLTableAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aResult); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& aResult);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
#ifdef SHOW_LAYOUT_HEURISTIC
  NS_IMETHOD GetDescription(nsAString& aDescription);
#endif

protected:
  virtual void CacheChildren();
  nsresult GetTableNode(nsIDOMNode **_retval);
  nsresult GetTableLayout(nsITableLayout **aLayoutObject);
  nsresult GetCellAt(PRInt32        aRowIndex,
                     PRInt32        aColIndex,
                     nsIDOMElement* &aCell);
  PRBool HasDescendant(char *aTagName, PRBool aAllowEmpty = PR_TRUE);
  PRBool mHasCaption;
#ifdef SHOW_LAYOUT_HEURISTIC
  nsAutoString mLayoutHeuristic;
#endif
};

class nsHTMLTableHeadAccessible : public nsHTMLTableAccessible
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsHTMLTableHeadAccessible(nsIDOMNode *aDomNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aResult);

  
  NS_IMETHOD GetCaption(nsIAccessible **aCaption);
  NS_IMETHOD SetCaption(nsIAccessible *aCaption);
  NS_IMETHOD GetSummary(nsAString &aSummary);
  NS_IMETHOD SetSummary(const nsAString &aSummary);
  NS_IMETHOD GetColumnHeader(nsIAccessibleTable **aColumnHeader);
  NS_IMETHOD GetRows(PRInt32 *aRows);
};

#endif  
