




#include "mozilla/Assertions.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsHTMLEditor.h"
#include "nsIContent.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsISupportsImpl.h"
#include "nsRange.h"
#include "nsSelectionState.h"
#include "nsString.h"
#include "nsTextEditUtils.h"
#include "nsTextFragment.h"
#include "nsWSRunObject.h"

const PRUnichar nbsp = 160;

static bool IsBlockNode(nsIDOMNode* node)
{
  bool isBlock (false);
  nsHTMLEditor::NodeIsBlockStatic(node, &isBlock);
  return isBlock;
}


nsWSRunObject::nsWSRunObject(nsHTMLEditor *aEd, nsIDOMNode *aNode, int32_t aOffset) :
mNode(aNode)
,mOffset(aOffset)
,mPRE(false)
,mStartNode()
,mStartOffset(0)
,mStartReason()
,mStartReasonNode()
,mEndNode()
,mEndOffset(0)
,mEndReason()
,mEndReasonNode()
,mFirstNBSPNode()
,mFirstNBSPOffset(0)
,mLastNBSPNode()
,mLastNBSPOffset(0)
,mNodeArray()
,mStartRun(nullptr)
,mEndRun(nullptr)
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
                                  int32_t *aOffset)
{
  NS_ENSURE_TRUE(aBlock && aHTMLEd, NS_ERROR_NULL_POINTER);
  if ((aBoundary == kBlockStart) || (aBoundary == kBlockEnd))
    return ScrubBlockBoundaryInner(aHTMLEd, aBlock, aBoundary);
  
  
  
  NS_ENSURE_TRUE(aOffset, NS_ERROR_NULL_POINTER);
  nsAutoTrackDOMPoint tracker(aHTMLEd->mRangeUpdater, aBlock, aOffset);
  nsWSRunObject theWSObj(aHTMLEd, *aBlock, *aOffset);
  return theWSObj.Scrub();
}

nsresult 
nsWSRunObject::PrepareToJoinBlocks(nsHTMLEditor *aHTMLEd, 
                                   nsIDOMNode *aLeftParent, 
                                   nsIDOMNode *aRightParent)
{
  NS_ENSURE_TRUE(aLeftParent && aRightParent && aHTMLEd, NS_ERROR_NULL_POINTER);
  uint32_t count;
  aHTMLEd->GetLengthOfDOMNode(aLeftParent, count);
  nsWSRunObject leftWSObj(aHTMLEd, aLeftParent, count);
  nsWSRunObject rightWSObj(aHTMLEd, aRightParent, 0);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult 
nsWSRunObject::PrepareToDeleteRange(nsHTMLEditor *aHTMLEd, 
                                    nsCOMPtr<nsIDOMNode> *aStartNode,
                                    int32_t *aStartOffset, 
                                    nsCOMPtr<nsIDOMNode> *aEndNode,
                                    int32_t *aEndOffset)
{
  NS_ENSURE_TRUE(aStartNode && aEndNode && *aStartNode && *aEndNode && aStartOffset && aEndOffset && aHTMLEd, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(aNode && aHTMLEd, NS_ERROR_NULL_POINTER);
  
  int32_t offset;
  nsCOMPtr<nsIDOMNode> parent = aHTMLEd->GetNodeLocation(aNode, &offset);
  
  nsWSRunObject leftWSObj(aHTMLEd, parent, offset);
  nsWSRunObject rightWSObj(aHTMLEd, parent, offset+1);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult 
nsWSRunObject::PrepareToSplitAcrossBlocks(nsHTMLEditor *aHTMLEd, 
                                          nsCOMPtr<nsIDOMNode> *aSplitNode, 
                                          int32_t *aSplitOffset)
{
  NS_ENSURE_TRUE(aSplitNode && aSplitOffset && *aSplitNode && aHTMLEd, NS_ERROR_NULL_POINTER);

  nsAutoTrackDOMPoint tracker(aHTMLEd->mRangeUpdater, aSplitNode, aSplitOffset);
  
  nsWSRunObject wsObj(aHTMLEd, *aSplitNode, *aSplitOffset);

  return wsObj.PrepareToSplitAcrossBlocksPriv();
}





nsresult 
nsWSRunObject::InsertBreak(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                           int32_t *aInOutOffset, 
                           nsCOMPtr<nsIDOMNode> *outBRNode, 
                           nsIEditor::EDirection aSelect)
{
  
  
  NS_ENSURE_TRUE(aInOutParent && aInOutOffset && outBRNode, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  WSFragment *beforeRun, *afterRun;
  FindRun(*aInOutParent, *aInOutOffset, &beforeRun, false);
  FindRun(*aInOutParent, *aInOutOffset, &afterRun, true);
  
  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent, aInOutOffset);

    
    if (!afterRun) {
      
    } else if (afterRun->mType & WSType::trailingWS) {
      
    } else if (afterRun->mType & WSType::leadingWS) {
      
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset, afterRun->mEndNode, afterRun->mEndOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (afterRun->mType == WSType::normalWS) {
      
      
      WSPoint thePoint = GetCharAfter(*aInOutParent, *aInOutOffset);
      if (thePoint.mTextNode && nsCRT::IsAsciiSpace(thePoint.mChar)) {
        WSPoint prevPoint = GetCharBefore(thePoint);
        if (prevPoint.mTextNode && !nsCRT::IsAsciiSpace(prevPoint.mChar)) {
          
          res = ConvertToNBSP(thePoint);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
    }
    
    
    if (!beforeRun) {
      
    } else if (beforeRun->mType & WSType::leadingWS) {
      
    } else if (beforeRun->mType & WSType::trailingWS) {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, *aInOutParent, *aInOutOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (beforeRun->mType == WSType::normalWS) {
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  return mHTMLEditor->CreateBRImpl(aInOutParent, aInOutOffset, outBRNode, aSelect);
}

nsresult 
nsWSRunObject::InsertText(const nsAString& aStringToInsert, 
                          nsCOMPtr<nsIDOMNode> *aInOutParent, 
                          int32_t *aInOutOffset,
                          nsIDOMDocument *aDoc)
{
  
  

  
  
  

  NS_ENSURE_TRUE(aInOutParent && aInOutOffset && aDoc, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  if (aStringToInsert.IsEmpty()) return res;
  
  
  nsAutoString theString(aStringToInsert);
  
  WSFragment *beforeRun, *afterRun;
  FindRun(*aInOutParent, *aInOutOffset, &beforeRun, false);
  FindRun(*aInOutParent, *aInOutOffset, &afterRun, true);
  
  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent, aInOutOffset);

    
    if (!afterRun) {
      
    } else if (afterRun->mType & WSType::trailingWS) {
      
    } else if (afterRun->mType & WSType::leadingWS) {
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset, afterRun->mEndNode, afterRun->mEndOffset,
                         eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (afterRun->mType == WSType::normalWS) {
      
      res = CheckLeadingNBSP(afterRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    if (!beforeRun) {
      
    } else if (beforeRun->mType & WSType::leadingWS) {
      
    } else if (beforeRun->mType & WSType::trailingWS) {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, *aInOutParent, *aInOutOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (beforeRun->mType == WSType::normalWS) {
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  
  
  
  
  if (nsCRT::IsAsciiSpace(theString[0]))
  {
    
    if (beforeRun) {
      if (beforeRun->mType & WSType::leadingWS) {
        theString.SetCharAt(nbsp, 0);
      } else if (beforeRun->mType & WSType::normalWS) {
        WSPoint wspoint = GetCharBefore(*aInOutParent, *aInOutOffset);
        if (wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar)) {
          theString.SetCharAt(nbsp, 0);
        }
      }
    } else {
      if (mStartReason & WSType::block || mStartReason == WSType::br) {
        theString.SetCharAt(nbsp, 0);
      }
    }
  }

  
  uint32_t lastCharIndex = theString.Length()-1;

  if (nsCRT::IsAsciiSpace(theString[lastCharIndex]))
  {
    
    if (afterRun)
    {
      if (afterRun->mType & WSType::trailingWS) {
        theString.SetCharAt(nbsp, lastCharIndex);
      } else if (afterRun->mType & WSType::normalWS) {
        WSPoint wspoint = GetCharAfter(*aInOutParent, *aInOutOffset);
        if (wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar)) {
          theString.SetCharAt(nbsp, lastCharIndex);
        }
      }
    }
    else
    {
      if (mEndReason & WSType::block) {
        theString.SetCharAt(nbsp, lastCharIndex);
      }
    }
  }
  
  
  
  
  
  uint32_t j;
  bool prevWS = false;
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
        prevWS = true;
      }
    }
    else
    {
      prevWS = false;
    }
  }
  
  
  res = mHTMLEditor->InsertTextImpl(theString, aInOutParent, aInOutOffset, aDoc);
  return NS_OK;
}

nsresult 
nsWSRunObject::DeleteWSBackward()
{
  nsresult res = NS_OK;
  WSPoint point = GetCharBefore(mNode, mOffset);
  NS_ENSURE_TRUE(point.mTextNode, NS_OK);  
  
  if (mPRE)  
  {
    if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar == nbsp))
    {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
      int32_t startOffset = point.mOffset;
      int32_t endOffset = point.mOffset+1;
      return DeleteChars(node, startOffset, node, endOffset);
    }
  }
  
  
  
  if (nsCRT::IsAsciiSpace(point.mChar))
  {
    nsCOMPtr<nsIDOMNode> startNode, endNode, node(do_QueryInterface(point.mTextNode));
    int32_t startOffset, endOffset;
    GetAsciiWSBounds(eBoth, node, point.mOffset+1, address_of(startNode),
                     &startOffset, address_of(endNode), &endOffset);
    
    
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(startNode), &startOffset, 
                                              address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  else if (point.mChar == nbsp)
  {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
    
    int32_t startOffset = point.mOffset;
    int32_t endOffset = point.mOffset+1;
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
  WSPoint point = GetCharAfter(mNode, mOffset);
  NS_ENSURE_TRUE(point.mTextNode, NS_OK);  
  
  if (mPRE)  
  {
    if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar == nbsp))
    {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
      int32_t startOffset = point.mOffset;
      int32_t endOffset = point.mOffset+1;
      return DeleteChars(node, startOffset, node, endOffset);
    }
  }
  
  
  
  if (nsCRT::IsAsciiSpace(point.mChar))
  {
    nsCOMPtr<nsIDOMNode> startNode, endNode, node(do_QueryInterface(point.mTextNode));
    int32_t startOffset, endOffset;
    GetAsciiWSBounds(eBoth, node, point.mOffset+1, address_of(startNode),
                     &startOffset, address_of(endNode), &endOffset);
    
    
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(startNode), &startOffset, 
                                              address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  else if (point.mChar == nbsp)
  {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(point.mTextNode));
    
    int32_t startOffset = point.mOffset;
    int32_t endOffset = point.mOffset+1;
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(node), &startOffset, 
                                              address_of(node), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    return DeleteChars(node, startOffset, node, endOffset);
  
  }
  return NS_OK;
}

void
nsWSRunObject::PriorVisibleNode(nsIDOMNode *aNode, 
                                int32_t aOffset, 
                                nsCOMPtr<nsIDOMNode> *outVisNode, 
                                int32_t *outVisOffset,
                                WSType *outType)
{
  
  
  MOZ_ASSERT(aNode && outVisNode && outVisOffset && outType);
    
  *outType = WSType::none;
  WSFragment *run;
  FindRun(aNode, aOffset, &run, false);
  
  
  while (run)
  {
    if (run->mType == WSType::normalWS) {
      WSPoint point = GetCharBefore(aNode, aOffset);
      if (point.mTextNode)
      {
        *outVisNode = do_QueryInterface(point.mTextNode);
        *outVisOffset = point.mOffset+1;
        if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar==nbsp))
        {
          *outType = WSType::normalWS;
        }
        else if (!point.mChar)
        {
          
          *outType = WSType::none;
        }
        else
        {
          *outType = WSType::text;
        }
        return;
      }
      
    }

    run = run->mLeft;
  }
  
  
  *outVisNode = mStartReasonNode;
  *outVisOffset = mStartOffset;  
  *outType = mStartReason;
}


void
nsWSRunObject::NextVisibleNode (nsIDOMNode *aNode, 
                                int32_t aOffset, 
                                nsCOMPtr<nsIDOMNode> *outVisNode, 
                                int32_t *outVisOffset,
                                WSType *outType)
{
  
  
  MOZ_ASSERT(aNode && outVisNode && outVisOffset && outType);
    
  WSFragment *run;
  FindRun(aNode, aOffset, &run, true);
  
  
  while (run)
  {
    if (run->mType == WSType::normalWS) {
      WSPoint point = GetCharAfter(aNode, aOffset);
      if (point.mTextNode)
      {
        *outVisNode = do_QueryInterface(point.mTextNode);
        *outVisOffset = point.mOffset;
        if (nsCRT::IsAsciiSpace(point.mChar) || (point.mChar==nbsp))
        {
          *outType = WSType::normalWS;
        }
        else if (!point.mChar)
        {
          
          *outType = WSType::none;
        }
        else
        {
          *outType = WSType::text;
        }
        return;
      }
      
    }

    run = run->mRight;
  }
  
  
  *outVisNode = mEndReasonNode;
  *outVisOffset = mEndOffset; 
  *outType = mEndReason;
}

nsresult 
nsWSRunObject::AdjustWhitespace()
{
  
  
  
  if (!mLastNBSPNode) {
    
    return NS_OK;
  }
  nsresult res = NS_OK;
  WSFragment *curRun = mStartRun;
  while (curRun)
  {
    
    if (curRun->mType == WSType::normalWS) {
      res = CheckTrailingNBSPOfRun(curRun);
      break;
    }
    curRun = curRun->mRight;
  }
  return res;
}






already_AddRefed<nsIDOMNode>
nsWSRunObject::GetWSBoundingParent()
{
  NS_ENSURE_TRUE(mNode, nullptr);
  nsCOMPtr<nsIDOMNode> wsBoundingParent = mNode;
  while (!IsBlockNode(wsBoundingParent))
  {
    nsCOMPtr<nsIDOMNode> parent;
    wsBoundingParent->GetParentNode(getter_AddRefs(parent));
    if (!parent || !mHTMLEditor->IsEditable(parent))
      break;
    wsBoundingParent.swap(parent);
  }
  return wsBoundingParent.forget();
}

nsresult
nsWSRunObject::GetWSNodes()
{
  
  
  
  nsresult res = NS_OK;
  
  DOMPoint start(mNode, mOffset), end(mNode, mOffset);
  nsCOMPtr<nsIDOMNode> wsBoundingParent = GetWSBoundingParent();

  
  if (mHTMLEditor->IsTextNode(mNode))
  {
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNode));
    const nsTextFragment *textFrag = textNode->GetText();
    
    res = PrependNodeToList(mNode);
    NS_ENSURE_SUCCESS(res, res);
    if (mOffset)
    {
      int32_t pos;
      for (pos=mOffset-1; pos>=0; pos--)
      {
        
        if (uint32_t(pos) >= textFrag->GetLength())
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
            mStartReason = WSType::text;
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
    
    res = GetPreviousWSNode(start, wsBoundingParent, address_of(priorNode));
    NS_ENSURE_SUCCESS(res, res);
    if (priorNode)
    {
      if (IsBlockNode(priorNode))
      {
        start.GetPoint(mStartNode, mStartOffset);
        mStartReason = WSType::otherBlock;
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
        uint32_t len = textNode->TextLength();

        if (len < 1)
        {
          
          
          start.SetPoint(priorNode,0);
        }
        else
        {
          int32_t pos;
          for (pos=len-1; pos>=0; pos--)
          {
            
            if (uint32_t(pos) >= textFrag->GetLength())
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
                mStartReason = WSType::text;
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
          mStartReason = WSType::br;
        else
          mStartReason = WSType::special;
        mStartReasonNode = priorNode;
      }
    }
    else
    {
      
      start.GetPoint(mStartNode, mStartOffset);
      mStartReason = WSType::thisBlock;
      mStartReasonNode = wsBoundingParent;
    } 
  }
  
  
  if (mHTMLEditor->IsTextNode(mNode))
  {
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNode));
    const nsTextFragment *textFrag = textNode->GetText();

    uint32_t len = textNode->TextLength();
    if (uint16_t(mOffset)<len)
    {
      int32_t pos;
      for (pos=mOffset; uint32_t(pos)<len; pos++)
      {
        
        if ((pos<0) || (uint32_t(pos)>=textFrag->GetLength()))
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
            mEndReason = WSType::text;
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
    
    res = GetNextWSNode(end, wsBoundingParent, address_of(nextNode));
    NS_ENSURE_SUCCESS(res, res);
    if (nextNode)
    {
      if (IsBlockNode(nextNode))
      {
        
        end.GetPoint(mEndNode, mEndOffset);
        mEndReason = WSType::otherBlock;
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
        uint32_t len = textNode->TextLength();

        if (len < 1)
        {
          
          
          end.SetPoint(nextNode,0);
        }
        else
        {
          int32_t pos;
          for (pos=0; uint32_t(pos)<len; pos++)
          {
            
            if (uint32_t(pos) >= textFrag->GetLength())
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
                mEndReason = WSType::text;
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
          mEndReason = WSType::br;
        else
          mEndReason = WSType::special;
        mEndReasonNode = nextNode;
      }
    }
    else
    {
      
      end.GetPoint(mEndNode, mEndOffset);
      mEndReason = WSType::thisBlock;
      mEndReasonNode = wsBoundingParent;
    } 
  }

  return NS_OK;
}

void
nsWSRunObject::GetRuns()
{
  ClearRuns();
  
  
  mHTMLEditor->IsPreformatted(mNode, &mPRE);
  
  
  if (mPRE ||
      ((mStartReason == WSType::text || mStartReason == WSType::special) &&
       (mEndReason == WSType::text || mEndReason == WSType::special ||
        mEndReason == WSType::br))) {
    MakeSingleWSRun(WSType::normalWS);
    return;
  }

  
  
  if (!mFirstNBSPNode && !mLastNBSPNode &&
      ((mStartReason & WSType::block) || mStartReason == WSType::br ||
       (mEndReason & WSType::block))) {
    WSType wstype;
    if ((mStartReason & WSType::block) || mStartReason == WSType::br) {
      wstype = WSType::leadingWS;
    }
    if (mEndReason & WSType::block) {
      wstype |= WSType::trailingWS;
    }
    MakeSingleWSRun(wstype);
    return;
  }
  
  
  mStartRun = new WSFragment();
  mStartRun->mStartNode = mStartNode;
  mStartRun->mStartOffset = mStartOffset;
  
  if (mStartReason & WSType::block || mStartReason == WSType::br) {
    
    mStartRun->mType = WSType::leadingWS;
    mStartRun->mEndNode = mFirstNBSPNode;
    mStartRun->mEndOffset = mFirstNBSPOffset;
    mStartRun->mLeftType = mStartReason;
    mStartRun->mRightType = WSType::normalWS;
    
    
    WSFragment *normalRun = new WSFragment();
    mStartRun->mRight = normalRun;
    normalRun->mType = WSType::normalWS;
    normalRun->mStartNode = mFirstNBSPNode;
    normalRun->mStartOffset = mFirstNBSPOffset;
    normalRun->mLeftType = WSType::leadingWS;
    normalRun->mLeft = mStartRun;
    if (mEndReason != WSType::block) {
      
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
        normalRun->mRightType = WSType::trailingWS;
        
        
        WSFragment *lastRun = new WSFragment();
        lastRun->mType = WSType::trailingWS;
        lastRun->mStartNode = mLastNBSPNode;
        lastRun->mStartOffset = mLastNBSPOffset+1;
        lastRun->mEndNode = mEndNode;
        lastRun->mEndOffset = mEndOffset;
        lastRun->mLeftType = WSType::normalWS;
        lastRun->mLeft = normalRun;
        lastRun->mRightType = mEndReason;
        mEndRun = lastRun;
        normalRun->mRight = lastRun;
      }
    }
  } else {
    
    mStartRun->mType = WSType::normalWS;
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
      lastRun->mType = WSType::trailingWS;
      lastRun->mStartNode = mLastNBSPNode;
      lastRun->mStartOffset = mLastNBSPOffset+1;
      lastRun->mLeftType = WSType::normalWS;
      lastRun->mLeft = mStartRun;
      lastRun->mRightType = mEndReason;
      mEndRun = lastRun;
      mStartRun->mRight = lastRun;
      mStartRun->mRightType = WSType::trailingWS;
    }
  }
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

void
nsWSRunObject::MakeSingleWSRun(WSType aType)
{
  mStartRun = new WSFragment();

  mStartRun->mStartNode   = mStartNode;
  mStartRun->mStartOffset = mStartOffset;
  mStartRun->mType        = aType;
  mStartRun->mEndNode     = mEndNode;
  mStartRun->mEndOffset   = mEndOffset;
  mStartRun->mLeftType    = mStartReason;
  mStartRun->mRightType   = mEndReason;
  
  mEndRun  = mStartRun;
}

nsresult 
nsWSRunObject::PrependNodeToList(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  if (!mNodeArray.InsertObjectAt(aNode, 0))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult 
nsWSRunObject::AppendNodeToList(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  if (!mNodeArray.AppendObject(aNode))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult 
nsWSRunObject::GetPreviousWSNode(nsIDOMNode *aStartNode, 
                                 nsIDOMNode *aBlockParent, 
                                 nsCOMPtr<nsIDOMNode> *aPriorNode)
{
  
  
  
  NS_ENSURE_TRUE(aStartNode && aBlockParent && aPriorNode, NS_ERROR_NULL_POINTER);
  
  nsresult res = aStartNode->GetPreviousSibling(getter_AddRefs(*aPriorNode));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> temp, curNode = aStartNode;
  while (!*aPriorNode)
  {
    
    res = curNode->GetParentNode(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(temp, NS_ERROR_NULL_POINTER);
    if (temp == aBlockParent)
    {
      
      *aPriorNode = nullptr;
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
  int32_t offset;
  aPoint.GetPoint(node, offset);
  return GetPreviousWSNode(node,offset,aBlockParent,aPriorNode);
}

nsresult 
nsWSRunObject::GetPreviousWSNode(nsIDOMNode *aStartNode,
                                 int32_t aOffset,
                                 nsIDOMNode *aBlockParent, 
                                 nsCOMPtr<nsIDOMNode> *aPriorNode)
{
  
  
  
  NS_ENSURE_TRUE(aStartNode && aBlockParent && aPriorNode, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_STATE(startContent);
  nsIContent *priorContent = startContent->GetChildAt(aOffset - 1);
  NS_ENSURE_TRUE(priorContent, NS_ERROR_NULL_POINTER);
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
  
  
  
  NS_ENSURE_TRUE(aStartNode && aBlockParent && aNextNode, NS_ERROR_NULL_POINTER);
  
  *aNextNode = 0;
  nsresult res = aStartNode->GetNextSibling(getter_AddRefs(*aNextNode));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> temp, curNode = aStartNode;
  while (!*aNextNode)
  {
    
    res = curNode->GetParentNode(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(temp, NS_ERROR_NULL_POINTER);
    if (temp == aBlockParent)
    {
      
      
      *aNextNode = nullptr;
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
  int32_t offset;
  aPoint.GetPoint(node, offset);
  return GetNextWSNode(node,offset,aBlockParent,aNextNode);
}

nsresult 
nsWSRunObject::GetNextWSNode(nsIDOMNode *aStartNode,
                             int32_t aOffset,
                             nsIDOMNode *aBlockParent, 
                             nsCOMPtr<nsIDOMNode> *aNextNode)
{
  
  
  NS_ENSURE_TRUE(aStartNode && aBlockParent && aNextNode, NS_ERROR_NULL_POINTER);
  *aNextNode = 0;

  if (mHTMLEditor->IsTextNode(aStartNode))
    return GetNextWSNode(aStartNode, aBlockParent, aNextNode);
  if (!mHTMLEditor->IsContainer(aStartNode))
    return GetNextWSNode(aStartNode, aBlockParent, aNextNode);
  
  nsCOMPtr<nsIContent> startContent( do_QueryInterface(aStartNode) );
  NS_ENSURE_STATE(startContent);
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
  
  
  
  
  
  
  
  NS_ENSURE_TRUE(aEndObject, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  
  
  WSFragment *beforeRun, *afterRun;
  FindRun(mNode, mOffset, &beforeRun, false);
  aEndObject->FindRun(aEndObject->mNode, aEndObject->mOffset, &afterRun, true);
  
  
  if (afterRun && (afterRun->mType & WSType::leadingWS)) {
    res = aEndObject->DeleteChars(aEndObject->mNode, aEndObject->mOffset, afterRun->mEndNode, afterRun->mEndOffset,
                                  eOutsideUserSelectAll);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  if (afterRun && afterRun->mType == WSType::normalWS && !aEndObject->mPRE) {
    if ((beforeRun && (beforeRun->mType & WSType::leadingWS)) ||
        (!beforeRun && ((mStartReason & WSType::block) ||
                        mStartReason == WSType::br))) {
      
      WSPoint point = aEndObject->GetCharAfter(aEndObject->mNode,
                                               aEndObject->mOffset);
      if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
      {
        res = aEndObject->ConvertToNBSP(point, eOutsideUserSelectAll);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  
  if (beforeRun && (beforeRun->mType & WSType::trailingWS)) {
    res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset, mNode, mOffset,
                      eOutsideUserSelectAll);
    NS_ENSURE_SUCCESS(res, res);
  } else if (beforeRun && beforeRun->mType == WSType::normalWS && !mPRE) {
    if ((afterRun && (afterRun->mType & WSType::trailingWS)) ||
        (afterRun && afterRun->mType == WSType::normalWS) ||
        (!afterRun && (aEndObject->mEndReason & WSType::block))) {
      
      WSPoint point = GetCharBefore(mNode, mOffset);
      if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
      {
        nsCOMPtr<nsIDOMNode> wsStartNode, wsEndNode;
        int32_t wsStartOffset, wsEndOffset;
        GetAsciiWSBounds(eBoth, mNode, mOffset, address_of(wsStartNode),
                         &wsStartOffset, address_of(wsEndNode), &wsEndOffset);
        point.mTextNode = do_QueryInterface(wsStartNode);
        if (!point.mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
          
          
          point.mTextNode = nullptr;
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
  FindRun(mNode, mOffset, &beforeRun, false);
  FindRun(mNode, mOffset, &afterRun, true);
  
  
  if (afterRun && afterRun->mType == WSType::normalWS) {
    
    WSPoint point = GetCharAfter(mNode, mOffset);
    if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
    {
      res = ConvertToNBSP(point);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  if (beforeRun && beforeRun->mType == WSType::normalWS) {
    
    WSPoint point = GetCharBefore(mNode, mOffset);
    if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
    {
      nsCOMPtr<nsIDOMNode> wsStartNode, wsEndNode;
      int32_t wsStartOffset, wsEndOffset;
      GetAsciiWSBounds(eBoth, mNode, mOffset, address_of(wsStartNode),
                       &wsStartOffset, address_of(wsEndNode), &wsEndOffset);
      point.mTextNode = do_QueryInterface(wsStartNode);
      if (!point.mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
        
        
        point.mTextNode = nullptr;
      }
      point.mOffset = wsStartOffset;
      res = ConvertToNBSP(point);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return res;
}

nsresult 
nsWSRunObject::DeleteChars(nsIDOMNode *aStartNode, int32_t aStartOffset, 
                           nsIDOMNode *aEndNode, int32_t aEndOffset,
                           AreaRestriction aAR)
{
  
  
  NS_ENSURE_TRUE(aStartNode && aEndNode, NS_ERROR_NULL_POINTER);

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
  int32_t idx = mNodeArray.IndexOf(aStartNode);
  if (idx==-1) idx = 0; 
                        
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMCharacterData> textnode;
  nsRefPtr<nsRange> range;

  if (aStartNode == aEndNode)
  {
    textnode = do_QueryInterface(aStartNode);
    if (textnode)
    {
      return mHTMLEditor->DeleteText(textnode, (uint32_t)aStartOffset, 
                                     (uint32_t)(aEndOffset-aStartOffset));
    }
  }

  int32_t count = mNodeArray.Count();
  while (idx < count)
  {
    node = mNodeArray[idx];
    if (!node)
      break;  
    if (node == aStartNode)
    {
      textnode = do_QueryInterface(node);
      uint32_t len;
      textnode->GetLength(&len);
      if (uint32_t(aStartOffset)<len)
      {
        res = mHTMLEditor->DeleteText(textnode, (uint32_t)aStartOffset, len-aStartOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    else if (node == aEndNode)
    {
      if (aEndOffset)
      {
        textnode = do_QueryInterface(node);
        res = mHTMLEditor->DeleteText(textnode, 0, (uint32_t)aEndOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      break;
    }
    else
    {
      if (!range)
      {
        range = new nsRange();
        res = range->SetStart(aStartNode, aStartOffset);
        NS_ENSURE_SUCCESS(res, res);
        res = range->SetEnd(aEndNode, aEndOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      bool nodeBefore, nodeAfter;
      nsCOMPtr<nsIContent> content (do_QueryInterface(node));
      res = nsRange::CompareNodeToRange(content, range, &nodeBefore, &nodeAfter);
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

nsWSRunObject::WSPoint
nsWSRunObject::GetCharAfter(nsIDOMNode *aNode, int32_t aOffset)
{
  MOZ_ASSERT(aNode);

  int32_t idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) 
  {
    
    return GetWSPointAfter(aNode, aOffset);
  }
  else
  {
    
    WSPoint point(aNode,aOffset,0);
    return GetCharAfter(point);
  }
}

nsWSRunObject::WSPoint
nsWSRunObject::GetCharBefore(nsIDOMNode *aNode, int32_t aOffset)
{
  MOZ_ASSERT(aNode);

  int32_t idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) 
  {
    
    return GetWSPointBefore(aNode, aOffset);
  }
  else
  {
    
    WSPoint point(aNode,aOffset,0);
    return GetCharBefore(point);
  }
}

nsWSRunObject::WSPoint
nsWSRunObject::GetCharAfter(const WSPoint &aPoint)
{
  MOZ_ASSERT(aPoint.mTextNode);
  
  WSPoint outPoint;
  outPoint.mTextNode = nullptr;
  outPoint.mOffset = 0;
  outPoint.mChar = 0;

  nsCOMPtr<nsIDOMNode> pointTextNode(do_QueryInterface(aPoint.mTextNode));
  int32_t idx = mNodeArray.IndexOf(pointTextNode);
  if (idx == -1) {
    
    return outPoint;
  }
  int32_t numNodes = mNodeArray.Count();
  
  if (uint16_t(aPoint.mOffset) < aPoint.mTextNode->TextLength())
  {
    outPoint = aPoint;
    outPoint.mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset);
    return outPoint;
  } else if (idx + 1 < (int32_t)numNodes) {
    nsIDOMNode* node = mNodeArray[idx+1];
    MOZ_ASSERT(node);
    outPoint.mTextNode = do_QueryInterface(node);
    if (!outPoint.mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
      
      
      outPoint.mTextNode = nullptr;
    }
    outPoint.mOffset = 0;
    outPoint.mChar = GetCharAt(outPoint.mTextNode, 0);
  }
  return outPoint;
}

nsWSRunObject::WSPoint
nsWSRunObject::GetCharBefore(const WSPoint &aPoint)
{
  MOZ_ASSERT(aPoint.mTextNode);
  
  WSPoint outPoint;
  outPoint.mTextNode = nullptr;
  outPoint.mOffset = 0;
  outPoint.mChar = 0;
  
  nsCOMPtr<nsIDOMNode> pointTextNode(do_QueryInterface(aPoint.mTextNode));
  int32_t idx = mNodeArray.IndexOf(pointTextNode);
  if (idx == -1) {
    
    return outPoint;
  }
  
  if (aPoint.mOffset != 0)
  {
    outPoint = aPoint;
    outPoint.mOffset--;
    outPoint.mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset-1);
    return outPoint;
  }
  else if (idx)
  {
    nsIDOMNode* node = mNodeArray[idx-1];
    MOZ_ASSERT(node);
    outPoint.mTextNode = do_QueryInterface(node);

    uint32_t len = outPoint.mTextNode->TextLength();

    if (len)
    {
      outPoint.mOffset = len-1;
      outPoint.mChar = GetCharAt(outPoint.mTextNode, len-1);
    }
  }
  return outPoint;
}

nsresult 
nsWSRunObject::ConvertToNBSP(WSPoint aPoint, AreaRestriction aAR)
{
  
  
  NS_ENSURE_TRUE(aPoint.mTextNode, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(textNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(textNode));
  
  
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  nsAutoString nbspStr(nbsp);
  nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr, textNode, aPoint.mOffset, true);
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  int32_t startOffset=0, endOffset=0;
  
  GetAsciiWSBounds(eAfter, node, aPoint.mOffset+1, address_of(startNode),
                   &startOffset, address_of(endNode), &endOffset);
  
  
  if (startNode)
  {
    res = DeleteChars(startNode, startOffset, endNode, endOffset);
  }
  
  return res;
}

void
nsWSRunObject::GetAsciiWSBounds(int16_t aDir, nsIDOMNode *aNode, int32_t aOffset,
                                nsCOMPtr<nsIDOMNode> *outStartNode, int32_t *outStartOffset,
                                nsCOMPtr<nsIDOMNode> *outEndNode, int32_t *outEndOffset)
{
  MOZ_ASSERT(aNode && outStartNode && outEndNode);

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  int32_t startOffset=0, endOffset=0;
  
  if (aDir & eAfter)
  {
    WSPoint point = GetCharAfter(aNode, aOffset);
    if (point.mTextNode) {
      
      endNode = do_QueryInterface(point.mTextNode);
      endOffset = point.mOffset;
      startNode = endNode;
      startOffset = endOffset;
      
      
      while (nsCRT::IsAsciiSpace(point.mChar))
      {
        endNode = do_QueryInterface(point.mTextNode);
        point.mOffset++;  
        endOffset = point.mOffset;
        point = GetCharAfter(point);
        if (!point.mTextNode) {
          break;
        }
      }
    }
  }
  
  if (aDir & eBefore)
  {
    WSPoint point = GetCharBefore(aNode, aOffset);
    if (point.mTextNode) {
      
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
        point = GetCharBefore(point);
        if (!point.mTextNode) {
          break;
        }
      }
    }
  }  
  
  *outStartNode = startNode;
  *outStartOffset = startOffset;
  *outEndNode = endNode;
  *outEndOffset = endOffset;
}

void
nsWSRunObject::FindRun(nsIDOMNode *aNode, int32_t aOffset, WSFragment **outRun, bool after)
{
  *outRun = nullptr;
  
  MOZ_ASSERT(aNode && outRun);
    
  WSFragment *run = mStartRun;
  while (run)
  {
    int16_t comp = nsContentUtils::ComparePoints(aNode, aOffset, run->mStartNode,
                                                 run->mStartOffset);
    if (comp <= 0)
    {
      if (after)
      {
        *outRun = run;
      }
      else 
      {
        *outRun = nullptr;
      }
      return;
    }
    comp = nsContentUtils::ComparePoints(aNode, aOffset,
                                         run->mEndNode, run->mEndOffset);
    if (comp < 0)
    {
      *outRun = run;
      return;
    }
    else if (comp == 0)
    {
      if (after)
      {
        *outRun = run->mRight;
      }
      else 
      {
        *outRun = run;
      }
      return;
    }
    if (!run->mRight)
    {
      if (after)
      {
        *outRun = nullptr;
      }
      else 
      {
        *outRun = run;
      }
      return;
    }
    run = run->mRight;
  }
}

PRUnichar 
nsWSRunObject::GetCharAt(nsIContent *aTextNode, int32_t aOffset)
{
  
  NS_ENSURE_TRUE(aTextNode, 0);

  int32_t len = int32_t(aTextNode->TextLength());
  if (aOffset < 0 || aOffset >= len)
    return 0;
    
  return aTextNode->GetText()->CharAt(aOffset);
}

nsWSRunObject::WSPoint
nsWSRunObject::GetWSPointAfter(nsIDOMNode *aNode, int32_t aOffset)
{
  
  
  
  int32_t numNodes, firstNum, curNum, lastNum;
  numNodes = mNodeArray.Count();
  
  if (!numNodes) {
    
    WSPoint outPoint;
    return outPoint;
  }

  firstNum = 0;
  curNum = numNodes/2;
  lastNum = numNodes;
  int16_t cmp=0;
  nsCOMPtr<nsIDOMNode>  curNode;
  
  
  
  
  while (curNum != lastNum)
  {
    curNode = mNodeArray[curNum];
    cmp = nsContentUtils::ComparePoints(aNode, aOffset, curNode, 0);
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
    return GetCharAfter(point);
  } else {
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum]));
    WSPoint point(textNode, 0, 0);
    return GetCharAfter(point);
  }
}

nsWSRunObject::WSPoint
nsWSRunObject::GetWSPointBefore(nsIDOMNode *aNode, int32_t aOffset)
{
  
  
  
  int32_t numNodes, firstNum, curNum, lastNum;
  numNodes = mNodeArray.Count();
  
  if (!numNodes) {
    
    WSPoint outPoint;
    return outPoint;
  }
  
  firstNum = 0;
  curNum = numNodes/2;
  lastNum = numNodes;
  int16_t cmp=0;
  nsCOMPtr<nsIDOMNode>  curNode;
  
  
  
  
  while (curNum != lastNum)
  {
    curNode = mNodeArray[curNum];
    cmp = nsContentUtils::ComparePoints(aNode, aOffset, curNode, 0);
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
    return GetCharBefore(point);
  } else {
    
    
    
    nsCOMPtr<nsIContent> textNode(do_QueryInterface(mNodeArray[curNum]));
    WSPoint point(textNode, 0, 0);
    return GetCharBefore(point);
  }
}

nsresult
nsWSRunObject::CheckTrailingNBSPOfRun(WSFragment *aRun)
{    
  
  
  NS_ENSURE_TRUE(aRun, NS_ERROR_NULL_POINTER);
  nsresult res;
  bool leftCheck = false;
  bool spaceNBSP = false;
  bool rightCheck = false;
  
  
  if (aRun->mType != WSType::normalWS) {
    return NS_ERROR_FAILURE;
  }
  
  
  WSPoint thePoint = GetCharBefore(aRun->mEndNode, aRun->mEndOffset);
  if (thePoint.mTextNode && thePoint.mChar == nbsp) {
    
    WSPoint prevPoint = GetCharBefore(thePoint);
    if (prevPoint.mTextNode) {
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) leftCheck = true;
      else spaceNBSP = true;
    } else if (aRun->mLeftType == WSType::text) {
      leftCheck = true;
    } else if (aRun->mLeftType == WSType::special) {
      leftCheck = true;
    }
    if (leftCheck || spaceNBSP)
    {
      
      if (aRun->mRightType == WSType::text) {
        rightCheck = true;
      }
      if (aRun->mRightType == WSType::special) {
        rightCheck = true;
      }
      if (aRun->mRightType == WSType::br) {
        rightCheck = true;
      }
      if ((aRun->mRightType & WSType::block) &&
          IsBlockNode(nsCOMPtr<nsIDOMNode>(GetWSBoundingParent()))) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIDOMNode> brNode;
        res = mHTMLEditor->CreateBR(aRun->mEndNode, aRun->mEndOffset, address_of(brNode));
        NS_ENSURE_SUCCESS(res, res);

        
        thePoint = GetCharBefore(aRun->mEndNode, aRun->mEndOffset);
        prevPoint = GetCharBefore(thePoint);
        rightCheck = true;
      }
    }
    if (leftCheck && rightCheck)
    {
      
      
      nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
      NS_ENSURE_TRUE(textNode, NS_ERROR_NULL_POINTER);
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString spaceStr(PRUnichar(32));
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode, thePoint.mOffset, true);
      NS_ENSURE_SUCCESS(res, res);
  
      
      nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
      res = DeleteChars(delNode, thePoint.mOffset+1, delNode, thePoint.mOffset+2);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (!mPRE && spaceNBSP && rightCheck)  
    {
      
      
      
      
      
      
      nsCOMPtr<nsIDOMNode> startNode, endNode, thenode(do_QueryInterface(prevPoint.mTextNode));
      int32_t startOffset, endOffset;
      GetAsciiWSBounds(eBoth, thenode, prevPoint.mOffset+1, address_of(startNode),
                       &startOffset, address_of(endNode), &endOffset);
      
      
      nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
      res = DeleteChars(delNode, thePoint.mOffset, delNode, thePoint.mOffset+1);
      NS_ENSURE_SUCCESS(res, res);
      
      
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString nbspStr(nbsp);
      nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(startNode));
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr, textNode, startOffset, true);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckTrailingNBSP(WSFragment *aRun, nsIDOMNode *aNode, int32_t aOffset)
{    
  
  
  
  
  NS_ENSURE_TRUE(aRun && aNode, NS_ERROR_NULL_POINTER);
  bool canConvert = false;
  WSPoint thePoint = GetCharBefore(aNode, aOffset);
  if (thePoint.mTextNode && thePoint.mChar == nbsp) {
    WSPoint prevPoint = GetCharBefore(thePoint);
    if (prevPoint.mTextNode) {
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) canConvert = true;
    } else if (aRun->mLeftType == WSType::text) {
      canConvert = true;
    } else if (aRun->mLeftType == WSType::special) {
      canConvert = true;
    }
  }
  if (canConvert)
  {
    
    nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
    NS_ENSURE_TRUE(textNode, NS_ERROR_NULL_POINTER);
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(PRUnichar(32));
    nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode,
                                                           thePoint.mOffset,
                                                           true);
    NS_ENSURE_SUCCESS(res, res);
  
    
    nsCOMPtr<nsIDOMNode> delNode(do_QueryInterface(thePoint.mTextNode));
    res = DeleteChars(delNode, thePoint.mOffset+1, delNode, thePoint.mOffset+2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckLeadingNBSP(WSFragment *aRun, nsIDOMNode *aNode, int32_t aOffset)
{    
  
  
  
  
  bool canConvert = false;
  WSPoint thePoint = GetCharAfter(aNode, aOffset);
  if (thePoint.mChar == nbsp) {
    WSPoint tmp = thePoint;
    tmp.mOffset++; 
    WSPoint nextPoint = GetCharAfter(tmp);
    if (nextPoint.mTextNode) {
      if (!nsCRT::IsAsciiSpace(nextPoint.mChar)) canConvert = true;
    } else if (aRun->mRightType == WSType::text) {
      canConvert = true;
    } else if (aRun->mRightType == WSType::special) {
      canConvert = true;
    } else if (aRun->mRightType == WSType::br) {
      canConvert = true;
    }
  }
  if (canConvert)
  {
    
    nsCOMPtr<nsIDOMCharacterData> textNode(do_QueryInterface(thePoint.mTextNode));
    NS_ENSURE_TRUE(textNode, NS_ERROR_NULL_POINTER);
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(PRUnichar(32));
    nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr, textNode,
                                                           thePoint.mOffset,
                                                           true);
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
  NS_ENSURE_TRUE(aBlock && aHTMLEd, NS_ERROR_NULL_POINTER);
  int32_t offset=0;
  if (aBoundary == kBlockEnd)
  {
    uint32_t uOffset;
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
    if (run->mType & (WSType::leadingWS | WSType::trailingWS)) {
      nsresult res = DeleteChars(run->mStartNode, run->mStartOffset, run->mEndNode, run->mEndOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    run = run->mRight;
  }
  return NS_OK;
}
