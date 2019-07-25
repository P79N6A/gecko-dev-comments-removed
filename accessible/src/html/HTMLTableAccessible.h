




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
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  


  already_AddRefed<nsIAccessibleTable> GetTableAccessible();

  


  nsITableCellLayout* GetCellLayout();

  


  nsresult GetCellIndexes(int32_t& aRowIdx, int32_t& aColIdx);

  


  nsresult GetHeaderCells(int32_t aRowOrColumnHeaderCell,
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
  virtual uint32_t ColCount();
  virtual uint32_t RowCount();
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex);
  virtual int32_t CellIndexAt(uint32_t aRowIdx, uint32_t aColIdx);
  virtual int32_t ColIndexAt(uint32_t aCellIdx);
  virtual int32_t RowIndexAt(uint32_t aCellIdx);
  virtual void RowAndColIndicesAt(uint32_t aCellIdx, int32_t* aRowIdx,
                                  int32_t* aColIdx);
  virtual uint32_t ColExtentAt(uint32_t aRowIdx, uint32_t aColIdx);
  virtual uint32_t RowExtentAt(uint32_t aRowIdx, uint32_t aColIdx);
  virtual bool IsColSelected(uint32_t aColIdx);
  virtual bool IsRowSelected(uint32_t aRowIdx);
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx);
  virtual uint32_t SelectedCellCount();
  virtual uint32_t SelectedColCount();
  virtual uint32_t SelectedRowCount();
  virtual void SelectedCells(nsTArray<Accessible*>* aCells);
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells);
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols);
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows);
  virtual void SelectCol(uint32_t aColIdx);
  virtual void SelectRow(uint32_t aRowIdx);
  virtual void UnselectCol(uint32_t aColIdx);
  virtual void UnselectRow(uint32_t aRowIdx);
  virtual bool IsProbablyLayoutTable();

  
  virtual void Shutdown();

  
  virtual TableAccessible* AsTable() { return this; }
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual Relation RelationByType(uint32_t aRelationType);

  

  


  nsresult GetCellAt(int32_t aRowIndex, int32_t aColIndex,
                     nsIDOMElement* &aCell);

  


  nsITableLayout* GetTableLayout();

protected:

  
  virtual void CacheChildren();

  

  






  nsresult AddRowOrColumnToSelection(int32_t aIndex, uint32_t aTarget);

  








  nsresult RemoveRowsOrColumnsFromSelection(int32_t aIndex,
                                            uint32_t aTarget,
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
  virtual Relation RelationByType(uint32_t aRelationType);
};

} 
} 

#endif
