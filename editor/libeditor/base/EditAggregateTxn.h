




































#ifndef EditAggregateTxn_h__
#define EditAggregateTxn_h__

#include "EditTxn.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"

#define EDIT_AGGREGATE_TXN_CID \
{/* 345921a0-ac49-11d2-86d8-000064657374 */ \
0x345921a0, 0xac49, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }






class EditAggregateTxn : public EditTxn
{
public:
  NS_IMETHOD QueryInterface(REFNSIID aIID, void **aInstancePtr);

  static const nsIID& GetCID() { static const nsIID cid = EDIT_AGGREGATE_TXN_CID; return cid; }

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

  nsCOMPtr<nsISupportsArray> mChildren;
  nsCOMPtr<nsIAtom> mName;
};

#endif
