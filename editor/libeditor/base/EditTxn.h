




































#ifndef EditTxn_h__
#define EditTxn_h__

#include "nsITransaction.h"
#include "nsString.h"
#include "nsPIEditorTransaction.h"
#include "nsCycleCollectionParticipant.h"




class EditTxn : public nsITransaction,
                public nsPIEditorTransaction
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(EditTxn, nsITransaction)

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
