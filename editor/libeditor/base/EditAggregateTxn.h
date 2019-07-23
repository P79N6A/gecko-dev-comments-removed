




































#ifndef EditAggregateTxn_h__
#define EditAggregateTxn_h__

#include "EditTxn.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"





class EditAggregateTxn : public EditTxn
{
public:
  NS_IMETHOD QueryInterface(REFNSIID aIID, void **aInstancePtr);

  EditAggregateTxn();

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();
  NS_IMETHOD Merge(nsITransaction *aTransaction, PRBool *aDidMerge);

  
  NS_IMETHOD AppendChild(EditTxn *aTxn);

  


  NS_IMETHOD GetCount(PRUint32 *aCount);

  


  NS_IMETHOD GetTxnAt(PRInt32 aIndex, EditTxn **aTxn);

  
  NS_IMETHOD SetName(nsIAtom *aName);

  
  NS_IMETHOD GetName(nsIAtom **aName);

protected:

  nsTArray< nsRefPtr<EditTxn> > mChildren;
  nsCOMPtr<nsIAtom> mName;
};

#endif
