




































#include "nsTextFragment.h"
#include "nsWSRunObject.h"
#include "nsIDOMNode.h"
#include "nsHTMLEditor.h"
#include "nsTextEditUtils.h"
#include "nsIContent.h"
#include "nsIDOMCharacterData.h"
#include "nsCRT.h"
#include "nsIRangeUtils.h"

const PRUnichar nbsp = 160;

static PRBool IsBlockNode(nsIDOMNode* node)
{
  PRBool isBlock (PR_FALSE);
  nsHTMLEditor::NodeIsBlockStatic(node, &isBlock);
  return isBlock;
}


nsWSRunObject::nsWSRunObject(nsHTMLEditor *aEd, nsIDOMNode *aNode, PRInt32 aOffset) :
mNode(aNode)
,mOffset(aOffset)
,mPRE(PR_FALSE)
,mStartNode()
,mStartOffset(0)
,mStartReason(0)
,mStartReasonNode()
,mEndNode()
,mEndOffset(0)
,mEndReason(0)
,mEndReasonNode()
,mFirstNBSPNode()
,mFirstNBSPOffset(0)
,mLastNBSPNode()
,mLastNBSPOffset(0)
,mNodeArray()
,mStartRun(nsnull)
,mEndRun(nsnull)
,mHTMLEditor(aEd)
{
  GetWSNodes();
  GetRuns();
}

nsWSRunObject::~nsWSRunObject()
{
  ClearRuns();
}







nsresult
nsWSRunObject::ScrubBlockBoundary(nsHTMLEditor *aHTMLEd, 
                                  nsCOMPtr<nsIDOMNode> *aBlock,
                                  BlockBoundary aBoundary,
                                  PRInt32 *aOffset)
{
  if (!aBlock || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;
  if ((aBoundary == kBlockStart) || (aBoundary == kBlockEnd))
    return ScrubBlockBoundaryInner(aHTMLEd, aBlock, aBoundary);
  
  
  
  if (!aOffset) 
    return NS_ERROR_NULL_POINTER;
  nsAutoTrackDOMPoint tracker(aHTMLEd->mRangeUpdater, aBlock, aOffset);
  nsWSRunObject theWSObj(aHTMLEd, *aBlock, *aOffset);
  return theWSObj.Scrub();
}

nsresult 
nsWSRunObject::PrepareToJoinBlocks(nsHTMLEditor *aHTMLEd, 
                                   nsIDOMNode *aLeftParent, 
                                   nsIDOMNode *aRightParent)
{
  if (!aLeftParent || !aRightParent || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;
  PRUint32 count;
  aHTMLEd->GetLengthOfDOMNode(aLeftParent, count);
  nsWSRunObject leftWSObj(aHTMLEd, aLeftParent, count);
  nsWSRunObject rightWSObj(aHTMLEd, aRightParent, 0);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult 
nsWSRunObject::PrepareToDeleteRange(nsHTMLEditor *aHTMLEd, 
                                    nsCOMPtr<nsIDOMNode> *aStartNode,
                                    PRInt32 *aStartOffset, 
                                    nsCOMPtr<nsIDOMNode> *aEndNode,
                                    PRInt32 *aEndOffset)
{
  if (!aStartNode || !aEndNode || !*aStartNode || !*aEndNode || !aStartOffset || !aEndOffset || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;

  nsAutoTrackDOMPoint trackerStart(aHTMLEd->mRangeUpdater, aStartNode, aStartOffset);
  nsAutoTrackDOMPoint trackerEnd(aHTMLEd->mRangeUpdater, aEndNode, aEndOffset);
  
  nsWSRunObject leftWSObj(aHTMLEd, *aStartNode, *aStartOffset);
  nsWSRunObject rightWSObj(aHTMLEd, *aEndNode, *aEndOffset);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult 
nsWSRunObject::PrepareToDeleteNode(nsHTMLEditor *aHTMLEd, 
                                   nsIDOMNode *aNode)
{
  if (!aNode || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  res = aHTMLEd->GetNodeLocation(aNode, address_of(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  
  nsWSRunObject leftWSObj(aHTMLEd, parent, offset);
  nsWSRunObject rightWSObj(aHTMLEd, parent, offset+1);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult 
nsWSRunObject::PrepareToSplitAcrossBlocks(nsHTMLEditor *aHTMLEd, 
                                          nsCOMPtr<nsIDOMNode> *aSplitNode, 
                                          PRInt32 *aSplitOffset)
{
  if (!aSplitNode || !aSplitOffset || !*aSplitNode || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;

  nsAutoTrackDOMPoint tracker(aHTMLEd->mRangeUpdater, aSplitNode, aSplitOffset);
  
  nsWSRunObject wsObj(aHTMLEd, *aSplitNode, *aSplitOffset);

  return wsObj.PrepareToSplitAcrossBlocksPriv();
}





nsresult 
nsWSRunObject::InsertBreak(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                           PRInt32 *aInOutOffset, 
                           nsCOMPtr<nsIDOMNode> *outBRNode, 
                           nsIEditor::EDirection aSelect)
{
  
  
  if (!aInOutParent || !aInOutOffset || !outBRNode)
    return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;
  WSFragment *beforeRun, *afterRun;
  res = FindRun(*aInOutParent, *aInOutOffset, &beforeRun, PR_FALSE);
  res = FindRun(*aInOutParent, *aInOutOffset, &afterRun, PR_TRUE);
  
  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent, aInOutOffset);

    
    if (!afterRun)
    {
      
    }
    else if (afterRun->mType & eTrailingWS)
    {
      
    }
    else if (afterRun->mType & eLeadingWS)
    {
      
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset, afterRun->mEndNode, afterRun->mEndOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (afterRun->mType == eNormalWS)
    {
      
      
      WSPoint thePoint;
      res = GetCharAfter(*aInOutParent, *aInOutOffset, &thePoint);
      if ( (NS_SUCCEEDED(res)) && thePoint.mTextNode && (nsCRT::IsAsciiSpace(thePoint.mChar)) )
      {
        WSPoint prevPoint;
        res = GetCharBefore(thePoint, &prevPoint);
        if ( (NS_FAILED(res)) || (prevPoint.mTextNode && !nsCRT::IsAsciiSpace(prevPoint.mChar)) )
        {
          
          res = ConvertToNBSP(thePoint);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
    }
    
    
    if (!beforeRun)
    {
      
    }
    else if (beforeRun->mType & eLeadingWS)
    {
      
    }
    else if (beforeRun->mType & eTrailingWS)
    {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, *aInOutParent, *aInOutOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (beforeRun->mType == eNormalWS)
    {
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  return mHTMLEditor->CreateBRImpl(aInOutParent, aInOutOffset, outBRNode, aSelect);
}

nsresult 
nsWSRunObject::InsertText(const nsAString& aStringToInsert, 
                          nsCOMPtr<nsIDOMNode> *aInOutParent, 
                          PRInt32 *aInOutOffset,
                          nsIDOMDocument *aDoc)
{
  
  

  
  
  

  if (!aInOutParent || !aInOutOffset || !aDoc)
    return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;
  if (aStringToInsert.IsEmpty()) return res;
  
  
  nsAutoString theString(aStringToInsert);
  
  WSFragment *beforeRun, *afterRun;
  res = FindRun(*aInOutParent, *aInOutOffset, &beforeRun, PR_FALSE);
  res = FindRun(*aInOutParent, *aInOutOffset, &afterRun, PR_TRUE);
  
  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent, aInOutOffset);

    
    if (!afterRun)
    {
      
    }
    else if (afterRun->mType & eTrailingWS)
    {
      
    }
    else if (afterRun->mType & eLeadingWS)
    {
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset, afterRun->mEndNode, afterRun->mEndOffset,
                         eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (afterRun->mType == eNormalWS)
    {
      
      res = CheckLeadingNBSP(afterRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    if (!beforeRun)
    {
      
    }
    else if (beforeRun->mType & eLeadingWS)
    {
      
    }
    else if (beforeRun->mType & eTrailingWS)
    {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, *aInOutParent, *aInOutOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (beforeRun->mType == eNormalWS)
    {
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  
  
  
  
  if (nsCRT::IsAsciiSpace(theString[0]))
  {
    
    if (beforeRun)
    {
      if (beforeRun->mType & eLeadingWS) 
      {
        theString.SetCharAt(nbsp, 0);
      }
      else if (beforeRun->mType & eNormalWS) 
      {
        WSPoint wspoint;
        res = GetCharBefore(*aInOutParent, *aInOutOffset, &wspoint);
        if (NS_SUCCEEDED(res) && wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar))
        {
          theString.SetCharAt(nbsp, 0);
        }
      }
    }
    else
    {
      if ((mStartReason & eBlock) || (mStartReason == eBreak))
      {
        theString.SetCharAt(nbsp, 0);
      }
    }
  }

  
  PRUint32 lastCharIndex = theString.Length()-1;

  if (nsCRT::IsAsciiSpace(theString[lastCharIndex]))
  {
    
    if (afterRun)
    {
      if (afterRun->mType & eTrailingWS)
      {
        theString.SetCharAt(nbsp, lastCharIndex);
      }
      else if (afterRun->mType & eNormalWS) 
      {
        WSPoint wspoint;
        res = GetCharAfter(*aInOutParent, *aInOutOffset, &wspoint);
        if (NS_SUCCEEDED(res) && wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar))
        {
          theString.SetCharAt(nbsp, lastCharIndex);
        }
      }
    }
    else
    {
      if ((mEndReason & eBlock))
      {
        theString.SetCharAt(nbsp, lastCharIndex);
      }
    }
  }
  
  
  
  
  
  PRUint32 j;
  PRBool prevWS = PR_FALSE;
  for (j=0; j<=lastCharIndex; j++)
  {
    if (nsCRT::IsAsciiSpace(theString[j]))
    {
      if (prevWS)
      {
        theString.SetCharAt(nbsp, j-1);  
      }
      else
      {
        prevWS = PR_TRUE;
      }
    }
    else
    {
      prevWS = PR_FALSE;
    }
  }
  
  
  res = mHTMLEditor->InsertTextImpl(theString, aInOutParent, aInOutOffset, aDoc);
  return NS_OK;
}

nsresult 
nsWSRunObject::DeleteWSBackward()
{
  nsresult res = NS_OK;
  WSPoint point;
  res = GetCharBefore(mNode, mOffset, &point);  
  NS_ENSURE_SUCCESS(res, res);
  if (!point.mTextNode) return NS_OK;  
  
  if (mPRE)  
  {
    if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar == nbsp))
    {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
      PRInt32 startOffset = point.mOffset;
      PRInt32 endOffset = point.mOffset+1;
      return DeleteChars(node, startOffset, node, endOffset);
    }
  }
  
  
  
  if (nsCRT::IsAsciiSpace(point.mChar))
  {
    nsCOMPtr<nsIDOMNode> startNode, endNode, node(do_QueryInterface(point.mTextNode));
    PRInt32 startOffset, endOffset;
    res = GetAsciiWSBounds(eBoth, node, point.mOffset+1, address_of(startNode), 
                         &startOffset, address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(startNode), &startOffset, 
                                              address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  else if (point.mChar == nbsp)
  {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
    
    PRInt32 startOffset = point.mOffset;
    PRInt32 endOffset = point.mOffset+1;
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(node), &startOffset, 
                                              address_of(node), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(node, startOffset, node, endOffset);
  
  }
  return NS_OK;
}

nsresult 
nsWSRunObject::DeleteWSForward()
{
  nsresult res = NS_OK;
  WSPoint point;
  res = GetCharAfter(mNode, mOffset, &point);  
  NS_ENSURE_SUCCESS(res, res);
  if (!point.mTextNode) return NS_OK;  
  
  if (mPRE)  
  {
    if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar == nbsp))
    {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
      PRInt32 startOffset = point.mOffset;
      PRInt32 endOffset = point.mOffset+1;
      return DeleteChars(node, startOffset, node, endOffset);
    }
  }
  
  
  
  if (nsCRT::IsAsciiSpace(point.mChar))
  {
    nsCOMPtr<nsIDOMNode> startNode, endNode, node(do_QueryInterface(point.mTextNode));
    PRInt32 startOffset, endOffset;
    res = GetAsciiWSBounds(eBoth, node, point.mOffset+1, address_of(startNode), 
                         &startOffset, address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(startNode), &startOffset, 
                                              address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  else if (point.mChar == nbsp)
  {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
    
    PRInt32 startOffset = point.mOffset;
    PRInt32 endOffset = point.mOffset+1;
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(node), &startOffset, 
                                              address_of(node), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(node, startOffset, node, endOffset);
  
  }
  return NS_OK;
}

nsresult 
nsWSRunObject::PriorVisibleNode(nsIDOMNode *aNode, 
                                PRInt32 aOffset, 
                                nsCOMPtr<nsIDOMNode> *outVisNode, 
                                PRInt32 *outVisOffset,
                                PRInt16 *outType)
{
  
  
  if (!aNode || !outVisNode || !outVisOffset || !outType)
    return NS_ERROR_NULL_POINTER;
    
  *outType = eNone;
  WSFragment *run;
  FindRun(aNode, aOffset, &run, PR_FALSE);
  
  
  while (run)
  {
    if (run->mType == eNormalWS)
    {
      WSPoint point;
      GetCharBefore(aNode, aOffset, &point);
      if (point.mTextNode)
      {
        *outVisNode = do_QueryInterface(point.mTextNode);
        *outVisOffset = point.mOffset+1;
        if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar==nbsp))
        {
          *outType = eNormalWS;
        }
        else if (!point.mChar)
        {
          
          *outType = eNone;
        }
        else
        {
          *outType = eText;
        }
        return NS_OK;
      }
      
    }

    run = run->mLeft;
  }
  
  
  *outVisNode = mStartReasonNode;
  *outVisOffset = mStartOffset;  
  *outType = mStartReason;
  return NS_OK;
}


nsresult 
nsWSRunObject::NextVisibleNode (nsIDOMNode *aNode, 
                                PRInt32 aOffset, 
                                nsCOMPtr<nsIDOMNode> *outVisNode, 
                                PRInt32 *outVisOffset,
                                PRInt16 *outType)
{
  
  
  if (!aNode || !outVisNode || !outVisOffset || !outType)
    return NS_ERROR_NULL_POINTER;
    
  WSFragment *run;
  FindRun(aNode, aOffset, &run, PR_TRUE);
  
  
  while (run)
  {
    if (run->mType == eNormalWS)
    {
      WSPoint point;
      GetCharAfter(aNode, aOffset, &point);
      if (point.mTextNode)
      {
        *outVisNode = do_QueryInterface(point.mTextNode);
        *outVisOffset = point.mOffset;
        if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar==nbsp))
        {
          *outType = eNormalWS;
        }
        else if (!point.mChar)
        {
          
          *outType = eNone;
        }
        else
        {
          *outType = eText;
        }
        return NS_OK;
      }
      
    }

    run = run->mRight;
  }
  
  
  *outVisNode = mEndReasonNode;
  *outVisOffset = mEndOffset; 
  *outType = mEndReason;
  return NS_OK;
}

nsresult 
nsWSRunObject::AdjustWhitespace()
{
  
  
  
  if (!mLastNBSPNode) return NS_OK; 
  nsresult res = NS_OK;
  WSFragment *curRun = mStartRun;
  while (curRun)
  {
    
    if (curRun->mType == eNormalWS)
    {
      res = CheckTrailingNBSPOfRun(curRun);
      break;
    }
    curRun = curRun->mRight;
  }
  return res;
}






nsresult
nsWSRunObject::GetWSNodes()
{
  
  
  
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> blockParent;
  DOMPoint start(mNode, mOffset), end(mNode, mOffset);
  if (IsBlockNode(mNode)) blockParent = mNode;
  else blockParent = mHTMLEditor->GetBlockNodeParent(mNode);

  
  if (mHTMLEditor->IsTextNode(mNode))
  {
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNode));
    const nsTextFragment *textFrag = textNode->GetText();
    
    res = PrependNodeToList(mNode);
    NS_ENSURE_SUCCESS(res, res);
    if (mOffset)
    {
      PRInt32 pos;
      for (pos=mOffset-1; pos>=0; pos--)
      {
        
        if (pos >= textFrag->GetLength())
        {
          NS_NOTREACHED("looking beyond end of text fragment");
          continue;
        }
        PRUnichar theChar = textFrag->CharAt(pos);
        if (!nsCRT::IsAsciiSpace(theChar))
        {
          if (theChar != nbsp)
          {
            mStartNode = mNode;
            mStartOffset = pos+1;
            mStartReason = eText;
            mStartReasonNode = mNode;
            break;
          }
          
          mFirstNBSPNode = mNode;
          mFirstNBSPOffset = pos;
          
          if (!mLastNBSPNode)
          {
            mLastNBSPNode = mNode;
            mLastNBSPOffset = pos;
          }
        }
        start.SetPoint(mNode,pos);
      }
    }
  }

  nsCOMPtr<nsIDOMNode> priorNode;
  while (!mStartNode)
  {
    
    res = GetPreviousWSNode(start, blockParent, address_of(priorNode));
    NS_ENSURE_SUCCESS(res, res);
    if (priorNode)
    {
      if (IsBlockNode(priorNode))
      {
        start.GetPoint(mStartNode, mStartOffset);
        mStartReason = eOtherBlock;
        mStartReasonNode = priorNode;
      }
      else if (mHTMLEditor->IsTextNode(priorNode))
      {
        res = PrependNodeToList(priorNode);
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<nsIContent> textNode(do_QueryInterface(priorNode));
        const nsTextFragment *textFrag;
        if (!textNode || !(textFrag = textNode->GetText())) {
          return NS_ERROR_NULL_POINTER;
        }
        PRUint32 len = textNode->TextLength();

        if (len < 1)
        {
          
          
          start.SetPoint(priorNode,0);
        }
        else
        {
          PRInt32 pos;
          for (pos=len-1; pos>=0; pos--)
          {
            
            if (pos >= textFrag->GetLength())
            {
              NS_NOTREACHED("looking beyond end of text fragment");
              continue;
            }
            PRUnichar theChar = textFrag->CharAt(pos);
            if (!nsCRT::IsAsciiSpace(theChar))
            {
              if (theChar != nbsp)
              {
                mStartNode = priorNode;
                mStartOffset = pos+1;
                mStartReason = eText;
                mStartReasonNode = priorNode;
                break;
              }
              
              mFirstNBSPNode = priorNode;
              mFirstNBSPOffset = pos;
              
              if (!mLastNBSPNode)
              {
                mLastNBSPNode = priorNode;
                mLastNBSPOffset = pos;
              }
            }
            start.SetPoint(priorNode,pos);
          }
        }
      }
      else
      {
        
        
        start.GetPoint(mStartNode, mStartOffset);
        if (nsTextEditUtils::IsBreak(priorNode))
          mStartReason = eBreak;
        else
          mStartReason = eSpecial;
        mStartReasonNode = priorNode;
      }
    }
    else
    {
      
      start.GetPoint(mStartNode, mStartOffset);
      mStartReason = eThisBlock;
      mStartReasonNode = blockParent;
    } 
  }
  
  
  if (mHTMLEditor->IsTextNode(mNode))
  {
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNode));
    const nsTextFragment *textFrag = textNode->GetText();

    PRUint32 len = textNode->TextLength();
    if (mOffset<len)
    {
      PRInt32 pos;
      for (pos=mOffset; pos<len; pos++)
      {
        
        if ((pos<0) || (pos>=textFrag->GetLength()))
        {
          NS_NOTREACHED("looking beyond end of text fragment");
          continue;
        }
        PRUnichar theChar = textFrag->CharAt(pos);
        if (!nsCRT::IsAsciiSpace(theChar))
        {
          if (theChar != nbsp)
          {
            mEndNode = mNode;
            mEndOffset = pos;
            mEndReason = eText;
            mEndReasonNode = mNode;
            break;
          }
          
          mLastNBSPNode = mNode;
          mLastNBSPOffset = pos;
          
          if (!mFirstNBSPNode)
          {
            mFirstNBSPNode = mNode;
            mFirstNBSPOffset = pos;
          }
        }
        end.SetPoint(mNode,pos+1);
      }
    }
  }

  nsCOMPtr<nsIDOMNode> nextNode;
  while (!mEndNode)
  {
    
    res = GetNextWSNode(end, blockParent, address_of(nextNode));
    NS_ENSURE_SUCCESS(res, res);
    if (nextNode)
    {
      if (IsBlockNode(nextNode))
      {
        
        end.GetPoint(mEndNode, mEndOffset);
        mEndReason = eOtherBlock;
        mEndReasonNode = nextNode;
      }
      else if (mHTMLEditor->IsTextNode(nextNode))
      {
        res = AppendNodeToList(nextNode);
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<nsIContent> textNode(do_QueryInterface(nextNode));
        const nsTextFragment *textFrag;
        if (!textNode || !(textFrag = textNode->GetText())) {
          return NS_ERROR_NULL_POINTER;
        }
        PRUint32 len = textNode->TextLength();

        if (len < 1)
        {
          
          
          end.SetPoint(nextNode,0);
        }
        else
        {
          PRInt32 pos;
          for (pos=0; pos<len; pos++)
          {
            
            if (pos >= textFrag->GetLength())
            {
              NS_NOTREACHED("looking beyond end of text fragment");
              continue;
            }
            PRUnichar theChar = textFrag->CharAt(pos);
            if (!nsCRT::IsAsciiSpace(theChar))
            {
              if (theChar != nbsp)
              {
                mEndNode = nextNode;
                mEndOffset = pos;
                mEndReason = eText;
                mEndReasonNode = nextNode;
                break;
              }
              
              mLastNBSPNode = nextNode;
              mLastNBSPOffset = pos;
              
              if (!mFirstNBSPNode)
              {
                mFirstNBSPNode = nextNode;
                mFirstNBSPOffset = pos;
              }
            }
            end.SetPoint(nextNode,pos+1);
          }
        }
      }
      else
      {
        
        
        
        end.GetPoint(mEndNode, mEndOffset);
        if (nsTextEditUtils::IsBreak(nextNode))
          mEndReason = eBreak;
        else
          mEndReason = eSpecial;
        mEndReasonNode = nextNode;
      }
    }
    else
    {
      
      end.GetPoint(mEndNode, mEndOffset);
      mEndReason = eThisBlock;
      mEndReasonNode = blockParent;
    } 
  }

  return NS_OK;
}

nsresult
nsWSRunObject::GetRuns()
{
  ClearRuns();
  
  
  mHTMLEditor->IsPreformatted(mNode, &mPRE);
  
  
  if ( mPRE || (((mStartReason == eText) || (mStartReason == eSpecial)) &&
       ((mEndReason == eText) || (mEndReason == eSpecial) || (mEndReason == eBreak))) )
  {
    return MakeSingleWSRun(eNormalWS);
  }

  
  
  if ( !(mFirstNBSPNode || mLastNBSPNode) &&
      ( (mStartReason & eBlock) || (mStartReason == eBreak) || (mEndReason & eBlock) ) )
  {
    PRInt16 wstype = eNone;
    if ((mStartReason & eBlock) || (mStartReason == eBreak))
      wstype = eLeadingWS;
    if (mEndReason & eBlock) 
      wstype |= eTrailingWS;
    return MakeSingleWSRun(wstype);
  }
  
  
  mStartRun = new WSFragment();
  if (!mStartRun) return NS_ERROR_NULL_POINTER;
  mStartRun->mStartNode = mStartNode;
  mStartRun->mStartOffset = mStartOffset;
  
  if ( (mStartReason & eBlock) || (mStartReason == eBreak) )
  {
    
    mStartRun->mType = eLeadingWS;
    mStartRun->mEndNode = mFirstNBSPNode;
    mStartRun->mEndOffset = mFirstNBSPOffset;
    mStartRun->mLeftType = mStartReason;
    mStartRun->mRightType = eNormalWS;
    
    
    WSFragment *normalRun = new WSFragment();
    if (!normalRun) return NS_ERROR_NULL_POINTER;
    mStartRun->mRight = normalRun;
    normalRun->mType = eNormalWS;
    normalRun->mStartNode = mFirstNBSPNode;
    normalRun->mStartOffset = mFirstNBSPOffset;
    normalRun->mLeftType = eLeadingWS;
    normalRun->mLeft = mStartRun;
    if (mEndReason != eBlock)
    {
      
      normalRun->mRightType = mEndReason;
      normalRun->mEndNode   = mEndNode;
      normalRun->mEndOffset = mEndOffset;
      mEndRun = normalRun;
    }
    else
    {
      
      
      
      
      if ((mLastNBSPNode == mEndNode) && (mLastNBSPOffset == (mEndOffset-1)))
      {
        
        normalRun->mRightType = mEndReason;
        normalRun->mEndNode   = mEndNode;
        normalRun->mEndOffset = mEndOffset;
        mEndRun = normalRun;
      }
      else
      {
        normalRun->mEndNode = mLastNBSPNode;
        normalRun->mEndOffset = mLastNBSPOffset+1;
        normalRun->mRightType = eTrailingWS;
        
        
        WSFragment *lastRun = new WSFragment();
        if (!lastRun) return NS_ERROR_NULL_POINTER;
        lastRun->mType = eTrailingWS;
        lastRun->mStartNode = mLastNBSPNode;
        lastRun->mStartOffset = mLastNBSPOffset+1;
        lastRun->mEndNode = mEndNode;
        lastRun->mEndOffset = mEndOffset;
        lastRun->mLeftType = eNormalWS;
        lastRun->mLeft = normalRun;
        lastRun->mRightType = mEndReason;
        mEndRun = lastRun;
        normalRun->mRight = lastRun;
      }
    }
  }
  else 
  {
    
    mStartRun->mType = eNormalWS;
    mStartRun->mEndNode = mLastNBSPNode;
    mStartRun->mEndOffset = mLastNBSPOffset+1;
    mStartRun->mLeftType = mStartReason;

    
    
    
    
    if ((mLastNBSPNode == mEndNode) && (mLastNBSPOffset == (mEndOffset-1)))
    {
      mStartRun->mRightType = mEndReason;
      mStartRun->mEndNode   = mEndNode;
      mStartRun->mEndOffset = mEndOffset;
      mEndRun = mStartRun;
    }
    else
    {
      
      WSFragment *lastRun = new WSFragment();
      if (!lastRun) return NS_ERROR_NULL_POINTER;
      lastRun->mType = eTrailingWS;
      lastRun->mStartNode = mLastNBSPNode;
      lastRun->mStartOffset = mLastNBSPOffset+1;
      lastRun->mLeftType = eNormalWS;
      lastRun->mLeft = mStartRun;
      lastRun->mRightType = mEndReason;
      mEndRun = lastRun;
      mStartRun->mRight = lastRun;
      mStartRun->mRightType = eTrailingWS;
    }
  }
  
  return NS_OK;
}

void
nsWSRunObject::ClearRuns()
{
  WSFragment *tmp, *run;
  run = mStartRun;
  while (run)
  {
    tmp = run->mRight;
    delete run;
    run = tmp;
  }
  mStartRun = 0;
  mEndRun = 0;
}

nsresult 
nsWSRunObject::MakeSingleWSRun(PRInt16 aType)
{
  mStartRun = new WSFragment();
  if (!mStartRun) return NS_ERROR_NULL_POINTER;

  mStartRun->mStartNode   = mStartNode;
  mStartRun->mStartOffset = mStartOffset;
  mStartRun->mType        = aType;
  mStartRun->mEndNode     = mEndNode;
  mStartRun->mEndOffset   = mEndOffset;
  mStartRun->mLeftType    = mStartReason;
  mStartRun->mRightType   = mEndReason;
  
  mEndRun  = mStartRun;
  
  return NS_OK;
}

nsresult 
nsWSRunObject::PrependNodeToList(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  if (!mNodeArray.InsertObjectAt(aNode, 0))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult 
nsWSRunObject::AppendNodeToList(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  if (!mNodeArray.AppendObject(aNode))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult 
nsWSRunObject::GetPreviousWSNode(nsIDOMNode *aStartNode, 
                                 nsIDOMNode *aBlockParent, 
                                 nsCOMPtr<nsIDOMNode> *aPriorNode)
{
  
  
  
  if (!aStartNode || !aBlockParent || !aPriorNode) return NS_ERROR_NULL_POINTER;
  
  nsresult res = aStartNode->GetPreviousSibling(getter_AddRefs(*aPriorNode));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> temp, curNode = aStartNode;
  while (!*aPriorNode)
  {
    
    res = curNode->GetParentNode(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    if (!temp) return NS_ERROR_NULL_POINTER;
    if (temp == aBlockParent)
    {
      
      *aPriorNode = nsnull;
      return NS_OK;
    }
    
    res = temp->GetPreviousSibling(getter_AddRefs(*aPriorNode));
    NS_ENSURE_SUCCESS(res, res);
    curNode = temp;
  }
  
  if (IsBlockNode(*aPriorNode))
    return NS_OK;
  
  else if (mHTMLEditor->IsContainer(*aPriorNode))
  {
    temp = mHTMLEditor->GetRightmostChild(*aPriorNode);
    if (temp)
      *aPriorNode = temp;
    return NS_OK;
  }
  
  return NS_OK;
}

nsresult 
nsWSRunObject::GetPreviousWSNode(DOMPoint aPoint,
                                 nsIDOMNode *aBlockParent, 
                                 nsCOMPtr<nsIDOMNode> *aPriorNode)
{
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  aPoint.GetPoint(node, offset);
  return GetPreviousWSNode(node,offset,aBlockParent,aPriorNode);
}

nsresult 
nsWSRunObject::GetPreviousWSNode(nsIDOMNode *aStartNode,
                                 PRInt16 aOffset, 
                                 nsIDOMNode *aBlockParent, 
                                 nsCOMPtr<nsIDOMNode> *aPriorNode)
{
  
  
  
  if (!aStartNode || !aBlockParent || !aPriorNode)
    return NS_ERROR_NULL_POINTER;
  *aPriorNode = 0;

  if (mHTMLEditor->IsTextNode(aStartNode))
    return GetPreviousWSNode(aStartNode, aBlockParent, aPriorNode);
  if (!mHTMLEditor->IsContainer(aStartNode))
    return GetPreviousWSNode(aStartNode, aBlockParent, aPriorNode);
  
  if (!aOffset)
  {
    if (aStartNode==aBlockParent)
    {
      
      return NS_OK;
    }

    
    return GetPreviousWSNode(aStartNode, aBlockParent, aPriorNode);
  }

  nsCOMPtr<nsIContent> startContent( do_QueryInterface(aStartNode) );

  nsIContent *priorContent = startContent->GetChildAt(aOffset - 1);
  if (!priorContent) 
    return NS_ERROR_NULL_POINTER;
  *aPriorNode = do_QueryInterface(priorContent);
  
  if (IsBlockNode(*aPriorNode))
    return NS_OK;
  
  else if (mHTMLEditor->IsContainer(*aPriorNode))
  {
    nsCOMPtr<nsIDOMNode> temp;
    temp = mHTMLEditor->GetRightmostChild(*aPriorNode);
    if (temp)
      *aPriorNode = temp;
    return NS_OK;
  }
  
  return NS_OK;
}

nsresult 
nsWSRunObject::GetNextWSNode(nsIDOMNode *aStartNode, 
                             nsIDOMNode *aBlockParent, 
                             nsCOMPtr<nsIDOMNode> *aNextNode)
{
  
  
  
  if (!aStartNode || !aBlockParent || !aNextNode)
    return NS_ERROR_NULL_POINTER;
  
  *aNextNode = 0;
  nsresult res = aStartNode->GetNextSibling(getter_AddRefs(*aNextNode));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> temp, curNode = aStartNode;
  while (!*aNextNode)
  {
    
    res = curNode->GetParentNode(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    if (!temp) return NS_ERROR_NULL_POINTER;
    if (temp == aBlockParent)
    {
      
      
      *aNextNode = nsnull;
      return NS_OK;
    }
    
    res = temp->GetNextSibling(getter_AddRefs(*aNextNode));
    NS_ENSURE_SUCCESS(res, res);
    curNode = temp;
  }
  
  if (IsBlockNode(*aNextNode))
    return NS_OK;
  
  else if (mHTMLEditor->IsContainer(*aNextNode))
  {
    temp = mHTMLEditor->GetLeftmostChild(*aNextNode);
    if (temp)
      *aNextNode = temp;
    return NS_OK;
  }
  
  return NS_OK;
}

nsresult 
nsWSRunObject::GetNextWSNode(DOMPoint aPoint,
                             nsIDOMNode *aBlockParent, 
                             nsCOMPtr<nsIDOMNode> *aNextNode)
{
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  aPoint.GetPoint(node, offset);
  return GetNextWSNode(node,offset,aBlockParent,aNextNode);
}

nsresult 
nsWSRunObject::GetNextWSNode(nsIDOMNode *aStartNode,
                             PRInt16 aOffset, 
                             nsIDOMNode *aBlockParent, 
                             nsCOMPtr<nsIDOMNode> *aNextNode)
{
  
  
  if (!aStartNode || !aBlockParent || !aNextNode)
    return NS_ERROR_NULL_POINTER;
  *aNextNode = 0;

  if (mHTMLEditor->IsTextNode(aStartNode))
    return GetNextWSNode(aStartNode, aBlockParent, aNextNode);
  if (!mHTMLEditor->IsContainer(aStartNode))
    return GetNextWSNode(aStartNode, aBlockParent, aNextNode);
  
  nsCOMPtr<nsIContent> startContent( do_QueryInterface(aStartNode) );
  nsIContent *nextContent = startContent->GetChildAt(aOffset);
  if (!nextContent)
  {
    if (aStartNode==aBlockParent)
    {
      
      return NS_OK;
    }

    
    return GetNextWSNode(aStartNode, aBlockParent, aNextNode);
  }
  
  *aNextNode = do_QueryInterface(nextContent);
  
  if (IsBlockNode(*aNextNode))
    return NS_OK;
  
  else if (mHTMLEditor->IsContainer(*aNextNode))
  {
    nsCOMPtr<nsIDOMNode> temp;
    temp = mHTMLEditor->GetLeftmostChild(*aNextNode);
    if (temp)
      *aNextNode = temp;
    return NS_OK;
  }
  
  return NS_OK;
}

nsresult 
nsWSRunObject::PrepareToDeleteRangePriv(nsWSRunObject* aEndObject)
{
  
  
  
  
  
  
  
  if (!aEndObject)
    return NS_ERROR_NULL_POINTER;
  nsresult res = NS_OK;
  
  
  WSFragment *beforeRun, *afterRun;
  res = FindRun(mNode, mOffset, &beforeRun, PR_FALSE);
  NS_ENSURE_SUCCESS(res, res);
  res = aEndObject->FindRun(aEndObject->mNode, aEndObject->mOffset, &afterRun, PR_TRUE);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (afterRun && (afterRun->mType & eLeadingWS))
  {
    res = aEndObject->DeleteChars(aEndObject->mNode, aEndObject->mOffset, afterRun->mEndNode, afterRun->mEndOffset,
                                  eOutsideUserSelectAll);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  if (afterRun && (afterRun->mType == eNormalWS) && !aEndObject->mPRE)
  {
    if ( (beforeRun && (beforeRun->mType & eLeadingWS)) ||
         (!beforeRun && ((mStartReason & eBlock) || (mStartReason == eBreak))) )
    {
      
      WSPoint point;
      aEndObject->GetCharAfter(aEndObject->mNode, aEndObject->mOffset, &point);
      if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
      {
        res = aEndObject->ConvertToNBSP(point, eOutsideUserSelectAll);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  
  if (beforeRun && (beforeRun->mType & eTrailingWS))
  {
    res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, mNode, mOffset,
                      eOutsideUserSelectAll);
    NS_ENSURE_SUCCESS(res, res);
  }
  else if (beforeRun && (beforeRun->mType == eNormalWS) && !mPRE)
  {
    if ( (afterRun && (afterRun->mType & eTrailingWS)) ||
         (afterRun && (afterRun->mType == eNormalWS))   ||
         (!afterRun && ((aEndObject->mEndReason & eBlock))) )
    {
      
      WSPoint point;
      GetCharBefore(mNode, mOffset, &point);
      if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
      {
        nsCOMPtr<nsIDOMNode> wsStartNode, wsEndNode;
        PRInt32 wsStartOffset, wsEndOffset;
        res = GetAsciiWSBounds(eBoth, mNode, mOffset, 
                               address_of(wsStartNode), &wsStartOffset, 
                               address_of(wsEndNode), &wsEndOffset);
        NS_ENSURE_SUCCESS(res, res);
        point.mTextNode = do_QueryInterface(wsStartNode);
        if (!point.mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
          
          
          point.mTextNode = nsnull;
        }
        point.mOffset = wsStartOffset;
        res = ConvertToNBSP(point, eOutsideUserSelectAll);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  return res;
}

nsresult 
nsWSRunObject::PrepareToSplitAcrossBlocksPriv()
{
  
  
  
  nsresult res = NS_OK;
  
  
  WSFragment *beforeRun, *afterRun;
  res = FindRun(mNode, mOffset, &beforeRun, PR_FALSE);
  NS_ENSURE_SUCCESS(res, res);
  res = FindRun(mNode, mOffset, &afterRun, PR_TRUE);
  
  
  if (afterRun && (afterRun->mType == eNormalWS))
  {
    
    WSPoint point;
    GetCharAfter(mNode, mOffset, &point);
    if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
    {
      res = ConvertToNBSP(point);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  if (beforeRun && (beforeRun->mType == eNormalWS))
  {
    
    WSPoint point;
    GetCharBefore(mNode, mOffset, &point);
    if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
    {
      nsCOMPtr<nsIDOMNode> wsStartNode, wsEndNode;
      PRInt32 wsStartOffset, wsEndOffset;
      res = GetAsciiWSBounds(eBoth, mNode, mOffset, 
                             address_of(wsStartNode), &wsStartOffset, 
                             address_of(wsEndNode), &wsEndOffset);
      NS_ENSURE_SUCCESS(res, res);
      point.mTextNode = do_QueryInterface(wsStartNode);
      if (!point.mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
        
        
        point.mTextNode = nsnull;
      }
      point.mOffset = wsStartOffset;
      res = ConvertToNBSP(point);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return res;
}

nsresult 
nsWSRunObject::DeleteChars(nsIDOMNode *aStartNode, PRInt32 aStartOffset, 
                           nsIDOMNode *aEndNode, PRInt32 aEndOffset,
                           AreaRestriction aAR)
{
  
  
  if (!aStartNode || !aEndNode)
    return NS_ERROR_NULL_POINTER;

  if (aAR == eOutsideUserSelectAll)
  {
    nsCOMPtr<nsIDOMNode> san = mHTMLEditor->FindUserSelectAllNode(aStartNode);
    if (san)
      return NS_OK;
    
    if (aStartNode != aEndNode)
    {
      san = mHTMLEditor->FindUserSelectAllNode(aEndNode);
      if (san)
        return NS_OK;
    }
  }

  if ((aStartNode == aEndNode) && (aStartOffset == aEndOffset))
    return NS_OK;  
  
  nsresult res = NS_OK;
  PRInt32 idx = mNodeArray.IndexOf(aStartNode);
  if (idx==-1) idx = 0; 
                        
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMCharacterData> textnode;
  nsCOMPtr<nsIDOMRange> range;

  if (aStartNode == aEndNode)
  {
    textnode = do_QueryInterface(aStartNode);
    if (textnode)
    {
      return mHTMLEditor->DeleteText(textnode, (PRUint32)aStartOffset, 
                                     (PRUint32)(aEndOffset-aStartOffset));
    }
  }

  PRInt32 count = mNodeArray.Count();
  while (idx < count)
  {
    node = mNodeArray[idx];
    if (!node)
      break;  
    if (node == aStartNode)
    {
      textnode = do_QueryInterface(node);
      PRUint32 len;
      textnode->GetLength(&len);
      if (aStartOffset<len)
      {
        res = mHTMLEditor->DeleteText(textnode, (PRUint32)aStartOffset, len-aStartOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    else if (node == aEndNode)
    {
      if (aEndOffset)
      {
        textnode = do_QueryInterface(node);
        res = mHTMLEditor->DeleteText(textnode, 0, (PRUint32)aEndOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      break;
    }
    else
    {
      if (!range)
      {
        range = do_CreateInstance("@mozilla.org/content/range;1");
        if (!range) return NS_ERROR_OUT_OF_MEMORY;
        res = range->SetStart(aStartNode, aStartOffset);
        NS_ENSURE_SUCCESS(res, res);
        res = range->SetEnd(aEndNode, aEndOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      PRBool nodeBefore, nodeAfter;
      nsCOMPtr<nsIContent> content (do_QueryInterface(node));
      res = mHTMLEditor->sRangeHelper->CompareNodeToRange(content, range, &nodeBefore, &nodeAfter);
      NS_ENSURE_SUCCESS(res, res);
      if (nodeAfter)
      {
        break;
      }
      if (!nodeBefore)
      {
        res = mHTMLEditor->DeleteNode(node);
        NS_ENSURE_SUCCESS(res, res);
        mNodeArray.RemoveObject(node);
        --count;
        --idx;
      }
    }
    idx++;
  }
  return res;
}

nsresult 
nsWSRunObject::GetCharAfter(nsIDOMNode *aNode, PRInt32 aOffset, WSPoint *outPoint)
{
  if (!aNode || !outPoint)
    return NS_ERROR_NULL_POINTER;

  PRInt32 idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) 
  {
    
    return GetWSPointAfter(aNode, aOffset, outPoint);
  }
  else
  {
    
    WSPoint point(aNode,aOffset,0);
    return GetCharAfter(point, outPoint);
  }
  
  return NS_ERROR_FAILURE;
}

nsresult 
nsWSRunObject::GetCharBefore(nsIDOMNode *aNode, PRInt32 aOffset, WSPoint *outPoint)
{
  if (!aNode || !outPoint)
    return NS_ERROR_NULL_POINTER;

  PRInt32 idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) 
  {
    
    return GetWSPointBefore(aNode, aOffset, outPoint);
  }
  else
  {
    
    WSPoint point(aNode,aOffset,0);
    return GetCharBefore(point, outPoint);
  }
  
  return NS_ERROR_FAILURE;
}

nsresult 
nsWSRunObject::GetCharAfter(WSPoint &aPoint, WSPoint *outPoint)
{
  if (!aPoint.mTextNode || !outPoint)
    return NS_ERROR_NULL_POINTER;
  
  outPoint->mTextNode = nsnull;
  outPoint->mOffset = 0;
  outPoint->mChar = 0;

  nsCOMPtr<nsIDOMNode> pointTextNode(do_QueryInterface(aPoint.mTextNode));
  PRInt32 idx = mNodeArray.IndexOf(pointTextNode);
  if (idx == -1) return NS_OK;  
  PRInt32 numNodes = mNodeArray.Count();
  
  if (aPoint.mOffset < aPoint.mTextNode->TextLength())
  {
    *outPoint = aPoint;
    outPoint->mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset);
  }
  else if (idx < (PRInt32)(numNodes-1))
  {
    nsIDOMNode* node = mNodeArray[idx+1];
    if (!node) return NS_ERROR_FAILURE;
    outPoint->mTextNode = do_QueryInterface(node);
    if (!outPoint->mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
      
      
      outPoint->mTextNode = nsnull;
    }
    outPoint->mOffset = 0;
    outPoint->mChar = GetCharAt(outPoint->mTextNode, 0);
  }
  return NS_OK;
}

nsresult 
nsWSRunObject::GetCharBefore(WSPoint &aPoint, WSPoint *outPoint)
{
  if (!aPoint.mTextNode || !outPoint)
    return NS_ERROR_NULL_POINTER;
  
  outPoint->mTextNode = nsnull;
  outPoint->mOffset = 0;
  outPoint->mChar = 0;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> pointTextNode(do_QueryInterface(aPoint.mTextNode));
  PRInt32 idx = mNodeArray.IndexOf(pointTextNode);
  if (idx == -1) return NS_OK;  
  
  if (aPoint.mOffset != 0)
  {
    *outPoint = aPoint;
    outPoint->mOffset--;
    outPoint->mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset-1);
  }
  else if (idx)
  {
    nsIDOMNode* node = mNodeArray[idx-1];
    if (!node) return NS_ERROR_FAILURE;
    outPoint->mTextNode = do_QueryInterface(node);

    PRUint32 len = outPoint->mTextNode->TextLength();

    if (len)
    {
      outPoint->mOffset = len-1;
      outPoint->mChar = GetCharAt(outPoint->mTextNode, len-1);
    }
  }
  return NS_OK;
}

nsresult 
nsWSRunObject::ConvertToNBSP(WSPoint aPoint, AreaRestriction aAR)
{
  
  
  if (!aPoint.mTextNode)
    return NS_ERROR_NULL_POINTER;

  if (aAR == eOutsideUserSelectAll)
  {
    nsCOMPtr<nsIDOMNode> domnode = do_QueryInterface(aPoint.mTextNode);
    if (domnode)
    {
      nsCOMPtr<nsIDOMNode> san = mHTMLEditor->FindUserSelectAllNode(domnode);
      if (san)
        return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(aPoint.mTextNode));
  if (!textNode)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(textNode));
  
  
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  nsAutoString nbspStr(nbsp);
  nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr, textNode, aPoint.mOffset, PR_TRUE);
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset=0, endOffset=0;
  
  res = GetAsciiWSBounds(eAfter, node, aPoint.mOffset+1, address_of(startNode), 
                         &startOffset, address_of(endNode), &endOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (startNode)
  {
    res = DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  
  return res;
}

nsresult
nsWSRunObject::GetAsciiWSBounds(PRInt16 aDir, nsIDOMNode *aNode, PRInt32 aOffset,
                                nsCOMPtr<nsIDOMNode> *outStartNode, PRInt32 *outStartOffset,
                                nsCOMPtr<nsIDOMNode> *outEndNode, PRInt32 *outEndOffset)
{
  if (!aNode || !outStartNode || !outEndNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset=0, endOffset=0;
  
  nsresult res = NS_OK;
  
  if (aDir & eAfter)
  {
    WSPoint point, tmp;
    res = GetCharAfter(aNode, aOffset, &point);
    if (NS_SUCCEEDED(res) && point.mTextNode)
    {  
      endNode = do_QueryInterface(point.mTextNode);
      endOffset = point.mOffset;
      startNode = endNode;
      startOffset = endOffset;
      
      
      while (nsCRT::IsAsciiSpace(point.mChar))
      {
        endNode = do_QueryInterface(point.mTextNode);
        point.mOffset++;  
        endOffset = point.mOffset;
        tmp = point;
        res = GetCharAfter(tmp, &point);
        if (NS_FAILED(res) || !point.mTextNode) break;
      }
    }
  }
  
  if (aDir & eBefore)
  {
    WSPoint point, tmp;
    res = GetCharBefore(aNode, aOffset, &point);
    if (NS_SUCCEEDED(res) && point.mTextNode)
    {  
      startNode = do_QueryInterface(point.mTextNode);
      startOffset = point.mOffset+1;
      if (!endNode)
      {
        endNode = startNode;
        endOffset = startOffset;
      }
      
      
      while (nsCRT::IsAsciiSpace(point.mChar))
      {
        startNode = do_QueryInterface(point.mTextNode);
        startOffset = point.mOffset;
        tmp = point;
        res = GetCharBefore(tmp, &point);
        if (NS_FAILED(res) || !point.mTextNode) break;
      }
    }
  }  
  
  *outStartNode = startNode;
  *outStartOffset = startOffset;
  *outEndNode = endNode;
  *outEndOffset = endOffset;

  return NS_OK;
}

nsresult
nsWSRunObject::FindRun(nsIDOMNode *aNode, PRInt32 aOffset, WSFragment **outRun, PRBool after)
{
  
  if (!aNode || !outRun)
    return NS_ERROR_NULL_POINTER;
    
  nsresult res = NS_OK;
  WSFragment *run = mStartRun;
  while (run)
  {
    PRInt16 comp = mHTMLEditor->sRangeHelper->ComparePoints(aNode, aOffset, run->mStartNode, run->mStartOffset);
    if (comp <= 0)
    {
      if (after)
      {
        *outRun = run;
        return res;
      }
      else 
      {
        *outRun = nsnull;
        return res;
      }
    }
    comp = mHTMLEditor->sRangeHelper->ComparePoints(aNode, aOffset, run->mEndNode, run->mEndOffset);
    if (comp < 0)
    {
      *outRun = run;
      return res;
    }
    else if (comp == 0)
    {
      if (after)
      {
        *outRun = run->mRight;
        return res;
      }
      else 
      {
        *outRun = run;
        return res;
      }
    }
    if (!run->mRight)
    {
      if (after)
      {
        *outRun = nsnull;
        return res;
      }
      else 
      {
        *outRun = run;
        return res;
      }
    }
    run = run->mRight;
  }
  return res;
}

PRUnichar 
nsWSRunObject::GetCharAt(nsIContent *aTextNode, PRInt32 aOffset)
{
  
  if (!aTextNode)
    return 0;

  PRUint32 len = aTextNode->TextLength();
  if (aOffset < 0 || aOffset >= len) 
    return 0;
    
  return aTextNode->GetText()->CharAt(aOffset);
}

nsresult 
nsWSRunObject::GetWSPointAfter(nsIDOMNode *aNode, PRInt32 aOffset, WSPoint *outPoint)
{
  
  
  
  PRInt32 numNodes, firstNum, curNum, lastNum;
  numNodes = mNodeArray.Count();
  
  if (!numNodes) 
    return NS_OK; 

  firstNum = 0;
  curNum = numNodes/2;
  lastNum = numNodes;
  PRInt16 cmp=0;
  nsCOMPtr<nsIDOMNode>  curNode;
  
  
  
  
  while (curNum != lastNum)
  {
    curNode = mNodeArray[curNum];
    cmp = mHTMLEditor->sRangeHelper->ComparePoints(aNode, aOffset, curNode, 0);
    if (cmp < 0)
      lastNum = curNum;
    else
      firstNum = curNum + 1;
    curNum = (lastNum - firstNum) / 2 + firstNum;
    NS_ASSERTION(firstNum <= curNum && curNum <= lastNum, "Bad binary search");
  }

  
  
  
  if (curNum == mNodeArray.Count()) {
    
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum-1]));
    WSPoint point(textNode, textNode->TextLength(), 0);
    return GetCharAfter(point, outPoint);
  } else {
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum]));
    WSPoint point(textNode, 0, 0);
    return GetCharAfter(point, outPoint);
  }
}

nsresult 
nsWSRunObject::GetWSPointBefore(nsIDOMNode *aNode, PRInt32 aOffset, WSPoint *outPoint)
{
  
  
  
  PRInt32 numNodes, firstNum, curNum, lastNum;
  numNodes = mNodeArray.Count();
  
  if (!numNodes) 
    return NS_OK; 
  
  firstNum = 0;
  curNum = numNodes/2;
  lastNum = numNodes;
  PRInt16 cmp=0;
  nsCOMPtr<nsIDOMNode>  curNode;
  
  
  
  
  while (curNum != lastNum)
  {
    curNode = mNodeArray[curNum];
    cmp = mHTMLEditor->sRangeHelper->ComparePoints(aNode, aOffset, curNode, 0);
    if (cmp < 0)
      lastNum = curNum;
    else
      firstNum = curNum + 1;
    curNum = (lastNum - firstNum) / 2 + firstNum;
    NS_ASSERTION(firstNum <= curNum && curNum <= lastNum, "Bad binary search");
  }

  
  
  
  if (curNum == mNodeArray.Count()) {
    
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum - 1]));
    WSPoint point(textNode, textNode->TextLength(), 0);
    return GetCharBefore(point, outPoint);
  } else {
    
    
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum]));
    WSPoint point(textNode, 0, 0);
    return GetCharBefore(point, outPoint);
  }
}

nsresult
nsWSRunObject::CheckTrailingNBSPOfRun(WSFragment *aRun)
{    
  
  
  if (!aRun) return NS_ERROR_NULL_POINTER;
  WSPoint thePoint;
  PRBool leftCheck = PR_FALSE;
  PRBool spaceNBSP = PR_FALSE;
  PRBool rightCheck = PR_FALSE;
  
  
  if (aRun->mType != eNormalWS) return NS_ERROR_FAILURE;
  
  
  nsresult res = GetCharBefore(aRun->mEndNode, aRun->mEndOffset, &thePoint);
  if (NS_SUCCEEDED(res) && thePoint.mTextNode && thePoint.mChar == nbsp)
  {
    
    WSPoint prevPoint;
    res = GetCharBefore(thePoint, &prevPoint);
    if (NS_SUCCEEDED(res) && prevPoint.mTextNode)
    {
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) leftCheck = PR_TRUE;
      else spaceNBSP = PR_TRUE;
    }
    else if (aRun->mLeftType == eText)    leftCheck = PR_TRUE;
    else if (aRun->mLeftType == eSpecial) leftCheck = PR_TRUE;
    if (leftCheck || spaceNBSP)
    {
      
      if (aRun->mRightType == eText)    rightCheck = PR_TRUE;
      if (aRun->mRightType == eSpecial) rightCheck = PR_TRUE;
      if (aRun->mRightType == eBreak)   rightCheck = PR_TRUE;
      if (aRun->mRightType & eBlock)
      {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        nsCOMPtr<nsIDOMNode> brNode;
        res = mHTMLEditor->CreateBR(aRun->mEndNode, aRun->mEndOffset, address_of(brNode));
        NS_ENSURE_SUCCESS(res, res);
        
        
        res = GetCharBefore(aRun->mEndNode, aRun->mEndOffset, &thePoint);
        NS_ENSURE_SUCCESS(res, res);
        res = GetCharBefore(thePoint, &prevPoint);
        NS_ENSURE_SUCCESS(res, res);
        rightCheck = PR_TRUE;
      }
    }
    if (leftCheck && rightCheck)
    {
      
      
      nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
      if (!textNode)
        return NS_ERROR_NULL_POINTER;
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString spaceStr(PRUnichar(32));
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode, thePoint.mOffset, PR_TRUE);
      NS_ENSURE_SUCCESS(res, res);
  
      
      nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
      res = DeleteChars(delNode, thePoint.mOffset+1, delNode, thePoint.mOffset+2);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (!mPRE && spaceNBSP && rightCheck)  
    {
      
      
      
      
      
      
      nsCOMPtr<nsIDOMNode> startNode, endNode, thenode(do_QueryInterface(prevPoint.mTextNode));
      PRInt32 startOffset, endOffset;
      res = GetAsciiWSBounds(eBoth, thenode, prevPoint.mOffset+1, address_of(startNode), 
                           &startOffset, address_of(endNode), &endOffset);
      NS_ENSURE_SUCCESS(res, res);
      
      
      nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
      res = DeleteChars(delNode, thePoint.mOffset, delNode, thePoint.mOffset+1);
      NS_ENSURE_SUCCESS(res, res);
      
      
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString nbspStr(nbsp);
      nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(startNode));
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr, textNode, startOffset, PR_TRUE);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckTrailingNBSP(WSFragment *aRun, nsIDOMNode *aNode, PRInt32 aOffset)
{    
  
  
  
  
  if (!aRun || !aNode) return NS_ERROR_NULL_POINTER;
  WSPoint thePoint;
  PRBool canConvert = PR_FALSE;
  nsresult res = GetCharBefore(aNode, aOffset, &thePoint);
  if (NS_SUCCEEDED(res) && thePoint.mTextNode && thePoint.mChar == nbsp)
  {
    WSPoint prevPoint;
    res = GetCharBefore(thePoint, &prevPoint);
    if (NS_SUCCEEDED(res) && prevPoint.mTextNode)
    {
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) canConvert = PR_TRUE;
    }
    else if (aRun->mLeftType == eText)    canConvert = PR_TRUE;
    else if (aRun->mLeftType == eSpecial) canConvert = PR_TRUE;
  }
  if (canConvert)
  {
    
    nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
    if (!textNode)
      return NS_ERROR_NULL_POINTER;
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(PRUnichar(32));
    res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode, thePoint.mOffset, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
  
    
    nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
    res = DeleteChars(delNode, thePoint.mOffset+1, delNode, thePoint.mOffset+2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckLeadingNBSP(WSFragment *aRun, nsIDOMNode *aNode, PRInt32 aOffset)
{    
  
  
  
  
  WSPoint thePoint;
  PRBool canConvert = PR_FALSE;
  nsresult res = GetCharAfter(aNode, aOffset, &thePoint);
  if (NS_SUCCEEDED(res) && thePoint.mChar == nbsp)
  {
    WSPoint nextPoint, tmp=thePoint;
    tmp.mOffset++; 
    res = GetCharAfter(tmp, &nextPoint);
    if (NS_SUCCEEDED(res) && nextPoint.mTextNode)
    {
      if (!nsCRT::IsAsciiSpace(nextPoint.mChar)) canConvert = PR_TRUE;
    }
    else if (aRun->mRightType == eText)    canConvert = PR_TRUE;
    else if (aRun->mRightType == eSpecial) canConvert = PR_TRUE;
    else if (aRun->mRightType == eBreak)   canConvert = PR_TRUE;
  }
  if (canConvert)
  {
    
    nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
    if (!textNode)
      return NS_ERROR_NULL_POINTER;
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(PRUnichar(32));
    res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode, thePoint.mOffset, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
  
    
    nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
    res = DeleteChars(delNode, thePoint.mOffset+1, delNode, thePoint.mOffset+2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}


nsresult
nsWSRunObject::ScrubBlockBoundaryInner(nsHTMLEditor *aHTMLEd, 
                                       nsCOMPtr<nsIDOMNode> *aBlock,
                                       BlockBoundary aBoundary)
{
  if (!aBlock || !aHTMLEd)
    return NS_ERROR_NULL_POINTER;
  PRInt32 offset=0;
  if (aBoundary == kBlockEnd)
  {
    PRUint32 uOffset;
    aHTMLEd->GetLengthOfDOMNode(*aBlock, uOffset); 
    offset = uOffset;
  }
  nsWSRunObject theWSObj(aHTMLEd, *aBlock, offset);
  return theWSObj.Scrub();    
}


nsresult
nsWSRunObject::Scrub()
{
  WSFragment *run = mStartRun;
  while (run)
  {
    if (run->mType & (eLeadingWS|eTrailingWS) )
    {
      nsresult res = DeleteChars(run->mStartNode, run->mStartOffset, run->mEndNode, run->mEndOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    run = run->mRight;
  }
  return NS_OK;
}

