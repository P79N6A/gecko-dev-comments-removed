




































#include "inDeepTreeWalker.h"
#include "inLayoutUtils.h"

#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNodeList.h"
#include "nsServiceManagerUtils.h"
#include "inIDOMUtils.h"









struct DeepTreeStackItem 
{
  DeepTreeStackItem()  { MOZ_COUNT_CTOR(DeepTreeStackItem); }
  ~DeepTreeStackItem() { MOZ_COUNT_DTOR(DeepTreeStackItem); }

  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMNodeList> kids;
  PRUint32 lastIndex;
};



inDeepTreeWalker::inDeepTreeWalker() 
  : mShowAnonymousContent(PR_FALSE),
    mShowSubDocuments(PR_FALSE),
    mWhatToShow(nsIDOMNodeFilter::SHOW_ALL)
{
}

inDeepTreeWalker::~inDeepTreeWalker() 
{ 
  for (PRInt32 i = mStack.Count() - 1; i >= 0; --i) {
    delete static_cast<DeepTreeStackItem*>(mStack[i]);
  }
}

NS_IMPL_ISUPPORTS1(inDeepTreeWalker, inIDeepTreeWalker)




NS_IMETHODIMP
inDeepTreeWalker::GetShowAnonymousContent(PRBool *aShowAnonymousContent)
{
  *aShowAnonymousContent = mShowAnonymousContent;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetShowAnonymousContent(PRBool aShowAnonymousContent)
{
  mShowAnonymousContent = aShowAnonymousContent;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::GetShowSubDocuments(PRBool *aShowSubDocuments)
{
  *aShowSubDocuments = mShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetShowSubDocuments(PRBool aShowSubDocuments)
{
  mShowSubDocuments = aShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::Init(nsIDOMNode* aRoot, PRUint32 aWhatToShow)
{
  mRoot = aRoot;
  mWhatToShow = aWhatToShow;
  
  PushNode(aRoot);

  return NS_OK;
}




NS_IMETHODIMP
inDeepTreeWalker::GetRoot(nsIDOMNode** aRoot)
{
  *aRoot = mRoot;
  NS_IF_ADDREF(*aRoot);
  
  return NS_OK;
}

NS_IMETHODIMP 
inDeepTreeWalker::GetWhatToShow(PRUint32* aWhatToShow)
{
  *aWhatToShow = mWhatToShow;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::GetFilter(nsIDOMNodeFilter** aFilter)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::GetExpandEntityReferences(PRBool* aExpandEntityReferences)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::GetCurrentNode(nsIDOMNode** aCurrentNode)
{
  *aCurrentNode = mCurrentNode;
  NS_IF_ADDREF(*aCurrentNode);
  
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetCurrentNode(nsIDOMNode* aCurrentNode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::ParentNode(nsIDOMNode** _retval)
{
  *_retval = nsnull;
  if (!mCurrentNode) return NS_OK;

  if (!mDOMUtils) {
    mDOMUtils = do_GetService("@mozilla.org/inspector/dom-utils;1");
    if (!mDOMUtils) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  nsresult rv = mDOMUtils->GetParentForNode(mCurrentNode, mShowAnonymousContent,
					    _retval);
  mCurrentNode = *_retval;
  return rv;
}

NS_IMETHODIMP
inDeepTreeWalker::FirstChild(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::LastChild(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousSibling(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::NextSibling(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousNode(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
inDeepTreeWalker::NextNode(nsIDOMNode **_retval)
{
  if (!mCurrentNode) return NS_OK;
  
  nsCOMPtr<nsIDOMNode> next;
  
  while (1) {
    DeepTreeStackItem* top = (DeepTreeStackItem*)mStack.ElementAt(mStack.Count()-1);
    nsCOMPtr<nsIDOMNodeList> kids = top->kids;
    PRUint32 childCount;
    kids->GetLength(&childCount);

    if (top->lastIndex == childCount) {
      mStack.RemoveElementAt(mStack.Count()-1);
      delete top;
      if (mStack.Count() == 0) {
        mCurrentNode = nsnull;
        break;
      }
    } else {
      kids->Item(top->lastIndex++, getter_AddRefs(next));
      PushNode(next);
      break;      
    }
  } 
  
  *_retval = next;
  NS_IF_ADDREF(*_retval);
  
  return NS_OK;
}

void
inDeepTreeWalker::PushNode(nsIDOMNode* aNode)
{
  mCurrentNode = aNode;
  if (!aNode) return;

  DeepTreeStackItem* item = new DeepTreeStackItem();
  item->node = aNode;

  nsCOMPtr<nsIDOMNodeList> kids;
  if (mShowSubDocuments) {
    nsCOMPtr<nsIDOMDocument> domdoc = inLayoutUtils::GetSubDocumentFor(aNode);
    if (domdoc) {
      domdoc->GetChildNodes(getter_AddRefs(kids));
    }
  }
  
  if (!kids) {
    if (mShowAnonymousContent) {
      nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
      nsRefPtr<nsBindingManager> bindingManager;
      if (content &&
          (bindingManager = inLayoutUtils::GetBindingManagerFor(aNode))) {
        bindingManager->GetAnonymousNodesFor(content, getter_AddRefs(kids));
        if (!kids)
          bindingManager->GetContentListFor(content, getter_AddRefs(kids));
      } else {
        aNode->GetChildNodes(getter_AddRefs(kids));
      }
    } else
      aNode->GetChildNodes(getter_AddRefs(kids));
  }
  
  item->kids = kids;
  item->lastIndex = 0;
  mStack.AppendElement((void*)item);
}




















































