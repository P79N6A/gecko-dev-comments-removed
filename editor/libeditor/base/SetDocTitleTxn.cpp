





































#include "SetDocTitleTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"


SetDocTitleTxn::SetDocTitleTxn()
  : EditTxn()
, mIsTransient(PR_FALSE)
{
}

NS_IMETHODIMP SetDocTitleTxn::Init(nsIHTMLEditor *aEditor,
                                   const nsAString *aValue)

{
  NS_ASSERTION(aEditor && aValue, "null args");
  if (!aEditor || !aValue) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mValue = *aValue;

  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::DoTransaction(void)
{
  nsresult res = SetDomTitle(mValue);
  if (NS_FAILED(res)) return res;

  return SetDocTitle(mValue);
}

NS_IMETHODIMP SetDocTitleTxn::UndoTransaction(void)
{
  return SetDocTitle(mUndoValue);
}

NS_IMETHODIMP SetDocTitleTxn::RedoTransaction(void)
{
  return SetDocTitle(mValue);
}

nsresult SetDocTitleTxn::SetDocTitle(const nsAString& aTitle)
{
  NS_ASSERTION(mEditor, "bad state");
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsresult rv = editor->GetDocument(getter_AddRefs(domDoc));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDOMHTMLDocument> HTMLDoc = do_QueryInterface(domDoc);
  if (!HTMLDoc) return NS_ERROR_FAILURE;

  return HTMLDoc->SetTitle(aTitle);
}

nsresult SetDocTitleTxn::SetDomTitle(const nsAString& aTitle)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsresult res = editor->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNodeList> titleList;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("title"), getter_AddRefs(titleList));
  if (NS_FAILED(res)) return res;

  
  
  mIsTransient = PR_TRUE;

  nsCOMPtr<nsIDOMNode>titleNode;
  if(titleList)
  {
    res = titleList->Item(0, getter_AddRefs(titleNode));
    if (NS_FAILED(res)) return res;
    if (titleNode)
    {
      
      
      nsCOMPtr<nsIDOMNode> child;
      res = titleNode->GetFirstChild(getter_AddRefs(child));
      if(NS_FAILED(res)) return res;
      if(child)
      {
        
        nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(child);
        if(textNode)
        {
          textNode->GetData(mUndoValue);

          
          
          if (mUndoValue == aTitle)
            return NS_OK;
        }
        res = editor->DeleteNode(child);
        if(NS_FAILED(res)) return res;
      }
    }
  }

  
  mIsTransient = PR_FALSE;

  
  nsCOMPtr<nsIDOMNodeList> headList;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("head"),getter_AddRefs(headList));
  if (NS_FAILED(res)) return res;
  if (!headList) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMNode>headNode;
  headList->Item(0, getter_AddRefs(headNode));
  if (!headNode) return NS_ERROR_FAILURE;

  PRBool   newTitleNode = PR_FALSE;
  PRUint32 newTitleIndex = 0;

  if (!titleNode)
  {
    
    nsCOMPtr<nsIDOMElement>titleElement;
    res = domDoc->CreateElement(NS_LITERAL_STRING("title"), getter_AddRefs(titleElement));
    if (NS_FAILED(res)) return res;
    if (!titleElement) return NS_ERROR_FAILURE;

    titleNode = do_QueryInterface(titleElement);
    newTitleNode = PR_TRUE;

    
    
    nsCOMPtr<nsIDOMNodeList> children;
    res = headNode->GetChildNodes(getter_AddRefs(children));
    if (NS_FAILED(res)) return res;
    if (children)
      children->GetLength(&newTitleIndex);
  }

  
  
  if (titleNode && !aTitle.IsEmpty())
  {
    nsCOMPtr<nsIDOMText> textNode;
    res = domDoc->CreateTextNode(aTitle, getter_AddRefs(textNode));
    if (NS_FAILED(res)) return res;
    nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(textNode);
    if (!newNode) return NS_ERROR_FAILURE;

    if (newTitleNode)
    {
      
      nsCOMPtr<nsIDOMNode> resultNode;
      res = titleNode->AppendChild(newNode, getter_AddRefs(resultNode));
    } 
    else 
    {
      
      res = editor->InsertNode(newNode, titleNode, 0);
    }
    if (NS_FAILED(res)) return res;
  }

  if (newTitleNode)
  {
    
    res = editor->InsertNode(titleNode, headNode, newTitleIndex);
  }
  return res;
}

NS_IMETHODIMP SetDocTitleTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("SetDocTitleTxn: ");
  aString += mValue;
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::GetIsTransient(PRBool *aIsTransient)
{
  if (!aIsTransient) return NS_ERROR_NULL_POINTER;  
  *aIsTransient = mIsTransient;
  return NS_OK;
}

