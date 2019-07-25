





































#ifndef _nsHTMLTableAccessible_H_
#define _nsHTMLTableAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsIAccessibleTable.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"

class nsITableLayout;
class nsITableCellLayout;




class nsHTMLTableCellAccessible : public nsHyperTextAccessibleWrap,
                                  public nsIAccessibleTableCell
{
public:
  nsHTMLTableCellAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  


  already_AddRefed<nsIAccessibleTable> GetTableAccessible();
  
  


  nsITableCellLayout* GetCellLayout();

  


  nsresult GetCellIndexes(PRInt32& aRowIdx, PRInt32& aColIdx);
  
  


  nsresult GetHeaderCells(PRInt32 aRowOrColumnHeaderCell,
                          nsIArray **aHeaderCells);
};





class nsHTMLTableHeaderCellAccessible : public nsHTMLTableCellAccessible
{
public:
  nsHTMLTableHeaderCellAccessible(nsIContent* aContent,
                                  nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};











#define NS_TABLEACCESSIBLE_IMPL_CID                     \
{  /* 8d6d9c40-74bd-47ac-88dc-4a23516aa23d */           \
  0x8d6d9c40,                                           \
  0x74bd,                                               \
  0x47ac,                                               \
  { 0x88, 0xdc, 0x4a, 0x23, 0x51, 0x6a, 0xa2, 0x3d }    \
}

class nsHTMLTableAccessible : public nsAccessibleWrap,
                              public xpcAccessibleTable,
                              public nsIAccessibleTable,
                              public mozilla::a11y::TableAccessible
{
public:
  nsHTMLTableAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TABLEACCESSIBLE_IMPL_CID)

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual nsAccessible* Caption();
  virtual bool IsProbablyLayoutTable();

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::TableAccessible* AsTable() { return this; }
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual Relation RelationByType(PRUint32 aRelationType);

  

  


  nsresult GetCellAt(PRInt32 aRowIndex, PRInt32 aColIndex,
                     nsIDOMElement* &aCell);

  


  nsITableLayout* GetTableLayout();

protected:

  
  virtual void CacheChildren();

  

  






  nsresult AddRowOrColumnToSelection(PRInt32 aIndex, PRUint32 aTarget);

  








  nsresult RemoveRowsOrColumnsFromSelection(PRInt32 aIndex,
                                            PRUint32 aTarget,
                                            bool aIsOuter);

  






  bool HasDescendant(const nsAString& aTagName, bool aAllowEmpty = true);

#ifdef SHOW_LAYOUT_HEURISTIC
  nsString mLayoutHeuristic;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLTableAccessible,
                              NS_TABLEACCESSIBLE_IMPL_CID)





class nsHTMLCaptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLCaptionAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsHyperTextAccessibleWrap(aContent, aDoc) { }
  virtual ~nsHTMLCaptionAccessible() { }

  

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aRelationType);
};

#endif  
