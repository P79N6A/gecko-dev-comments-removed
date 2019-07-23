




































#include "nsFilteredContentIterator.h"
#include "nsIContentIterator.h"
#include "nsComponentManagerUtils.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsIEnumerator.h"

#include "nsTextServicesDocument.h"

#include "nsIDOMNode.h"
#include "nsIDOMRange.h"


nsFilteredContentIterator::nsFilteredContentIterator(nsITextServicesFilter* aFilter) :
  mFilter(aFilter),
  mDidSkip(PR_FALSE),
  mIsOutOfRange(PR_FALSE),
  mDirection(eDirNotSet)
{
  mIterator = do_CreateInstance("@mozilla.org/content/post-content-iterator;1");
  mPreIterator = do_CreateInstance("@mozilla.org/content/pre-content-iterator;1");
}


nsFilteredContentIterator::~nsFilteredContentIterator()
{
}


NS_IMPL_ISUPPORTS1(nsFilteredContentIterator, nsIContentIterator)


nsresult
nsFilteredContentIterator::Init(nsIContent* aRoot)
{
  NS_ENSURE_TRUE(mPreIterator, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);
  mIsOutOfRange    = PR_FALSE;
  mDirection       = eForward;
  mCurrentIterator = mPreIterator;

  nsresult rv;
  mRange = do_CreateInstance("@mozilla.org/content/range;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMRange> domRange(do_QueryInterface(mRange));
  nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(aRoot));
  if (domRange && domNode) {
    domRange->SelectNode(domNode);
  }

  rv = mPreIterator->Init(domRange);
  NS_ENSURE_SUCCESS(rv, rv);
  return mIterator->Init(domRange);
}


nsresult
nsFilteredContentIterator::Init(nsIDOMRange* aRange)
{
  NS_ENSURE_TRUE(mPreIterator, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(aRange);
  mIsOutOfRange    = PR_FALSE;
  mDirection       = eForward;
  mCurrentIterator = mPreIterator;

  nsCOMPtr<nsIDOMRange> domRange;
  nsresult rv = aRange->CloneRange(getter_AddRefs(domRange));
  NS_ENSURE_SUCCESS(rv, rv);
  mRange = do_QueryInterface(domRange);

  rv = mPreIterator->Init(domRange);
  NS_ENSURE_SUCCESS(rv, rv);
  return mIterator->Init(domRange);
}


nsresult 
nsFilteredContentIterator::SwitchDirections(PRPackedBool aChangeToForward)
{
  nsIContent *node = mCurrentIterator->GetCurrentNode();

  if (aChangeToForward) {
    mCurrentIterator = mPreIterator;
    mDirection       = eForward;
  } else {
    mCurrentIterator = mIterator;
    mDirection       = eBackward;
  }

  if (node) {
    nsresult rv = mCurrentIterator->PositionAt(node);
    if (NS_FAILED(rv)) {
      mIsOutOfRange = PR_TRUE;
      return rv;
    }
  }
  return NS_OK;
}


void
nsFilteredContentIterator::First()
{
  if (!mCurrentIterator) {
    NS_ERROR("Missing iterator!");

    return;
  }

  
  
  if (mDirection != eForward) {
    mCurrentIterator = mPreIterator;
    mDirection       = eForward;
    mIsOutOfRange    = PR_FALSE;
  }

  mCurrentIterator->First();

  if (mCurrentIterator->IsDone()) {
    return;
  }

  nsIContent *currentContent = mCurrentIterator->GetCurrentNode();
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(currentContent));

  PRPackedBool didCross;
  CheckAdvNode(node, didCross, eForward);
}


void
nsFilteredContentIterator::Last()
{
  if (!mCurrentIterator) {
    NS_ERROR("Missing iterator!");

    return;
  }

  
  
  if (mDirection != eBackward) {
    mCurrentIterator = mIterator;
    mDirection       = eBackward;
    mIsOutOfRange    = PR_FALSE;
  }

  mCurrentIterator->Last();

  if (mCurrentIterator->IsDone()) {
    return;
  }

  nsIContent *currentContent = mCurrentIterator->GetCurrentNode();
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(currentContent));

  PRPackedBool didCross;
  CheckAdvNode(node, didCross, eBackward);
}




static void
ContentToParentOffset(nsIContent *aContent, nsIDOMNode **aParent,
                      PRInt32 *aOffset)
{
  if (!aParent || !aOffset)
    return;

  *aParent = nsnull;
  *aOffset  = 0;

  if (!aContent)
    return;

  nsIContent* parent = aContent->GetParent();

  if (!parent)
    return;

  *aOffset = parent->IndexOf(aContent);

  CallQueryInterface(parent, aParent);
}





static PRBool
ContentIsInTraversalRange(nsIContent *aContent,   PRBool aIsPreMode,
                          nsIDOMNode *aStartNode, PRInt32 aStartOffset,
                          nsIDOMNode *aEndNode,   PRInt32 aEndOffset)
{
  if (!aStartNode || !aEndNode || !aContent)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> parentNode;
  PRInt32 indx = 0;

  ContentToParentOffset(aContent, getter_AddRefs(parentNode), &indx);

  if (!parentNode)
    return PR_FALSE;

  if (!aIsPreMode)
    ++indx;

  PRInt32 startRes;
  PRInt32 endRes;
  nsresult rv = nsTextServicesDocument::ComparePoints(aStartNode, aStartOffset, parentNode, indx, &startRes);
  if (NS_FAILED(rv)) return PR_FALSE;

  rv = nsTextServicesDocument::ComparePoints(aEndNode,   aEndOffset,   parentNode, indx,  &endRes);
  if (NS_FAILED(rv)) return PR_FALSE;

  return (startRes <= 0) && (endRes >= 0);
}

static PRBool
ContentIsInTraversalRange(nsIDOMNSRange *aRange, nsIDOMNode* aNextNode, PRBool aIsPreMode)
{
  nsCOMPtr<nsIContent>  content(do_QueryInterface(aNextNode));
  nsCOMPtr<nsIDOMRange> range(do_QueryInterface(aRange));
  if (!content || !range)
    return PR_FALSE;



  nsCOMPtr<nsIDOMNode> sNode;
  nsCOMPtr<nsIDOMNode> eNode;
  PRInt32 sOffset;
  PRInt32 eOffset;
  range->GetStartContainer(getter_AddRefs(sNode));
  range->GetStartOffset(&sOffset);
  range->GetEndContainer(getter_AddRefs(eNode));
  range->GetEndOffset(&eOffset);
  return ContentIsInTraversalRange(content, aIsPreMode, sNode, sOffset, eNode, eOffset);
}



nsresult 
nsFilteredContentIterator::AdvanceNode(nsIDOMNode* aNode, nsIDOMNode*& aNewNode, eDirectionType aDir)
{
  nsCOMPtr<nsIDOMNode> nextNode;
  if (aDir == eForward) {
    aNode->GetNextSibling(getter_AddRefs(nextNode));
  } else {
    aNode->GetPreviousSibling(getter_AddRefs(nextNode));
  }

  if (nextNode) {
    
    
    PRBool intersects = ContentIsInTraversalRange(mRange, nextNode, aDir == eForward);
    if (intersects) {
      aNewNode = nextNode;
      NS_ADDREF(aNewNode);
      return NS_OK;
    }
  } else {
    
    nsCOMPtr<nsIDOMNode> parent;
    aNode->GetParentNode(getter_AddRefs(parent));
    NS_ASSERTION(parent, "parent can't be NULL");

    
    PRBool intersects = ContentIsInTraversalRange(mRange, nextNode, aDir == eForward);
    if (intersects) {
      
      nsresult rv = AdvanceNode(parent, aNewNode, aDir);
      if (NS_SUCCEEDED(rv) && aNewNode) {
        return NS_OK;
      }
    }
  }

  
  
  mIsOutOfRange = PR_TRUE;

  return NS_ERROR_FAILURE;
}



void
nsFilteredContentIterator::CheckAdvNode(nsIDOMNode* aNode, PRPackedBool& aDidSkip, eDirectionType aDir)
{
  aDidSkip      = PR_FALSE;
  mIsOutOfRange = PR_FALSE;

  if (aNode && mFilter) {
    nsCOMPtr<nsIDOMNode> currentNode = aNode;
    PRBool skipIt;
    while (1) {
      nsresult rv = mFilter->Skip(aNode, &skipIt);
      if (NS_SUCCEEDED(rv) && skipIt) {
        aDidSkip = PR_TRUE;
        
        
        nsCOMPtr<nsIDOMNode> advNode;
        rv = AdvanceNode(aNode, *getter_AddRefs(advNode), aDir);
        if (NS_SUCCEEDED(rv) && advNode) {
          aNode = advNode;
        } else {
          return; 
        }
      } else {
        if (aNode != currentNode) {
          nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
          mCurrentIterator->PositionAt(content);
        }
        return; 
      }
    }
  }
}

void
nsFilteredContentIterator::Next()
{
  if (mIsOutOfRange || !mCurrentIterator) {
    NS_ASSERTION(mCurrentIterator, "Missing iterator!");

    return;
  }

  
  
  if (mDirection != eForward) {
    nsresult rv = SwitchDirections(PR_TRUE);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  mCurrentIterator->Next();

  if (mCurrentIterator->IsDone()) {
    return;
  }

  
  
  nsIContent *currentContent = mCurrentIterator->GetCurrentNode();

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(currentContent));
  CheckAdvNode(node, mDidSkip, eForward);
}

void
nsFilteredContentIterator::Prev()
{
  if (mIsOutOfRange || !mCurrentIterator) {
    NS_ASSERTION(mCurrentIterator, "Missing iterator!");

    return;
  }

  
  
  if (mDirection != eBackward) {
    nsresult rv = SwitchDirections(PR_FALSE);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  mCurrentIterator->Prev();

  if (mCurrentIterator->IsDone()) {
    return;
  }

  
  
  nsIContent *currentContent = mCurrentIterator->GetCurrentNode();

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(currentContent));
  CheckAdvNode(node, mDidSkip, eBackward);
}

nsIContent *
nsFilteredContentIterator::GetCurrentNode()
{
  if (mIsOutOfRange || !mCurrentIterator) {
    return nsnull;
  }

  return mCurrentIterator->GetCurrentNode();
}

PRBool
nsFilteredContentIterator::IsDone()
{
  if (mIsOutOfRange || !mCurrentIterator) {
    return PR_TRUE;
  }

  return mCurrentIterator->IsDone();
}

nsresult
nsFilteredContentIterator::PositionAt(nsIContent* aCurNode)
{
  NS_ENSURE_TRUE(mCurrentIterator, NS_ERROR_FAILURE);
  mIsOutOfRange = PR_FALSE;
  return mCurrentIterator->PositionAt(aCurNode);
}
