





#include <stddef.h>                     

#include "nsAString.h"
#include "nsCOMPtr.h"                   
#include "mozilla/CSSStyleSheet.h"      
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsIDOMDocument.h"             
#include "nsIDocument.h"                
#include "nsIDocumentObserver.h"        
#include "nsIEditor.h"                  
#include "nsStyleSheetTxns.h"

using namespace mozilla;

class nsIStyleSheet;

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
,  mEditor(nullptr)
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(AddStyleSheetTxn, EditTxn,
                                   mSheet)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AddStyleSheetTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
AddStyleSheetTxn::Init(nsIEditor *aEditor, CSSStyleSheet *aSheet)
{
  NS_ENSURE_TRUE(aEditor && aSheet, NS_ERROR_INVALID_ARG);

  mEditor = aEditor;
  mSheet = aSheet;

  return NS_OK;
}


NS_IMETHODIMP
AddStyleSheetTxn::DoTransaction()
{
  NS_ENSURE_TRUE(mEditor && mSheet, NS_ERROR_NOT_INITIALIZED);

  AddStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
AddStyleSheetTxn::UndoTransaction()
{
  NS_ENSURE_TRUE(mEditor && mSheet, NS_ERROR_NOT_INITIALIZED);

  RemoveStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
AddStyleSheetTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("AddStyleSheetTxn");
  return NS_OK;
}


RemoveStyleSheetTxn::RemoveStyleSheetTxn()
:  EditTxn()
,  mEditor(nullptr)
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(RemoveStyleSheetTxn, EditTxn,
                                   mSheet)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(RemoveStyleSheetTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
RemoveStyleSheetTxn::Init(nsIEditor *aEditor, CSSStyleSheet *aSheet)
{
  NS_ENSURE_TRUE(aEditor && aSheet, NS_ERROR_INVALID_ARG);

  mEditor = aEditor;
  mSheet = aSheet;

  return NS_OK;
}


NS_IMETHODIMP
RemoveStyleSheetTxn::DoTransaction()
{
  NS_ENSURE_TRUE(mEditor && mSheet, NS_ERROR_NOT_INITIALIZED);

  RemoveStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
RemoveStyleSheetTxn::UndoTransaction()
{
  NS_ENSURE_TRUE(mEditor && mSheet, NS_ERROR_NOT_INITIALIZED);

  AddStyleSheet(mEditor, mSheet);
  return NS_OK;
}

NS_IMETHODIMP
RemoveStyleSheetTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("RemoveStyleSheetTxn");
  return NS_OK;
}
