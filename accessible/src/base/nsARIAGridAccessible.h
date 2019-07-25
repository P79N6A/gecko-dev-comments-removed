





































#ifndef nsARIAGridAccessible_h_
#define nsARIAGridAccessible_h_

#include "nsIAccessibleTable.h"

#include "nsHyperTextAccessibleWrap.h"




class nsARIAGridAccessible : public nsAccessibleWrap,
                             public nsIAccessibleTable
{
public:
  nsARIAGridAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLE

protected:
  


  PRBool IsValidRow(PRInt32 aRow);

  


  PRBool IsValidColumn(PRInt32 aColumn);

  


  PRBool IsValidRowNColumn(PRInt32 aRow, PRInt32 aColumn);

  


  nsAccessible *GetRowAt(PRInt32 aRow);

  


  nsAccessible *GetCellInRowAt(nsAccessible *aRow, PRInt32 aColumn);

  







  nsresult SetARIASelected(nsAccessible *aAccessible, PRBool aIsSelected,
                           PRBool aNotify = PR_TRUE);

  


  nsresult GetSelectedColumnsArray(PRUint32 *acolumnCount,
                                   PRInt32 **aColumns = nsnull);
};





class nsARIAGridCellAccessible : public nsHyperTextAccessibleWrap,
                                 public nsIAccessibleTableCell
{
public:
  nsARIAGridCellAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual void ApplyARIAState(PRUint64* aState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

#endif
