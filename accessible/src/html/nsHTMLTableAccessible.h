





































#ifndef _nsHTMLTableAccessible_H_
#define _nsHTMLTableAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleTable.h"

class nsHTMLTableCellAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLTableCellAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

class nsITableLayout;






class nsHTMLTableAccessible : public nsAccessibleWrap,
                              public nsIAccessibleTable
{
public:
  nsHTMLTableAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  

  




  PRBool IsValidColumn(PRInt32 aColumn);

  




  PRBool IsValidRow(PRInt32 aRow);

protected:

  








  nsresult SelectRowOrColumn(PRInt32 aIndex, PRUint32 aTarget, PRBool aDoSelect);

  







  nsresult SelectCell(nsISelection *aSelection, nsIDocument *aDocument,
                      nsIDOMElement *aCellElement, PRBool aDoSelect);

  virtual void CacheChildren();
  nsresult GetTableNode(nsIDOMNode **_retval);
  nsresult GetTableLayout(nsITableLayout **aLayoutObject);
  nsresult GetCellAt(PRInt32        aRowIndex,
                     PRInt32        aColIndex,
                     nsIDOMElement* &aCell);
  PRBool HasDescendant(char *aTagName, PRBool aAllowEmpty = PR_TRUE);
#ifdef SHOW_LAYOUT_HEURISTIC
  nsAutoString mLayoutHeuristic;
#endif
};

class nsHTMLTableHeadAccessible : public nsHTMLTableAccessible
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsHTMLTableHeadAccessible(nsIDOMNode *aDomNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetCaption(nsIAccessible **aCaption);
  NS_IMETHOD GetSummary(nsAString &aSummary);
  NS_IMETHOD GetColumnHeader(nsIAccessibleTable **aColumnHeader);
  NS_IMETHOD GetRows(PRInt32 *aRows);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

class nsHTMLCaptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLCaptionAccessible(nsIDOMNode *aDomNode, nsIWeakReference *aShell) :
    nsHyperTextAccessibleWrap(aDomNode, aShell) { }

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif  
