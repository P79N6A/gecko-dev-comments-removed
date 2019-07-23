





































#include "nsEditor.h"

#include "nsIPresShell.h"
#include "nsICSSStyleSheet.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsISelectionController.h"


#include "nsStyleSheetTxns.h"

static void
AddStyleSheet(nsIEditor* aEditor, nsIStyleSheet* aSheet)
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  aEditor->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (doc) {
    doc->BeginUpdate(UPDATE_STYLE);
    doc->AddStyleSheet(aSheet);
    doc->EndUpdate(UPDATE_STYLE);
  }
}

static void
RemoveStyleSheet(nsIEditor *aEditor, nsIStyleSheet *aSheet)
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  aEditor->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (doc) {
    doc->BeginUpdate(UPDATE_STYLE);
    doc->RemoveStyleSheet(aSheet);
    doc->EndUpdate(UPDATE_STYLE);
  }
}

AddStyleSheetTxn::AddStyleSheetTxn()
:  EditTxn()
,  mEditor(NULL)
{
}

NS_IMETHODIMP
AddStyleSheetTxn::Init(nsIEditor *aEditor, nsICSSStyleSheet *aSheet)
{
  if (!aEditor)
    return NS_ERROR_INVALID_ARG;

  if (!aSheet)
    return NS_ERROR_INVALID_ARG;

  mEditor = aEditor;
  mSheet = do_QueryInterface(aSheet);
  
  return NS_OK;
}


NS_IMETHODIMP
AddStyleSheetTxn::DoTransaction()
{
  if (!mEditor || !mSheet)
    return NS_ERROR_NOT_INITIALIZED;
  
  AddStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
AddStyleSheetTxn::UndoTransaction()
{
  if (!mEditor || !mSheet)
    return NS_ERROR_NOT_INITIALIZED;
  
  RemoveStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
AddStyleSheetTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("AddStyleSheetTxn");
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif


RemoveStyleSheetTxn::RemoveStyleSheetTxn()
:  EditTxn()
,  mEditor(NULL)
{
}

NS_IMETHODIMP
RemoveStyleSheetTxn::Init(nsIEditor *aEditor, nsICSSStyleSheet *aSheet)
{
  if (!aEditor)
    return NS_ERROR_INVALID_ARG;

  if (!aSheet)
    return NS_ERROR_INVALID_ARG;

  mEditor = aEditor;
  mSheet = do_QueryInterface(aSheet);
  
  return NS_OK;
}


NS_IMETHODIMP
RemoveStyleSheetTxn::DoTransaction()
{
  if (!mEditor || !mSheet)
    return NS_ERROR_NOT_INITIALIZED;

  RemoveStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
RemoveStyleSheetTxn::UndoTransaction()
{
  if (!mEditor || !mSheet)
    return NS_ERROR_NOT_INITIALIZED;

  AddStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
RemoveStyleSheetTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("RemoveStyleSheetTxn");
  return NS_OK;
}
