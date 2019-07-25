




































#include "nsHTMLEditor.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsIContent.h"
#include "nsHTMLEditUtils.h"
#include "nsReadableUtils.h"





NS_IMETHODIMP
nsHTMLEditor::SetInlineTableEditingEnabled(PRBool aIsEnabled)
{
  mIsInlineTableEditingEnabled = aIsEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetInlineTableEditingEnabled(PRBool * aIsEnabled)
{
  *aIsEnabled = mIsInlineTableEditingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::ShowInlineTableEditingUI(nsIDOMElement * aCell)
{
  NS_ENSURE_ARG_POINTER(aCell);

  
  if (!nsHTMLEditUtils::IsTableCell(aCell))
    return NS_OK;

  if (mInlineEditedCell) {
    NS_ERROR("call HideInlineTableEditingUI first");
    return NS_ERROR_UNEXPECTED;
  }

  
  nsIDOMElement *bodyElement = GetRoot();
  NS_ENSURE_TRUE(bodyElement, NS_ERROR_NULL_POINTER);

  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableAddColumnBefore"),
                         PR_FALSE, getter_AddRefs(mAddColumnBeforeButton));
  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableRemoveColumn"),
                         PR_FALSE, getter_AddRefs(mRemoveColumnButton));
  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableAddColumnAfter"),
                         PR_FALSE, getter_AddRefs(mAddColumnAfterButton));

  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableAddRowBefore"),
                         PR_FALSE, getter_AddRefs(mAddRowBeforeButton));
  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableRemoveRow"),
                         PR_FALSE, getter_AddRefs(mRemoveRowButton));
  CreateAnonymousElement(NS_LITERAL_STRING("a"), bodyElement,
                         NS_LITERAL_STRING("mozTableAddRowAfter"),
                         PR_FALSE, getter_AddRefs(mAddRowAfterButton));

  AddMouseClickListener(mAddColumnBeforeButton);
  AddMouseClickListener(mRemoveColumnButton);
  AddMouseClickListener(mAddColumnAfterButton);
  AddMouseClickListener(mAddRowBeforeButton);
  AddMouseClickListener(mRemoveRowButton);
  AddMouseClickListener(mAddRowAfterButton);

  mInlineEditedCell = aCell;
  return RefreshInlineTableEditingUI();
}

NS_IMETHODIMP
nsHTMLEditor::HideInlineTableEditingUI()
{
  mInlineEditedCell = nsnull;

  RemoveMouseClickListener(mAddColumnBeforeButton);
  RemoveMouseClickListener(mRemoveColumnButton);
  RemoveMouseClickListener(mAddColumnAfterButton);
  RemoveMouseClickListener(mAddRowBeforeButton);
  RemoveMouseClickListener(mRemoveRowButton);
  RemoveMouseClickListener(mAddRowAfterButton);

  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  
  
  

  

  nsIDOMElement *bodyElement = GetRoot();

  nsCOMPtr<nsIContent> bodyContent( do_QueryInterface(bodyElement) );
  NS_ENSURE_TRUE(bodyContent, NS_ERROR_FAILURE);

  DeleteRefToAnonymousNode(mAddColumnBeforeButton, bodyContent, ps);
  mAddColumnBeforeButton = nsnull;
  DeleteRefToAnonymousNode(mRemoveColumnButton, bodyContent, ps);
  mRemoveColumnButton = nsnull;
  DeleteRefToAnonymousNode(mAddColumnAfterButton, bodyContent, ps);
  mAddColumnAfterButton = nsnull;
  DeleteRefToAnonymousNode(mAddRowBeforeButton, bodyContent, ps);
  mAddRowBeforeButton = nsnull;
  DeleteRefToAnonymousNode(mRemoveRowButton, bodyContent, ps);
  mRemoveRowButton = nsnull;
  DeleteRefToAnonymousNode(mAddRowAfterButton, bodyContent, ps);
  mAddRowAfterButton = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DoInlineTableEditingAction(nsIDOMElement * aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  PRBool anonElement = PR_FALSE;
  if (aElement &&
      NS_SUCCEEDED(aElement->HasAttribute(NS_LITERAL_STRING("_moz_anonclass"), &anonElement)) &&
      anonElement) {
    nsAutoString anonclass;
    nsresult res = aElement->GetAttribute(NS_LITERAL_STRING("_moz_anonclass"), anonclass);
    NS_ENSURE_SUCCESS(res, res);

    if (!StringBeginsWith(anonclass, NS_LITERAL_STRING("mozTable")))
      return NS_OK;

    nsCOMPtr<nsIDOMNode> tableNode = GetEnclosingTable(mInlineEditedCell);
    nsCOMPtr<nsIDOMElement> tableElement = do_QueryInterface(tableNode);
    PRInt32 rowCount, colCount;
    res = GetTableSize(tableElement, &rowCount, &colCount);
    NS_ENSURE_SUCCESS(res, res);

    PRBool hideUI = PR_FALSE;
    PRBool hideResizersWithInlineTableUI = (mResizedObject == tableElement);

    if (anonclass.EqualsLiteral("mozTableAddColumnBefore"))
      InsertTableColumn(1, PR_FALSE);
    else if (anonclass.EqualsLiteral("mozTableAddColumnAfter"))
      InsertTableColumn(1, PR_TRUE);
    else if (anonclass.EqualsLiteral("mozTableAddRowBefore"))
      InsertTableRow(1, PR_FALSE);
    else if (anonclass.EqualsLiteral("mozTableAddRowAfter"))
      InsertTableRow(1, PR_TRUE);
    else if (anonclass.EqualsLiteral("mozTableRemoveColumn")) {
      DeleteTableColumn(1);
#ifndef DISABLE_TABLE_DELETION
      hideUI = (colCount == 1);
#endif
    }
    else if (anonclass.EqualsLiteral("mozTableRemoveRow")) {
      DeleteTableRow(1);
#ifndef DISABLE_TABLE_DELETION
      hideUI = (rowCount == 1);
#endif
    }
    else
      return NS_OK;

    if (hideUI) {
      HideInlineTableEditingUI();
      if (hideResizersWithInlineTableUI)
        HideResizers();
    }
  }

  return NS_OK;
}

void
nsHTMLEditor::AddMouseClickListener(nsIDOMElement * aElement)
{
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aElement));
  if (evtTarget) {
    evtTarget->AddEventListener(NS_LITERAL_STRING("click"),
                                mEventListener, PR_TRUE);
  }
}

void
nsHTMLEditor::RemoveMouseClickListener(nsIDOMElement * aElement)
{
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aElement));
  if (evtTarget) {
    evtTarget->RemoveEventListener(NS_LITERAL_STRING("click"),
                                   mEventListener, PR_TRUE);
  }
}

NS_IMETHODIMP
nsHTMLEditor::RefreshInlineTableEditingUI()
{
  nsCOMPtr<nsIDOMNSHTMLElement> nsElement = do_QueryInterface(mInlineEditedCell);
  if (!nsElement) {return NS_ERROR_NULL_POINTER; }

  PRInt32 xCell, yCell, wCell, hCell;
  GetElementOrigin(mInlineEditedCell, xCell, yCell);

  nsresult res = nsElement->GetOffsetWidth(&wCell);
  NS_ENSURE_SUCCESS(res, res);
  res = nsElement->GetOffsetHeight(&hCell);
  NS_ENSURE_SUCCESS(res, res);

  PRInt32 xHoriz = xCell + wCell/2;
  PRInt32 yVert  = yCell + hCell/2;

  nsCOMPtr<nsIDOMNode> tableNode = GetEnclosingTable(mInlineEditedCell);
  nsCOMPtr<nsIDOMElement> tableElement = do_QueryInterface(tableNode);
  PRInt32 rowCount, colCount;
  res = GetTableSize(tableElement, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  SetAnonymousElementPosition(xHoriz-10, yCell-7,  mAddColumnBeforeButton);
#ifdef DISABLE_TABLE_DELETION
  NS_NAMED_LITERAL_STRING(classStr, "class");

  if (colCount== 1) {
    mRemoveColumnButton->SetAttribute(classStr,
                                      NS_LITERAL_STRING("hidden"));
  }
  else {
    PRBool hasClass = PR_FALSE;
    res = mRemoveColumnButton->HasAttribute(classStr, &hasClass);
    if (NS_SUCCEEDED(res) && hasClass)
      mRemoveColumnButton->RemoveAttribute(classStr);
#endif
    SetAnonymousElementPosition(xHoriz-4, yCell-7,  mRemoveColumnButton);
#ifdef DISABLE_TABLE_DELETION
  }
#endif
  SetAnonymousElementPosition(xHoriz+6, yCell-7,  mAddColumnAfterButton);

  SetAnonymousElementPosition(xCell-7, yVert-10,  mAddRowBeforeButton);
#ifdef DISABLE_TABLE_DELETION
  if (rowCount== 1) {
    mRemoveRowButton->SetAttribute(classStr,
                                   NS_LITERAL_STRING("hidden"));
  }
  else {
    PRBool hasClass = PR_FALSE;
    res = mRemoveRowButton->HasAttribute(classStr, &hasClass);
    if (NS_SUCCEEDED(res) && hasClass)
      mRemoveRowButton->RemoveAttribute(classStr);
#endif
    SetAnonymousElementPosition(xCell-7, yVert-4,  mRemoveRowButton);
#ifdef DISABLE_TABLE_DELETION
  }
#endif
  SetAnonymousElementPosition(xCell-7, yVert+6,  mAddRowAfterButton);

  return NS_OK;
}

