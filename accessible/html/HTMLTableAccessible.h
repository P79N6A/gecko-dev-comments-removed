




#ifndef mozilla_a11y_HTMLTableAccessible_h__
#define mozilla_a11y_HTMLTableAccessible_h__

#include "HyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"

class nsITableCellLayout;

namespace mozilla {
namespace a11y {




class HTMLTableCellAccessible : public HyperTextAccessibleWrap,
                                public TableCellAccessible
{
public:
  HTMLTableCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableCellAccessible* AsTableCell() override { return this; }
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual uint64_t NativeInteractiveState() const override;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;

  
  virtual TableAccessible* Table() const override;
  virtual uint32_t ColIdx() const override;
  virtual uint32_t RowIdx() const override;
  virtual uint32_t ColExtent() const override;
  virtual uint32_t RowExtent() const override;
  virtual void ColHeaderCells(nsTArray<Accessible*>* aCells) override;
  virtual void RowHeaderCells(nsTArray<Accessible*>* aCells) override;
  virtual bool Selected() override;

protected:
  virtual ~HTMLTableCellAccessible() {}

  


  nsITableCellLayout* GetCellLayout() const;

  


  nsresult GetCellIndexes(int32_t& aRowIdx, int32_t& aColIdx) const;
};





class HTMLTableHeaderCellAccessible : public HTMLTableCellAccessible
{
public:
  HTMLTableHeaderCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() override;
};





class HTMLTableRowAccessible : public AccessibleWrap
{
public:
  HTMLTableRowAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc)
  {
    mType = eHTMLTableRowType;
    mGenericTypes |= eTableRow;
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole() override;

protected:
  virtual ~HTMLTableRowAccessible() { }
};











class HTMLTableAccessible : public AccessibleWrap,
                            public TableAccessible
{
public:
  HTMLTableAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc)
  {
    mType = eHTMLTableType;
    mGenericTypes |= eTable;
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Accessible* Caption() const override;
  virtual void Summary(nsString& aSummary) override;
  virtual uint32_t ColCount() override;
  virtual uint32_t RowCount() override;
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex) override;
  virtual int32_t CellIndexAt(uint32_t aRowIdx, uint32_t aColIdx) override;
  virtual int32_t ColIndexAt(uint32_t aCellIdx) override;
  virtual int32_t RowIndexAt(uint32_t aCellIdx) override;
  virtual void RowAndColIndicesAt(uint32_t aCellIdx, int32_t* aRowIdx,
                                  int32_t* aColIdx) override;
  virtual uint32_t ColExtentAt(uint32_t aRowIdx, uint32_t aColIdx) override;
  virtual uint32_t RowExtentAt(uint32_t aRowIdx, uint32_t aColIdx) override;
  virtual bool IsColSelected(uint32_t aColIdx) override;
  virtual bool IsRowSelected(uint32_t aRowIdx) override;
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx) override;
  virtual uint32_t SelectedCellCount() override;
  virtual uint32_t SelectedColCount() override;
  virtual uint32_t SelectedRowCount() override;
  virtual void SelectedCells(nsTArray<Accessible*>* aCells) override;
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells) override;
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols) override;
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows) override;
  virtual void SelectCol(uint32_t aColIdx) override;
  virtual void SelectRow(uint32_t aRowIdx) override;
  virtual void UnselectCol(uint32_t aColIdx) override;
  virtual void UnselectRow(uint32_t aRowIdx) override;
  virtual bool IsProbablyLayoutTable() override;
  virtual Accessible* AsAccessible() override { return this; }

  
  virtual TableAccessible* AsTable() override { return this; }
  virtual void Description(nsString& aDescription) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;
  virtual Relation RelationByType(RelationType aRelationType) override;

protected:
  virtual ~HTMLTableAccessible() {}

  
  virtual ENameValueFlag NativeName(nsString& aName) override;
  virtual void CacheChildren() override;

  

  






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

  
  virtual a11y::role NativeRole() override;
  virtual Relation RelationByType(RelationType aRelationType) override;

protected:
  virtual ~HTMLCaptionAccessible() { }
};

} 
} 

#endif
