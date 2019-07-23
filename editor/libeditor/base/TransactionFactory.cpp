





































#include "TransactionFactory.h"

#include "EditAggregateTxn.h"
#include "PlaceholderTxn.h"
#include "InsertTextTxn.h"
#include "DeleteTextTxn.h"
#include "CreateElementTxn.h"
#include "InsertElementTxn.h"
#include "DeleteElementTxn.h"
#include "DeleteRangeTxn.h"
#include "ChangeAttributeTxn.h"
#include "SplitElementTxn.h"
#include "JoinElementTxn.h"
#include "nsStyleSheetTxns.h"
#include "IMETextTxn.h"

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
#include "SetDocTitleTxn.h"
#include "ChangeCSSInlineStyleTxn.h"
#endif 

TransactionFactory::TransactionFactory()
{
}

TransactionFactory::~TransactionFactory()
{
}

nsresult
TransactionFactory::GetNewTransaction(REFNSIID aTxnType, EditTxn **aResult)
{
  nsresult result = NS_OK;
  *aResult = nsnull;
  if (aTxnType.Equals(InsertTextTxn::GetCID()))
    *aResult = new InsertTextTxn();
  else if (aTxnType.Equals(DeleteTextTxn::GetCID()))
    *aResult = new DeleteTextTxn();
  else if (aTxnType.Equals(CreateElementTxn::GetCID()))
    *aResult = new CreateElementTxn();
  else if (aTxnType.Equals(InsertElementTxn::GetCID()))
    *aResult = new InsertElementTxn();
  else if (aTxnType.Equals(DeleteElementTxn::GetCID()))
    *aResult = new DeleteElementTxn();
  else if (aTxnType.Equals(DeleteRangeTxn::GetCID()))
    *aResult = new DeleteRangeTxn();
  else if (aTxnType.Equals(ChangeAttributeTxn::GetCID()))
    *aResult = new ChangeAttributeTxn();
#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
  else if (aTxnType.Equals(ChangeCSSInlineStyleTxn::GetCID()))
    *aResult = new ChangeCSSInlineStyleTxn();
#endif 
  else if (aTxnType.Equals(SplitElementTxn::GetCID()))
    *aResult = new SplitElementTxn();
  else if (aTxnType.Equals(JoinElementTxn::GetCID()))
    *aResult = new JoinElementTxn();
  else if (aTxnType.Equals(EditAggregateTxn::GetCID()))
    *aResult = new EditAggregateTxn();
  else if (aTxnType.Equals(IMETextTxn::GetCID()))
    *aResult = new IMETextTxn();
  else if (aTxnType.Equals(AddStyleSheetTxn::GetCID()))
    *aResult = new AddStyleSheetTxn();
  else if (aTxnType.Equals(RemoveStyleSheetTxn::GetCID()))
    *aResult = new RemoveStyleSheetTxn();
#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
  else if (aTxnType.Equals(SetDocTitleTxn::GetCID()))
    *aResult = new SetDocTitleTxn();
#endif 
  else if (aTxnType.Equals(PlaceholderTxn::GetCID()))
    *aResult = new PlaceholderTxn();
  else
    result = NS_ERROR_NO_INTERFACE;
  
  if (NS_SUCCEEDED(result) && !*aResult)
    result = NS_ERROR_OUT_OF_MEMORY;

  if (NS_SUCCEEDED(result))
    NS_ADDREF(*aResult);

  return result;
}

