




































#ifndef EditTxn_h__
#define EditTxn_h__

#include "nsITransaction.h"
#include "nsString.h"
#include "nsPIEditorTransaction.h"

#define EDIT_TXN_CID \
{/* c5ea31b0-ac48-11d2-86d8-000064657374 */ \
0xc5ea31b0, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }




class EditTxn : public nsITransaction,
                public nsPIEditorTransaction
{
public:

  static const nsIID& GetCID() { static const nsIID iid = EDIT_TXN_CID; return iid; }

  NS_DECL_ISUPPORTS

  virtual ~EditTxn();

  NS_IMETHOD RedoTransaction(void);
  NS_IMETHOD GetIsTransient(PRBool *aIsTransient);
  NS_IMETHOD Merge(nsITransaction *aTransaction, PRBool *aDidMerge);
};

#define NS_DECL_EDITTXN \
  NS_IMETHOD DoTransaction(); \
  NS_IMETHOD UndoTransaction(); \
  NS_IMETHOD GetTxnDescription(nsAString& aTxnDescription);

#endif
