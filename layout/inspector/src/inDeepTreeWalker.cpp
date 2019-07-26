





#include "inDeepTreeWalker.h"
#include "inLayoutUtils.h"

#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNodeList.h"
#include "nsServiceManagerUtils.h"
#include "inIDOMUtils.h"
#include "nsIContent.h"
#include "nsBindingManager.h"









inDeepTreeWalker::inDeepTreeWalker() 
  : mShowAnonymousContent(false),
    mShowSubDocuments(false),
    mWhatToShow(nsIDOMNodeFilter::SHOW_ALL)
{
}

inDeepTreeWalker::~inDeepTreeWalker() 
{ 
}

NS_IMPL_ISUPPORTS2(inDeepTreeWalker,
                   inIDeepTreeWalker,
                   nsIDOMTreeWalker)




NS_IMETHODIMP
inDeepTreeWalker::GetShowAnonymousContent(bool *aShowAnonymousContent)
{
  *aShowAnonymousContent = mShowAnonymousContent;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetShowAnonymousContent(bool aShowAnonymousContent)
{
  mShowAnonymousContent = aShowAnonymousContent;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::GetShowSubDocuments(bool *aShowSubDocuments)
{
  *aShowSubDocuments = mShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetShowSubDocuments(bool aShowSubDocuments)
{
  mShowSubDocuments = aShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::Init(nsIDOMNode* aRoot, uint32_t aWhatToShow)
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
inDeepTreeWalker::GetWhatToShow(uint32_t* aWhatToShow)
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
  *_retval = nullptr;
  if (!mCurrentNode) return NS_OK;

  if (mStack.Length() == 1) {
    
    return NS_OK;
  }

  
  mStack.RemoveElementAt(mStack.Length()-1);
  DeepTreeStackItem& top = mStack.ElementAt(mStack.Length() - 1);
  mCurrentNode = top.node;
  top.lastIndex = 0;
  NS_ADDREF(*_retval = mCurrentNode);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::FirstChild(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode) {
    return NS_OK;
  }

  DeepTreeStackItem& top = mStack.ElementAt(mStack.Length() - 1);
  nsCOMPtr<nsIDOMNode> kid;
  top.kids->Item(0, getter_AddRefs(kid));
  if (!kid) {
    return NS_OK;
  }
  top.lastIndex = 1;
  PushNode(kid);
  kid.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::LastChild(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode) {
    return NS_OK;
  }

  DeepTreeStackItem& top = mStack.ElementAt(mStack.Length() - 1);
  nsCOMPtr<nsIDOMNode> kid;
  uint32_t length;
  top.kids->GetLength(&length);
  top.kids->Item(length - 1, getter_AddRefs(kid));
  if (!kid) {
    return NS_OK;
  }
  top.lastIndex = length;
  PushNode(kid);
  kid.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousSibling(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode) {
    return NS_OK;
  }

  NS_ASSERTION(mStack.Length() > 0, "Should have things in mStack");

  if (mStack.Length() == 1) {
    
    return NS_OK;
  }

  DeepTreeStackItem& parent = mStack.ElementAt(mStack.Length()-2);
  nsCOMPtr<nsIDOMNode> previousSibling;
  parent.kids->Item(parent.lastIndex-2, getter_AddRefs(previousSibling));
  if (!previousSibling) {
    return NS_OK;
  }

  
  
  
  mStack.RemoveElementAt(mStack.Length() - 1);
  parent.lastIndex--;
  PushNode(previousSibling);
  previousSibling.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::NextSibling(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode) {
    return NS_OK;
  }

  NS_ASSERTION(mStack.Length() > 0, "Should have things in mStack");

  if (mStack.Length() == 1) {
    
    return NS_OK;
  }

  DeepTreeStackItem& parent = mStack.ElementAt(mStack.Length()-2);
  nsCOMPtr<nsIDOMNode> nextSibling;
  parent.kids->Item(parent.lastIndex, getter_AddRefs(nextSibling));
  if (!nextSibling) {
    return NS_OK;
  }

  
  
  
  mStack.RemoveElementAt(mStack.Length() - 1);
  parent.lastIndex++;
  PushNode(nextSibling);
  nextSibling.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousNode(nsIDOMNode **_retval)
{
  if (!mCurrentNode || mStack.Length() == 1) {
    
    *_retval = nullptr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> node;
  PreviousSibling(getter_AddRefs(node));

  if (!node) {
    return ParentNode(_retval);
  }

  
  
  
  
  
  while (node) {
    LastChild(getter_AddRefs(node));
  }

  NS_ADDREF(*_retval = mCurrentNode);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::NextNode(nsIDOMNode **_retval)
{
  
  FirstChild(_retval);

  if (*_retval) {
    return NS_OK;
  }

  
  
#ifdef DEBUG
  nsIDOMNode* origCurrentNode = mCurrentNode;
#endif
  uint32_t lastChildCallsToMake = 0;
  while (1) {
    NextSibling(_retval);

    if (*_retval) {
      return NS_OK;
    }

    nsCOMPtr<nsIDOMNode> parent;
    ParentNode(getter_AddRefs(parent));
    if (!parent) {
      
      while (lastChildCallsToMake--) {
        nsCOMPtr<nsIDOMNode> dummy;
        LastChild(getter_AddRefs(dummy));
      }
      NS_ASSERTION(mCurrentNode == origCurrentNode,
                   "Didn't go back to the right node?");
      *_retval = nullptr;
      return NS_OK;
    }
    ++lastChildCallsToMake;
  }

  NS_NOTREACHED("how did we get here?");
  return NS_OK;
}

void
inDeepTreeWalker::PushNode(nsIDOMNode* aNode)
{
  mCurrentNode = aNode;
  if (!aNode) return;

  DeepTreeStackItem item;
  item.node = aNode;

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
  
  item.kids = kids;
  item.lastIndex = 0;
  mStack.AppendElement(item);
}




















































