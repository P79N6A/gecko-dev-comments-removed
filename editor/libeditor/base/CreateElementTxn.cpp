




































#include "CreateElementTxn.h"
#include "nsEditor.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsISelection.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsReadableUtils.h"


#include "nsIContent.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif

CreateElementTxn::CreateElementTxn()
  : EditTxn()
{
}

NS_IMETHODIMP CreateElementTxn::Init(nsEditor      *aEditor,
                                     const nsAString &aTag,
                                     nsIDOMNode     *aParent,
                                     PRUint32        aOffsetInParent)
{
  NS_ASSERTION(aEditor&&aParent, "null args");
  if (!aEditor || !aParent) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mTag = aTag;
  mParent = do_QueryInterface(aParent);
  mOffsetInParent = aOffsetInParent;
#ifdef NS_DEBUG
  {
    nsCOMPtr<nsIDOMNodeList> testChildNodes;
    nsresult testResult = mParent->GetChildNodes(getter_AddRefs(testChildNodes));
    NS_ASSERTION(testChildNodes, "bad parent type, can't have children.");
    NS_ASSERTION(NS_SUCCEEDED(testResult), "bad result.");
  }
#endif
  return NS_OK;
}


NS_IMETHODIMP CreateElementTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy)
  {
    char* nodename = ToNewCString(mTag);
    printf("Do Create Element parent = %p <%s>, offset = %d\n", 
           mParent.get(), nodename, mOffsetInParent);
    nsMemory::Free(nodename);
  }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  if (!mEditor || !mParent) return NS_ERROR_NOT_INITIALIZED;
  
  nsAutoString textNodeTag;
  nsresult result = nsEditor::GetTextNodeTag(textNodeTag);
  if (NS_FAILED(result)) { return result; }

  if (textNodeTag == mTag) 
  {
    nsCOMPtr<nsIDOMDocument>doc;
    result = mEditor->GetDocument(getter_AddRefs(doc));
    if (NS_FAILED(result)) return result;
    if (!doc) return NS_ERROR_NULL_POINTER;

    const nsString stringData;
    nsCOMPtr<nsIDOMText>newTextNode;
    result = doc->CreateTextNode(stringData, getter_AddRefs(newTextNode));
    if (NS_FAILED(result)) return result;
    if (!newTextNode) return NS_ERROR_NULL_POINTER;
    mNewNode = do_QueryInterface(newTextNode);
  }
  else 
  {
    nsCOMPtr<nsIContent> newContent;
 
    
    result = mEditor->CreateHTMLContent(mTag, getter_AddRefs(newContent));
    if (NS_FAILED(result)) return result;
    nsCOMPtr<nsIDOMElement>newElement = do_QueryInterface(newContent);
    if (!newElement) return NS_ERROR_NULL_POINTER;
    mNewNode = do_QueryInterface(newElement);
    
    mEditor->MarkNodeDirty(mNewNode);
  }
  NS_ASSERTION(((NS_SUCCEEDED(result)) && (mNewNode)), "could not create element.");
  if (!mNewNode) return NS_ERROR_NULL_POINTER;

#ifdef NS_DEBUG
  if (gNoisy) { printf("  newNode = %p\n", mNewNode.get()); }
#endif

  
  nsCOMPtr<nsIDOMNode> resultNode;
  if (CreateElementTxn::eAppend==(PRInt32)mOffsetInParent)
  {
    result = mParent->AppendChild(mNewNode, getter_AddRefs(resultNode));
  }
  else
  {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    result = mParent->GetChildNodes(getter_AddRefs(childNodes));
    if ((NS_SUCCEEDED(result)) && (childNodes))
    {
      PRUint32 count;
      childNodes->GetLength(&count);
      if (mOffsetInParent>count)
        mOffsetInParent = count;
      result = childNodes->Item(mOffsetInParent, getter_AddRefs(mRefNode));
      if (NS_FAILED(result)) return result; 

      result = mParent->InsertBefore(mNewNode, mRefNode, getter_AddRefs(resultNode));
      if (NS_FAILED(result)) return result; 

      
      PRBool bAdjustSelection;
      mEditor->ShouldTxnSetSelection(&bAdjustSelection);
      if (bAdjustSelection)
      {
        nsCOMPtr<nsISelection> selection;
        result = mEditor->GetSelection(getter_AddRefs(selection));
        if (NS_FAILED(result)) return result;
        if (!selection) return NS_ERROR_NULL_POINTER;

        PRInt32 offset=0;
        result = nsEditor::GetChildOffset(mNewNode, mParent, offset);
        if (NS_FAILED(result)) return result;

        result = selection->Collapse(mParent, offset+1);
        NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after insert.");
       }
      else
      {
        
      }
    }
  }
  return result;
}

NS_IMETHODIMP CreateElementTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Undo Create Element, mParent = %p, node = %p\n",
                        mParent.get(), mNewNode.get()); }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  if (!mEditor || !mParent) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->RemoveChild(mNewNode, getter_AddRefs(resultNode));
}

NS_IMETHODIMP CreateElementTxn::RedoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Redo Create Element\n"); }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  if (!mEditor || !mParent) return NS_ERROR_NOT_INITIALIZED;

  
  nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(mNewNode);
  if (nodeAsText)
  {
    nodeAsText->SetData(EmptyString());
  }
  
  
  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->InsertBefore(mNewNode, mRefNode, getter_AddRefs(resultNode));
}

NS_IMETHODIMP CreateElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("CreateElementTxn: ");
  aString += mTag;
  return NS_OK;
}

NS_IMETHODIMP CreateElementTxn::GetNewNode(nsIDOMNode **aNewNode)
{
  if (!aNewNode)
    return NS_ERROR_NULL_POINTER;
  if (!mNewNode)
    return NS_ERROR_NOT_INITIALIZED;
  *aNewNode = mNewNode;
  NS_ADDREF(*aNewNode);
  return NS_OK;
}
