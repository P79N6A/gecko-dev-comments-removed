




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

  NS_IMETHOD RedoTransaction(void) MOZ_OVERRIDE;
  NS_IMETHOD GetIsTransient(bool *aIsTransient) MOZ_OVERRIDE;
  NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge) MOZ_OVERRIDE;

protected:
  virtual ~EditTxn();
};

#define NS_DECL_EDITTXN \
  NS_IMETHOD DoTransaction() MOZ_OVERRIDE; \
  NS_IMETHOD UndoTransaction() MOZ_OVERRIDE; \
  NS_IMETHOD GetTxnDescription(nsAString& aTxnDescription) MOZ_OVERRIDE;

#endif
