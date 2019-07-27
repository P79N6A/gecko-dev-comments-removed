




#ifndef mozilla_a11y_HTMLTableAccessible_h__
#define mozilla_a11y_HTMLTableAccessible_h__

#include "HyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"

class nsITableLayout;
class nsITableCellLayout;

namespace mozilla {
namespace a11y {




class HTMLTableCellAccessible : public HyperTextAccessibleWrap,
                                public TableCellAccessible
{
public:
  HTMLTableCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableCellAccessible* AsTableCell() MOZ_OVERRIDE { return this; }
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;

  
  virtual TableAccessible* Table() const MOZ_OVERRIDE;
  virtual uint32_t ColIdx() const MOZ_OVERRIDE;
  virtual uint32_t RowIdx() const MOZ_OVERRIDE;
  virtual uint32_t ColExtent() const MOZ_OVERRIDE;
  virtual uint32_t RowExtent() const MOZ_OVERRIDE;
  virtual void ColHeaderCells(nsTArray<Accessible*>* aCells) MOZ_OVERRIDE;
  virtual void RowHeaderCells(nsTArray<Accessible*>* aCells) MOZ_OVERRIDE;
  virtual bool Selected() MOZ_OVERRIDE;

protected:
  virtual ~HTMLTableCellAccessible() {}

  


  nsITableCellLayout* GetCellLayout() const;

  


  nsresult GetCellIndexes(int32_t& aRowIdx, int32_t& aColIdx) const;
};





class HTMLTableHeaderCellAccessible : public HTMLTableCellAccessible
{
public:
  HTMLTableHeaderCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
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

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;

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

  
  virtual Accessible* Caption() const MOZ_OVERRIDE;
  virtual void Summary(nsString& aSummary) MOZ_OVERRIDE;
  virtual uint32_t ColCount() MOZ_OVERRIDE;
  virtual uint32_t RowCount() MOZ_OVERRIDE;
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex) MOZ_OVERRIDE;
  virtual int32_t CellIndexAt(uint32_t aRowIdx, uint32_t aColIdx) MOZ_OVERRIDE;
  virtual int32_t ColIndexAt(uint32_t aCellIdx) MOZ_OVERRIDE;
  virtual int32_t RowIndexAt(uint32_t aCellIdx) MOZ_OVERRIDE;
  virtual void RowAndColIndicesAt(uint32_t aCellIdx, int32_t* aRowIdx,
                                  int32_t* aColIdx) MOZ_OVERRIDE;
  virtual uint32_t ColExtentAt(uint32_t aRowIdx, uint32_t aColIdx) MOZ_OVERRIDE;
  virtual uint32_t RowExtentAt(uint32_t aRowIdx, uint32_t aColIdx) MOZ_OVERRIDE;
  virtual bool IsColSelected(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual bool IsRowSelected(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx) MOZ_OVERRIDE;
  virtual uint32_t SelectedCellCount() MOZ_OVERRIDE;
  virtual uint32_t SelectedColCount() MOZ_OVERRIDE;
  virtual uint32_t SelectedRowCount() MOZ_OVERRIDE;
  virtual void SelectedCells(nsTArray<Accessible*>* aCells) MOZ_OVERRIDE;
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells) MOZ_OVERRIDE;
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols) MOZ_OVERRIDE;
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows) MOZ_OVERRIDE;
  virtual void SelectCol(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual void SelectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual void UnselectCol(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual void UnselectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual bool IsProbablyLayoutTable() MOZ_OVERRIDE;
  virtual Accessible* AsAccessible() MOZ_OVERRIDE { return this; }

  
  virtual TableAccessible* AsTable() MOZ_OVERRIDE { return this; }
  virtual void Description(nsString& aDescription) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aRelationType) MOZ_OVERRIDE;

protected:
  virtual ~HTMLTableAccessible() {}

  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren() MOZ_OVERRIDE;

  

  






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

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aRelationType) MOZ_OVERRIDE;

protected:
  virtual ~HTMLCaptionAccessible() { }
};

} 
} 

#endif
