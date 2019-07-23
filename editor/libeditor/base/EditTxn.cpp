





































#include "EditTxn.h"

NS_IMPL_ISUPPORTS2(EditTxn, nsITransaction, nsPIEditorTransaction)

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
