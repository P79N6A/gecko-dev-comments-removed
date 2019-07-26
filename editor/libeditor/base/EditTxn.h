




#ifndef EditTxn_h__
#define EditTxn_h__

#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "nsITransaction.h"
#include "nsPIEditorTransaction.h"
#include "nscore.h"




class EditTxn : public nsITransaction,
                public nsPIEditorTransaction
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(EditTxn, nsITransaction)

  virtual void LastRelease() {}

  NS_IMETHOD RedoTransaction(void);
  NS_IMETHOD GetIsTransient(bool *aIsTransient);
  NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge);

protected:
  virtual ~EditTxn();
};

#define NS_DECL_EDITTXN \
  NS_IMETHOD DoTransaction(); \
  NS_IMETHOD UndoTransaction(); \
  NS_IMETHOD GetTxnDescription(nsAString& aTxnDescription);

#endif
