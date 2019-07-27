





#include "inDeepTreeWalker.h"
#include "inLayoutUtils.h"

#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNodeList.h"
#include "nsServiceManagerUtils.h"
#include "inIDOMUtils.h"
#include "nsIContent.h"
#include "nsContentList.h"
#include "ChildIterator.h"
#include "mozilla/dom/Element.h"









inDeepTreeWalker::inDeepTreeWalker()
  : mShowAnonymousContent(false),
    mShowSubDocuments(false),
    mShowDocumentsAsNodes(false),
    mWhatToShow(nsIDOMNodeFilter::SHOW_ALL)
{
}

inDeepTreeWalker::~inDeepTreeWalker()
{
}

NS_IMPL_ISUPPORTS(inDeepTreeWalker,
                  inIDeepTreeWalker)




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
inDeepTreeWalker::GetShowDocumentsAsNodes(bool *aShowDocumentsAsNodes)
{
  *aShowDocumentsAsNodes = mShowDocumentsAsNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::SetShowDocumentsAsNodes(bool aShowDocumentsAsNodes)
{
  mShowDocumentsAsNodes = aShowDocumentsAsNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::Init(nsIDOMNode* aRoot, uint32_t aWhatToShow)
{
  if (!aRoot) {
    return NS_ERROR_INVALID_ARG;
  }

  mRoot = aRoot;
  mCurrentNode = aRoot;
  mWhatToShow = aWhatToShow;

  mDOMUtils = do_GetService("@mozilla.org/inspector/dom-utils;1");
  return mDOMUtils ? NS_OK : NS_ERROR_UNEXPECTED;
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

already_AddRefed<nsIDOMNode>
inDeepTreeWalker::GetParent()
{
  if (mCurrentNode == mRoot) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMNode> parent;
  MOZ_ASSERT(mDOMUtils, "mDOMUtils should have been initiated already in Init");
  mDOMUtils->GetParentForNode(mCurrentNode, mShowAnonymousContent,
                              getter_AddRefs(parent));

  uint16_t nodeType = 0;
  if (parent) {
    parent->GetNodeType(&nodeType);
  }
  
  
  if (!mShowDocumentsAsNodes &&
      nodeType == nsIDOMNode::DOCUMENT_NODE &&
      parent != mRoot) {
    mDOMUtils->GetParentForNode(parent, mShowAnonymousContent,
                                getter_AddRefs(parent));
  }

  return parent.forget();
}

static already_AddRefed<nsINodeList>
GetChildren(nsIDOMNode* aParent,
            bool aShowAnonymousContent,
            bool aShowSubDocuments)
{
  MOZ_ASSERT(aParent);

  nsCOMPtr<nsINodeList> ret;
  if (aShowSubDocuments) {
    nsCOMPtr<nsIDOMDocument> domdoc = inLayoutUtils::GetSubDocumentFor(aParent);
    if (domdoc) {
      aParent = domdoc;
    }
  }

  nsCOMPtr<nsIContent> parentAsContent = do_QueryInterface(aParent);
  if (parentAsContent && aShowAnonymousContent) {
      ret = parentAsContent->GetChildren(nsIContent::eAllChildren);
  } else {
    
    
    
    nsCOMPtr<nsINode> parentNode = do_QueryInterface(aParent);
    MOZ_ASSERT(parentNode);
    ret = parentNode->ChildNodes();
  }
  return ret.forget();
}

NS_IMETHODIMP
inDeepTreeWalker::SetCurrentNode(nsIDOMNode* aCurrentNode)
{
  
  
  if (!mCurrentNode || !aCurrentNode) {
    return NS_ERROR_FAILURE;
  }

  
  
  uint16_t nodeType = 0;
  aCurrentNode->GetNodeType(&nodeType);
  if (!mShowDocumentsAsNodes && nodeType == nsIDOMNode::DOCUMENT_NODE) {
    return NS_ERROR_FAILURE;
  }

  return SetCurrentNode(aCurrentNode, nullptr);
}


nsresult
inDeepTreeWalker::SetCurrentNode(nsIDOMNode* aCurrentNode,
                                 nsINodeList* aSiblings)
{
  MOZ_ASSERT(aCurrentNode);

  
  
  nsCOMPtr<nsINodeList> tmpSiblings = mSiblings;
  nsCOMPtr<nsIDOMNode> tmpCurrent = mCurrentNode;
  mSiblings = aSiblings;
  mCurrentNode = aCurrentNode;

  
  
  
  
  
  
  uint16_t nodeType = 0;
  aCurrentNode->GetNodeType(&nodeType);
  if (!mSiblings && nodeType != nsIDOMNode::DOCUMENT_NODE) {
    nsCOMPtr<nsIDOMNode> parent = GetParent();
    if (parent) {
      mSiblings = GetChildren(parent,
                              mShowAnonymousContent,
                              mShowSubDocuments);
    }
  }

  if (mSiblings && mSiblings->Length()) {
    
    
    nsCOMPtr<nsIContent> currentAsContent = do_QueryInterface(mCurrentNode);
    MOZ_ASSERT(currentAsContent);
    int32_t index = mSiblings->IndexOf(currentAsContent);
    if (index < 0) {
      
      
      

      
      mCurrentNode = tmpCurrent;
      mSiblings = tmpSiblings;
      return NS_ERROR_INVALID_ARG;
    }
    mCurrentIndex = index;
  } else {
    mCurrentIndex = -1;
  }
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::ParentNode(nsIDOMNode** _retval)
{
  *_retval = nullptr;
  if (!mCurrentNode || mCurrentNode == mRoot) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parent = GetParent();

  if (!parent) {
    return NS_OK;
  }

  nsresult rv = SetCurrentNode(parent);
  NS_ENSURE_SUCCESS(rv,rv);

  parent.forget(_retval);
  return NS_OK;
}



nsresult
inDeepTreeWalker::EdgeChild(nsIDOMNode** _retval, bool aFront)
{
  if (!mCurrentNode) {
    return NS_ERROR_FAILURE;
  }

  *_retval = nullptr;

  nsCOMPtr<nsIDOMNode> echild;
  if (mShowSubDocuments && mShowDocumentsAsNodes) {
    
    
    
    echild = inLayoutUtils::GetSubDocumentFor(mCurrentNode);
  }

  nsCOMPtr<nsINodeList> children;
  if (!echild) {
    children = GetChildren(mCurrentNode,
                           mShowAnonymousContent,
                           mShowSubDocuments);
    if (children && children->Length() > 0) {
      nsINode* childNode = children->Item(aFront ? 0 : children->Length() - 1);
      echild = childNode ? childNode->AsDOMNode() : nullptr;
    }
  }

  if (echild) {
    nsresult rv = SetCurrentNode(echild, children);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF(*_retval = mCurrentNode);
  }

  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::FirstChild(nsIDOMNode** _retval)
{
  return EdgeChild(_retval,  true);
}

NS_IMETHODIMP
inDeepTreeWalker::LastChild(nsIDOMNode **_retval)
{
  return EdgeChild(_retval,  false);
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousSibling(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode || !mSiblings || mCurrentIndex < 1) {
    return NS_OK;
  }

  nsIContent* prev = mSiblings->Item(--mCurrentIndex);
  mCurrentNode = prev->AsDOMNode();
  NS_ADDREF(*_retval = mCurrentNode);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::NextSibling(nsIDOMNode **_retval)
{
  *_retval = nullptr;
  if (!mCurrentNode || !mSiblings ||
      mCurrentIndex + 1 >= (int32_t) mSiblings->Length()) {
    return NS_OK;
  }

  nsIContent* next = mSiblings->Item(++mCurrentIndex);
  mCurrentNode = next->AsDOMNode();
  NS_ADDREF(*_retval = mCurrentNode);
  return NS_OK;
}

NS_IMETHODIMP
inDeepTreeWalker::PreviousNode(nsIDOMNode **_retval)
{
  if (!mCurrentNode || mCurrentNode == mRoot) {
    
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
  if (!mCurrentNode) {
    return NS_OK;
  }

  
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
