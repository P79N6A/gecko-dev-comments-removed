







































#include "nsGenConList.h"
#include "nsLayoutUtils.h"

void
nsGenConList::Clear()
{
  
  if (!mFirstNode)
    return;
  for (nsGenConNode *node = Next(mFirstNode); node != mFirstNode;
       node = Next(mFirstNode))
  {
    Remove(node);
    delete node;
  }
  delete mFirstNode;

  mFirstNode = nsnull;
  mSize = 0;
}

PRBool
nsGenConList::DestroyNodesFor(nsIFrame* aFrame)
{
  if (!mFirstNode)
    return PR_FALSE; 
  nsGenConNode* node;
  PRBool destroyed = PR_FALSE;
  while (mFirstNode->mPseudoFrame == aFrame) {
    destroyed = PR_TRUE;
    node = Next(mFirstNode);
    PRBool isLastNode = node == mFirstNode; 
    Remove(mFirstNode);
    delete mFirstNode;
    if (isLastNode) {
      mFirstNode = nsnull;
      return PR_TRUE;
    }
    else {
      mFirstNode = node;
    }
  }
  node = Next(mFirstNode);
  while (node != mFirstNode) {
    if (node->mPseudoFrame == aFrame) {
      destroyed = PR_TRUE;
      nsGenConNode *nextNode = Next(node);
      Remove(node);
      delete node;
      node = nextNode;
    } else {
      node = Next(node);
    }
  }
  return destroyed;
}


inline PRInt32 PseudoCompareType(nsIFrame *aFrame)
{
  nsIAtom *pseudo = aFrame->GetStyleContext()->GetPseudoType();
  if (pseudo == nsCSSPseudoElements::before)
    return -1;
  if (pseudo == nsCSSPseudoElements::after)
    return 1;
  return 0;
}

 PRBool
nsGenConList::NodeAfter(const nsGenConNode* aNode1, const nsGenConNode* aNode2)
{
  nsIFrame *frame1 = aNode1->mPseudoFrame;
  nsIFrame *frame2 = aNode2->mPseudoFrame;
  if (frame1 == frame2) {
    NS_ASSERTION(aNode2->mContentIndex != aNode1->mContentIndex, "identical");
    return aNode1->mContentIndex > aNode2->mContentIndex;
  }
  PRInt32 pseudoType1 = PseudoCompareType(frame1);
  PRInt32 pseudoType2 = PseudoCompareType(frame2);
  nsIContent *content1 = frame1->GetContent();
  nsIContent *content2 = frame2->GetContent();
  if (pseudoType1 == 0 || pseudoType2 == 0) {
    if (content1 == content2) {
      NS_ASSERTION(pseudoType1 != pseudoType2, "identical");
      return pseudoType2 == 0;
    }
    
    
    if (pseudoType1 == 0) pseudoType1 = -1;
    if (pseudoType2 == 0) pseudoType2 = -1;
  } else {
    if (content1 == content2) {
      NS_ASSERTION(pseudoType1 != pseudoType2, "identical");
      return pseudoType1 == 1;
    }
  }
  
  PRInt32 cmp = nsLayoutUtils::DoCompareTreePosition(content1, content2,
                                                     pseudoType1, -pseudoType2);
  NS_ASSERTION(cmp != 0, "same content, different frames");
  return cmp > 0;
}

void
nsGenConList::Insert(nsGenConNode* aNode)
{
  if (mFirstNode) {
    
    if (NodeAfter(aNode, Prev(mFirstNode))) {
      PR_INSERT_BEFORE(aNode, mFirstNode);
    }
    else {
      

      
      
      PRUint32 first = 0, last = mSize - 1;

      
      nsGenConNode *curNode = Prev(mFirstNode);
      PRUint32 curIndex = mSize - 1;

      while (first != last) {
        PRUint32 test = (first + last) / 2;
        if (last == curIndex) {
          for ( ; curIndex != test; --curIndex)
            curNode = Prev(curNode);
        } else {
          for ( ; curIndex != test; ++curIndex)
            curNode = Next(curNode);
        }

        if (NodeAfter(aNode, curNode)) {
          first = test + 1;
          
          ++curIndex;
          curNode = Next(curNode);
        } else {
          last = test;
        }
      }
      PR_INSERT_BEFORE(aNode, curNode);
      if (curNode == mFirstNode) {
        mFirstNode = aNode;
      }
    }
  }
  else {
    
    PR_INIT_CLIST(aNode);
    mFirstNode = aNode;
  }
  ++mSize;

  NS_ASSERTION(aNode == mFirstNode || NodeAfter(aNode, Prev(aNode)),
               "sorting error");
  NS_ASSERTION(IsLast(aNode) || NodeAfter(Next(aNode), aNode),
               "sorting error");
}
