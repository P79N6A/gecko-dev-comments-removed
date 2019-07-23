





































#ifndef _nsHTMLTableAccessible_H_
#define _nsHTMLTableAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleTable.h"

class nsITableLayout;




class nsHTMLTableCellAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLTableCellAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);
  
  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  already_AddRefed<nsIAccessibleTable> GetTableAccessible();
  nsresult GetCellIndexes(PRInt32& aRowIdx, PRInt32& aColIdx);

  


  enum {
    
    eHeadersForCell,
    
    eCellsForRowHeader,
    
    eCellsForColumnHeader
  };

  







  nsresult FindCellsForRelation(PRInt32 aSearchHint, PRUint32 aRelationType,
                                nsIAccessibleRelation **aRelation);

  










  nsIContent* FindCell(nsHTMLTableAccessible *aTableAcc, nsIContent *aAnchorCell,
                       PRInt32 aRowIdx, PRInt32 aColIdx,
                       PRInt32 aLookForHeader);
};





class nsHTMLTableHeaderAccessible : public nsHTMLTableCellAccessible
{
public:
  nsHTMLTableHeaderAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);
};











#define NS_TABLEACCESSIBLE_IMPL_CID                     \
{  /* 8d6d9c40-74bd-47ac-88dc-4a23516aa23d */           \
  0x8d6d9c40,                                           \
  0x74bd,                                               \
  0x47ac,                                               \
  { 0x88, 0xdc, 0x4a, 0x23, 0x51, 0x6a, 0xa2, 0x3d }    \
}

class nsHTMLTableAccessible : public nsAccessibleWrap,
                              public nsIAccessibleTable
{
public:
  nsHTMLTableAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TABLEACCESSIBLE_IMPL_CID)

  
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  

  




  PRBool IsValidColumn(PRInt32 aColumn);

  




  PRBool IsValidRow(PRInt32 aRow);

  


  nsresult GetCellAt(PRInt32 aRowIndex, PRInt32 aColIndex,
                     nsIDOMElement* &aCell);
protected:

  






  nsresult AddRowOrColumnToSelection(PRInt32 aIndex, PRUint32 aTarget);

  








  nsresult RemoveRowsOrColumnsFromSelection(PRInt32 aIndex,
                                            PRUint32 aTarget,
                                            PRBool aIsOuter);

  virtual void CacheChildren();
  nsresult GetTableNode(nsIDOMNode **_retval);
  nsresult GetTableLayout(nsITableLayout **aLayoutObject);

  






  PRBool HasDescendant(const nsAString& aTagName, PRBool aAllowEmpty = PR_TRUE);

#ifdef SHOW_LAYOUT_HEURISTIC
  nsString mLayoutHeuristic;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLTableAccessible,
                              NS_TABLEACCESSIBLE_IMPL_CID)


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
