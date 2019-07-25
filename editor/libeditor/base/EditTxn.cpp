





































#include "EditTxn.h"

NS_IMPL_CYCLE_COLLECTION_0(EditTxn)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(EditTxn)
  NS_INTERFACE_MAP_ENTRY(nsITransaction)
  NS_INTERFACE_MAP_ENTRY(nsPIEditorTransaction)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITransaction)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(EditTxn)
NS_IMPL_CYCLE_COLLECTING_RELEASE(EditTxn)

EditTxn::~EditTxn()
{
}

NS_IMETHODIMP
EditTxn::RedoTransaction(void)
{
  return DoTransaction();
}

NS_IMETHODIMP
EditTxn::GetIsTransient(PRBool *aIsTransient)
{
  *aIsTransient = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
EditTxn::Merge(nsITransaction *aTransaction, PRBool *aDidMerge)
{
  *aDidMerge = PR_FALSE;

  return NS_OK;
}
