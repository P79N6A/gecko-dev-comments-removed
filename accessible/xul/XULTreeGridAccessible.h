




#ifndef mozilla_a11y_XULTreeGridAccessible_h__
#define mozilla_a11y_XULTreeGridAccessible_h__

#include "XULTreeAccessible.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "xpcAccessibleTable.h"
#include "xpcAccessibleTableCell.h"

namespace mozilla {
namespace a11y {

class XULTreeGridCellAccessible;




class XULTreeGridAccessible : public XULTreeAccessible,
                              public TableAccessible
{
public:
  XULTreeGridAccessible(nsIContent* aContent, DocAccessible* aDoc,
                        nsTreeBodyFrame* aTreeFrame) :
    XULTreeAccessible(aContent, aDoc, aTreeFrame)
    { mGenericTypes |= eTable; }

  
  virtual uint32_t ColCount() MOZ_OVERRIDE;
  virtual uint32_t RowCount() MOZ_OVERRIDE;
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex) MOZ_OVERRIDE;
  virtual void ColDescription(uint32_t aColIdx, nsString& aDescription) MOZ_OVERRIDE;
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
  virtual void SelectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual void UnselectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual Accessible* AsAccessible() MOZ_OVERRIDE { return this; }

  
  virtual TableAccessible* AsTable() MOZ_OVERRIDE { return this; }
  virtual a11y::role NativeRole() MOZ_OVERRIDE;

protected:
  virtual ~XULTreeGridAccessible();

  
  virtual already_AddRefed<Accessible>
    CreateTreeItemAccessible(int32_t aRow) const MOZ_OVERRIDE;
};






class XULTreeGridRowAccessible MOZ_FINAL : public XULTreeItemAccessibleBase
{
public:
  using Accessible::GetChildAt;

  XULTreeGridRowAccessible(nsIContent* aContent, DocAccessible* aDoc,
                           Accessible* aParent, nsITreeBoxObject* aTree,
                           nsITreeView* aTreeView, int32_t aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeGridRowAccessible,
                                           XULTreeItemAccessibleBase)

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual ENameValueFlag Name(nsString& aName) MOZ_OVERRIDE;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) MOZ_OVERRIDE;

  virtual Accessible* GetChildAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual uint32_t ChildCount() const MOZ_OVERRIDE;

  
  virtual XULTreeGridCellAccessible* GetCellAccessible(nsITreeColumn* aColumn)
    const MOZ_OVERRIDE MOZ_FINAL;
  virtual void RowInvalidated(int32_t aStartColIdx, int32_t aEndColIdx) MOZ_OVERRIDE;

protected:
  virtual ~XULTreeGridRowAccessible();

  
  virtual void CacheChildren() MOZ_OVERRIDE;

  
  mutable nsRefPtrHashtable<nsPtrHashKey<const void>, XULTreeGridCellAccessible>
    mAccessibleCache;
};







class XULTreeGridCellAccessible : public LeafAccessible,
                                  public TableCellAccessible
{
public:

  XULTreeGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc,
                            XULTreeGridRowAccessible* aRowAcc,
                            nsITreeBoxObject* aTree, nsITreeView* aTreeView,
                            int32_t aRow, nsITreeColumn* aColumn);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeGridCellAccessible,
                                           LeafAccessible)

  
  virtual TableCellAccessible* AsTableCell() MOZ_OVERRIDE { return this; }
  virtual nsIntRect Bounds() const MOZ_OVERRIDE;
  virtual ENameValueFlag Name(nsString& aName) MOZ_OVERRIDE;
  virtual Accessible* FocusedChild() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual int32_t IndexInParent() const MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual TableAccessible* Table() const MOZ_OVERRIDE;
  virtual uint32_t ColIdx() const MOZ_OVERRIDE;
  virtual uint32_t RowIdx() const MOZ_OVERRIDE;
  virtual void ColHeaderCells(nsTArray<Accessible*>* aHeaderCells) MOZ_OVERRIDE;
  virtual void RowHeaderCells(nsTArray<Accessible*>* aCells) MOZ_OVERRIDE { }
  virtual bool Selected() MOZ_OVERRIDE;

  




  bool CellInvalidated();

protected:
  virtual ~XULTreeGridCellAccessible();

  
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult* aError = nullptr) const MOZ_OVERRIDE;
  virtual void DispatchClickEvent(nsIContent* aContent, uint32_t aActionIndex) MOZ_OVERRIDE;

  

  


  bool IsEditable() const;

  enum { eAction_Click = 0 };

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;

  int32_t mRow;
  nsCOMPtr<nsITreeColumn> mColumn;

  nsString mCachedTextEquiv;
};

} 
} 

#endif
