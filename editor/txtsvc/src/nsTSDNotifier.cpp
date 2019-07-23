



































#include "nsString.h"
#include "nsIEditActionListener.h"
#include "nsTSDNotifier.h"
#include "nsTextServicesDocument.h"


nsTSDNotifier::nsTSDNotifier(nsTextServicesDocument *aDoc) : mDoc(aDoc)
{
}

nsTSDNotifier::~nsTSDNotifier()
{
  mDoc = 0;
}

#define DEBUG_TSD_NOTIFIER_REFCNT 1

#ifdef DEBUG_TSD_NOTIFIER_REFCNT

nsrefcnt nsTSDNotifier::AddRef(void)
{
  return ++mRefCnt;
}

nsrefcnt nsTSDNotifier::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  if (--mRefCnt == 0) {
    NS_DELETEXPCOM(this);
    return 0;
  }
  return mRefCnt;
}

#else

NS_IMPL_ADDREF(nsTSDNotifier)
NS_IMPL_RELEASE(nsTSDNotifier)

#endif

NS_IMPL_QUERY_INTERFACE1(nsTSDNotifier, nsIEditActionListener)

NS_IMETHODIMP
nsTSDNotifier::WillInsertNode(nsIDOMNode *aNode,
                              nsIDOMNode *aParent,
                              PRInt32     aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidInsertNode(nsIDOMNode *aNode,
                             nsIDOMNode *aParent,
                             PRInt32     aPosition,
                             nsresult    aResult)
{
  if (NS_FAILED(aResult))
    return NS_OK;

  if (!mDoc)
    return NS_ERROR_FAILURE;

  return mDoc->InsertNode(aNode, aParent, aPosition);
}

NS_IMETHODIMP
nsTSDNotifier::WillDeleteNode(nsIDOMNode *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  if (NS_FAILED(aResult))
    return NS_OK;

  if (!mDoc)
    return NS_ERROR_FAILURE;

  return mDoc->DeleteNode(aChild);
}

NS_IMETHODIMP
nsTSDNotifier::WillSplitNode(nsIDOMNode *aExistingRightNode,
                             PRInt32     aOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidSplitNode(nsIDOMNode *aExistingRightNode,
                            PRInt32     aOffset,
                            nsIDOMNode *aNewLeftNode,
                            nsresult    aResult)
{
  if (NS_FAILED(aResult))
    return NS_OK;

  if (!mDoc)
    return NS_ERROR_FAILURE;

  return mDoc->SplitNode(aExistingRightNode, aOffset, aNewLeftNode);
}

NS_IMETHODIMP
nsTSDNotifier::WillJoinNodes(nsIDOMNode  *aLeftNode,
                             nsIDOMNode  *aRightNode,
                             nsIDOMNode  *aParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidJoinNodes(nsIDOMNode  *aLeftNode,
                            nsIDOMNode  *aRightNode,
                            nsIDOMNode  *aParent,
                            nsresult     aResult)
{
  if (NS_FAILED(aResult))
    return NS_OK;

  if (!mDoc)
    return NS_ERROR_FAILURE;

  return mDoc->JoinNodes(aLeftNode, aRightNode, aParent);
}





NS_IMETHODIMP
nsTSDNotifier::WillCreateNode(const nsAString& aTag, nsIDOMNode *aParent, PRInt32 aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidCreateNode(const nsAString& aTag, nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::WillInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::WillDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::WillDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTSDNotifier::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}



