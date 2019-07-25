





































#include "SetDocTitleTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"


SetDocTitleTxn::SetDocTitleTxn()
  : EditTxn()
, mIsTransient(false)
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
  return SetDomTitle(mValue);
}

NS_IMETHODIMP SetDocTitleTxn::UndoTransaction(void)
{
  
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::RedoTransaction(void)
{
  
  return NS_OK;
}

nsresult SetDocTitleTxn::SetDomTitle(const nsAString& aTitle)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  NS_ENSURE_TRUE(editor, NS_ERROR_FAILURE);
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsresult res = editor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNodeList> titleList;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("title"), getter_AddRefs(titleList));
  NS_ENSURE_SUCCESS(res, res);

  
  
  mIsTransient = true;

  nsCOMPtr<nsIDOMNode>titleNode;
  if(titleList)
  {
    res = titleList->Item(0, getter_AddRefs(titleNode));
    NS_ENSURE_SUCCESS(res, res);
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

  
  mIsTransient = false;

  
  nsCOMPtr<nsIDOMNodeList> headList;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("head"),getter_AddRefs(headList));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(headList, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDOMNode>headNode;
  headList->Item(0, getter_AddRefs(headNode));
  NS_ENSURE_TRUE(headNode, NS_ERROR_FAILURE);

  bool     newTitleNode = false;
  PRUint32 newTitleIndex = 0;

  if (!titleNode)
  {
    
    nsCOMPtr<nsIDOMElement>titleElement;
    res = domDoc->CreateElement(NS_LITERAL_STRING("title"), getter_AddRefs(titleElement));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(titleElement, NS_ERROR_FAILURE);

    titleNode = do_QueryInterface(titleElement);
    newTitleNode = true;

    
    
    nsCOMPtr<nsIDOMNodeList> children;
    res = headNode->GetChildNodes(getter_AddRefs(children));
    NS_ENSURE_SUCCESS(res, res);
    if (children)
      children->GetLength(&newTitleIndex);
  }

  
  
  if (titleNode && !aTitle.IsEmpty())
  {
    nsCOMPtr<nsIDOMText> textNode;
    res = domDoc->CreateTextNode(aTitle, getter_AddRefs(textNode));
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(textNode);
    NS_ENSURE_TRUE(newNode, NS_ERROR_FAILURE);

    if (newTitleNode)
    {
      
      nsCOMPtr<nsIDOMNode> resultNode;
      res = titleNode->AppendChild(newNode, getter_AddRefs(resultNode));
    } 
    else 
    {
      
      res = editor->InsertNode(newNode, titleNode, 0);
    }
    NS_ENSURE_SUCCESS(res, res);
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

NS_IMETHODIMP SetDocTitleTxn::GetIsTransient(bool *aIsTransient)
{
  NS_ENSURE_TRUE(aIsTransient, NS_ERROR_NULL_POINTER);  
  *aIsTransient = mIsTransient;
  return NS_OK;
}

