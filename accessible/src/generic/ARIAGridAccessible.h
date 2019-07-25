




#ifndef MOZILLA_A11Y_ARIAGridAccessible_h_
#define MOZILLA_A11Y_ARIAGridAccessible_h_

#include "nsIAccessibleTable.h"

#include "nsHyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"

namespace mozilla {
namespace a11y {




class ARIAGridAccessible : public nsAccessibleWrap,
                           public xpcAccessibleTable,
                           public nsIAccessibleTable,
                           public TableAccessible
{
public:
  ARIAGridAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual mozilla::a11y::TableAccessible* AsTable() { return this; }

  
  virtual void Shutdown();

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual void UnselectCol(PRUint32 aColIdx);
  virtual void UnselectRow(PRUint32 aRowIdx);

protected:
  


  bool IsValidRow(PRInt32 aRow);

  


  bool IsValidColumn(PRInt32 aColumn);

  


  bool IsValidRowNColumn(PRInt32 aRow, PRInt32 aColumn);

  


  nsAccessible *GetRowAt(PRInt32 aRow);

  


  nsAccessible *GetCellInRowAt(nsAccessible *aRow, PRInt32 aColumn);

  







  nsresult SetARIASelected(nsAccessible *aAccessible, bool aIsSelected,
                           bool aNotify = true);

  


  nsresult GetSelectedColumnsArray(PRUint32 *acolumnCount,
                                   PRInt32 **aColumns = nsnull);
};





class ARIAGridCellAccessible : public nsHyperTextAccessibleWrap,
                               public nsIAccessibleTableCell
{
public:
  ARIAGridCellAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual void ApplyARIAState(PRUint64* aState) const;
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

} 
} 

#endif
