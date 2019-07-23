





































#ifndef nsARIAGridAccessible_h_
#define nsARIAGridAccessible_h_

#include "nsIAccessibleTable.h"

#include "nsHyperTextAccessibleWrap.h"




class nsARIAGridAccessible : public nsAccessibleWrap,
                             public nsIAccessibleTable
{
public:
  nsARIAGridAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLE

protected:
  PRBool IsValidRow(PRInt32 aRow);
  PRBool IsValidColumn(PRInt32 aColumn);
  PRBool IsValidRowNColumn(PRInt32 aRow, PRInt32 aColumn);
};




class nsARIAGridCellAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsARIAGridCellAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

#endif
