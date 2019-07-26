




#ifndef MOZILLA_A11Y_ARIAGridAccessible_h_
#define MOZILLA_A11Y_ARIAGridAccessible_h_

#include "nsIAccessibleTable.h"

#include "HyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"

namespace mozilla {
namespace a11y {




class ARIAGridAccessible : public AccessibleWrap,
                           public xpcAccessibleTable,
                           public nsIAccessibleTable,
                           public TableAccessible
{
public:
  ARIAGridAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual TableAccessible* AsTable() { return this; }

  
  virtual void Shutdown();

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
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

protected:
  


  bool IsValidRow(PRInt32 aRow);

  


  bool IsValidColumn(PRInt32 aColumn);

  


  Accessible* GetRowAt(PRInt32 aRow);

  


  Accessible* GetCellInRowAt(Accessible* aRow, PRInt32 aColumn);

  







  nsresult SetARIASelected(Accessible* aAccessible, bool aIsSelected,
                           bool aNotify = true);
};





class ARIAGridCellAccessible : public HyperTextAccessibleWrap,
                               public nsIAccessibleTableCell
{
public:
  ARIAGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual void ApplyARIAState(PRUint64* aState) const;
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

} 
} 

#endif
