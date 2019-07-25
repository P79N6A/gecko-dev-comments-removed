




#ifndef mozilla_a11y_HTMLTableAccessible_h__
#define mozilla_a11y_HTMLTableAccessible_h__

#include "HyperTextAccessibleWrap.h"
#include "nsIAccessibleTable.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "xpcAccessibleTable.h"
#include "xpcAccessibleTableCell.h"

class nsITableLayout;
class nsITableCellLayout;

namespace mozilla {
namespace a11y {




class HTMLTableCellAccessible : public HyperTextAccessibleWrap,
                                public nsIAccessibleTableCell,
                                public TableCellAccessible,
                                public xpcAccessibleTableCell
{
public:
  HTMLTableCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLECELL_WITH_XPCACCESSIBLETABLECELL

  
  virtual void Shutdown();
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  


  already_AddRefed<nsIAccessibleTable> GetTableAccessible();

  


  nsITableCellLayout* GetCellLayout();

  


  nsresult GetCellIndexes(PRInt32& aRowIdx, PRInt32& aColIdx);

  


  nsresult GetHeaderCells(PRInt32 aRowOrColumnHeaderCell,
                          nsIArray **aHeaderCells);
};





class HTMLTableHeaderCellAccessible : public HTMLTableCellAccessible
{
public:
  HTMLTableHeaderCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
};











class HTMLTableAccessible : public AccessibleWrap,
                            public xpcAccessibleTable,
                            public nsIAccessibleTable,
                            public TableAccessible
{
public:
  HTMLTableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIACCESSIBLETABLE(xpcAccessibleTable::)

  
  virtual Accessible* Caption();
  virtual void Summary(nsString& aSummary);
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
  virtual PRInt32 CellIndexAt(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual PRInt32 ColIndexAt(PRUint32 aCellIdx);
  virtual PRInt32 RowIndexAt(PRUint32 aCellIdx);
  virtual void RowAndColIndicesAt(PRUint32 aCellIdx, PRInt32* aRowIdx,
                                  PRInt32* aColIdx);
  virtual PRUint32 ColExtentAt(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual PRUint32 RowExtentAt(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual bool IsColSelected(PRUint32 aColIdx);
  virtual bool IsRowSelected(PRUint32 aRowIdx);
  virtual bool IsCellSelected(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual PRUint32 SelectedCellCount();
  virtual PRUint32 SelectedColCount();
  virtual PRUint32 SelectedRowCount();
  virtual void SelectedCells(nsTArray<Accessible*>* aCells);
  virtual void SelectedCellIndices(nsTArray<PRUint32>* aCells);
  virtual void SelectedColIndices(nsTArray<PRUint32>* aCols);
  virtual void SelectedRowIndices(nsTArray<PRUint32>* aRows);
  virtual void SelectCol(PRUint32 aColIdx);
  virtual void SelectRow(PRUint32 aRowIdx);
  virtual void UnselectCol(PRUint32 aColIdx);
  virtual void UnselectRow(PRUint32 aRowIdx);
  virtual bool IsProbablyLayoutTable();

  
  virtual void Shutdown();

  
  virtual TableAccessible* AsTable() { return this; }
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
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




class HTMLCaptionAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLCaptionAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) { }
  virtual ~HTMLCaptionAccessible() { }

  

  
  virtual a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aRelationType);
};

} 
} 

#endif
