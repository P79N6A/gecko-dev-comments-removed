




#ifndef EditAggregateTxn_h__
#define EditAggregateTxn_h__

#include "EditTxn.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIAtom.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "nscore.h"

class nsITransaction;





class EditAggregateTxn : public EditTxn
{
public:
  EditAggregateTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(EditAggregateTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();
  NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge);

  
  NS_IMETHOD AppendChild(EditTxn *aTxn);

  
  NS_IMETHOD GetName(nsIAtom **aName);

protected:
  virtual ~EditAggregateTxn();

  nsTArray< nsRefPtr<EditTxn> > mChildren;
  nsCOMPtr<nsIAtom> mName;
};

#endif
