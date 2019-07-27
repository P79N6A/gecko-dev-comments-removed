




#include "nsWSRunObject.h"

#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/Assertions.h"
#include "mozilla/Casting.h"
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
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsISupportsImpl.h"
#include "nsRange.h"
#include "nsSelectionState.h"
#include "nsString.h"
#include "nsTextEditUtils.h"
#include "nsTextFragment.h"

using namespace mozilla;
using namespace mozilla::dom;

const char16_t nbsp = 160;

static bool IsBlockNode(nsINode* node)
{
  return node && node->IsElement() &&
         nsHTMLEditor::NodeIsBlockStatic(node->AsElement());
}


nsWSRunObject::nsWSRunObject(nsHTMLEditor* aEd, nsINode* aNode, int32_t aOffset)
  : mNode(aNode)
  , mOffset(aOffset)
  , mPRE(false)
  , mStartNode()
  , mStartOffset(0)
  , mStartReason()
  , mStartReasonNode()
  , mEndNode()
  , mEndOffset(0)
  , mEndReason()
  , mEndReasonNode()
  , mFirstNBSPNode()
  , mFirstNBSPOffset(0)
  , mLastNBSPNode()
  , mLastNBSPOffset(0)
  , mNodeArray()
  , mStartRun(nullptr)
  , mEndRun(nullptr)
  , mHTMLEditor(aEd)
{
  GetWSNodes();
  GetRuns();
}

nsWSRunObject::nsWSRunObject(nsHTMLEditor *aEd, nsIDOMNode *aNode, int32_t aOffset) :
mNode(do_QueryInterface(aNode))
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
nsWSRunObject::ScrubBlockBoundary(nsHTMLEditor* aHTMLEd,
                                  BlockBoundary aBoundary,
                                  nsINode* aBlock,
                                  int32_t aOffset)
{
  NS_ENSURE_TRUE(aHTMLEd && aBlock, NS_ERROR_NULL_POINTER);

  int32_t offset;
  if (aBoundary == kBlockStart) {
    offset = 0;
  } else if (aBoundary == kBlockEnd) {
    offset = aBlock->Length();
  } else {
    
    
    NS_ENSURE_STATE(aOffset >= 0);
    offset = aOffset;
  }
  
  nsWSRunObject theWSObj(aHTMLEd, aBlock, offset);
  return theWSObj.Scrub();
}

nsresult
nsWSRunObject::PrepareToJoinBlocks(nsHTMLEditor* aHTMLEd,
                                   Element* aLeftBlock,
                                   Element* aRightBlock)
{
  NS_ENSURE_TRUE(aLeftBlock && aRightBlock && aHTMLEd, NS_ERROR_NULL_POINTER);

  nsWSRunObject leftWSObj(aHTMLEd, aLeftBlock, aLeftBlock->Length());
  nsWSRunObject rightWSObj(aHTMLEd, aRightBlock, 0);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult
nsWSRunObject::PrepareToDeleteRange(nsHTMLEditor* aHTMLEd,
                                    nsCOMPtr<nsINode>* aStartNode,
                                    int32_t* aStartOffset,
                                    nsCOMPtr<nsINode>* aEndNode,
                                    int32_t* aEndOffset)
{
  NS_ENSURE_TRUE(aHTMLEd && aStartNode && *aStartNode && aStartOffset &&
                 aEndNode && *aEndNode && aEndOffset, NS_ERROR_NULL_POINTER);

  nsAutoTrackDOMPoint trackerStart(aHTMLEd->mRangeUpdater, aStartNode,
                                   aStartOffset);
  nsAutoTrackDOMPoint trackerEnd(aHTMLEd->mRangeUpdater, aEndNode, aEndOffset);

  nsWSRunObject leftWSObj(aHTMLEd, *aStartNode, *aStartOffset);
  nsWSRunObject rightWSObj(aHTMLEd, *aEndNode, *aEndOffset);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult
nsWSRunObject::PrepareToDeleteNode(nsHTMLEditor* aHTMLEd,
                                   nsIContent* aContent)
{
  NS_ENSURE_TRUE(aContent && aHTMLEd, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> parent = aContent->GetParentNode();
  NS_ENSURE_STATE(parent);
  int32_t offset = parent->IndexOf(aContent);

  nsWSRunObject leftWSObj(aHTMLEd, parent, offset);
  nsWSRunObject rightWSObj(aHTMLEd, parent, offset + 1);

  return leftWSObj.PrepareToDeleteRangePriv(&rightWSObj);
}

nsresult
nsWSRunObject::PrepareToSplitAcrossBlocks(nsHTMLEditor* aHTMLEd,
                                          nsCOMPtr<nsINode>* aSplitNode,
                                          int32_t* aSplitOffset)
{
  NS_ENSURE_TRUE(aHTMLEd && aSplitNode && *aSplitNode && aSplitOffset,
                 NS_ERROR_NULL_POINTER);

  nsAutoTrackDOMPoint tracker(aHTMLEd->mRangeUpdater, aSplitNode, aSplitOffset);

  nsWSRunObject wsObj(aHTMLEd, *aSplitNode, *aSplitOffset);

  return wsObj.PrepareToSplitAcrossBlocksPriv();
}





already_AddRefed<Element>
nsWSRunObject::InsertBreak(nsCOMPtr<nsINode>* aInOutParent,
                           int32_t* aInOutOffset,
                           nsIEditor::EDirection aSelect)
{
  
  
  
  NS_ENSURE_TRUE(aInOutParent && aInOutOffset, nullptr);

  nsresult res = NS_OK;
  WSFragment *beforeRun, *afterRun;
  FindRun(*aInOutParent, *aInOutOffset, &beforeRun, false);
  FindRun(*aInOutParent, *aInOutOffset, &afterRun, true);

  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent,
                                aInOutOffset);

    
    if (!afterRun || (afterRun->mType & WSType::trailingWS)) {
      
    } else if (afterRun->mType & WSType::leadingWS) {
      
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset,
                        afterRun->mEndNode, afterRun->mEndOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, nullptr);
    } else if (afterRun->mType == WSType::normalWS) {
      
      
      WSPoint thePoint = GetCharAfter(*aInOutParent, *aInOutOffset);
      if (thePoint.mTextNode && nsCRT::IsAsciiSpace(thePoint.mChar)) {
        WSPoint prevPoint = GetCharBefore(thePoint);
        if (prevPoint.mTextNode && !nsCRT::IsAsciiSpace(prevPoint.mChar)) {
          
          res = ConvertToNBSP(thePoint);
          NS_ENSURE_SUCCESS(res, nullptr);
        }
      }
    }

    
    if (!beforeRun || (beforeRun->mType & WSType::leadingWS)) {
      
    } else if (beforeRun->mType & WSType::trailingWS) {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset,
                        *aInOutParent, *aInOutOffset,
                        eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, nullptr);
    } else if (beforeRun->mType == WSType::normalWS) {
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, nullptr);
    }
  }

  
  return mHTMLEditor->CreateBRImpl(aInOutParent, aInOutOffset, aSelect);
}

nsresult
nsWSRunObject::InsertText(const nsAString& aStringToInsert,
                          nsCOMPtr<nsINode>* aInOutParent,
                          int32_t* aInOutOffset,
                          nsIDocument* aDoc)
{
  
  
  

  
  
  

  NS_ENSURE_TRUE(aInOutParent && aInOutOffset && aDoc, NS_ERROR_NULL_POINTER);

  if (aStringToInsert.IsEmpty()) {
    return NS_OK;
  }

  nsAutoString theString(aStringToInsert);

  WSFragment *beforeRun, *afterRun;
  FindRun(*aInOutParent, *aInOutOffset, &beforeRun, false);
  FindRun(*aInOutParent, *aInOutOffset, &afterRun, true);

  nsresult res;
  {
    
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, aInOutParent,
                                aInOutOffset);

    
    if (!afterRun || afterRun->mType & WSType::trailingWS) {
      
    } else if (afterRun->mType & WSType::leadingWS) {
      
      
      res = DeleteChars(*aInOutParent, *aInOutOffset, afterRun->mEndNode,
                        afterRun->mEndOffset, eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (afterRun->mType == WSType::normalWS) {
      
      
      res = CheckLeadingNBSP(afterRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    if (!beforeRun || beforeRun->mType & WSType::leadingWS) {
      
    } else if (beforeRun->mType & WSType::trailingWS) {
      
      
      res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset,
                        *aInOutParent, *aInOutOffset, eOutsideUserSelectAll);
      NS_ENSURE_SUCCESS(res, res);
    } else if (beforeRun->mType == WSType::normalWS) {
      
      
      res = CheckTrailingNBSP(beforeRun, *aInOutParent, *aInOutOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  
  

  if (nsCRT::IsAsciiSpace(theString[0])) {
    
    if (beforeRun) {
      if (beforeRun->mType & WSType::leadingWS) {
        theString.SetCharAt(nbsp, 0);
      } else if (beforeRun->mType & WSType::normalWS) {
        WSPoint wspoint = GetCharBefore(*aInOutParent, *aInOutOffset);
        if (wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar)) {
          theString.SetCharAt(nbsp, 0);
        }
      }
    } else if (mStartReason & WSType::block || mStartReason == WSType::br) {
      theString.SetCharAt(nbsp, 0);
    }
  }

  
  uint32_t lastCharIndex = theString.Length() - 1;

  if (nsCRT::IsAsciiSpace(theString[lastCharIndex])) {
    
    if (afterRun) {
      if (afterRun->mType & WSType::trailingWS) {
        theString.SetCharAt(nbsp, lastCharIndex);
      } else if (afterRun->mType & WSType::normalWS) {
        WSPoint wspoint = GetCharAfter(*aInOutParent, *aInOutOffset);
        if (wspoint.mTextNode && nsCRT::IsAsciiSpace(wspoint.mChar)) {
          theString.SetCharAt(nbsp, lastCharIndex);
        }
      }
    } else if (mEndReason & WSType::block) {
      theString.SetCharAt(nbsp, lastCharIndex);
    }
  }

  
  
  
  
  bool prevWS = false;
  for (uint32_t i = 0; i <= lastCharIndex; i++) {
    if (nsCRT::IsAsciiSpace(theString[i])) {
      if (prevWS) {
        
        theString.SetCharAt(nbsp, i - 1);
      } else {
        prevWS = true;
      }
    } else {
      prevWS = false;
    }
  }

  
  res = mHTMLEditor->InsertTextImpl(theString, aInOutParent, aInOutOffset,
                                    aDoc);
  return NS_OK;
}

nsresult
nsWSRunObject::DeleteWSBackward()
{
  WSPoint point = GetCharBefore(mNode, mOffset);
  NS_ENSURE_TRUE(point.mTextNode, NS_OK);  

  if (mPRE) {
    
    if (nsCRT::IsAsciiSpace(point.mChar) || point.mChar == nbsp) {
      return DeleteChars(point.mTextNode, point.mOffset,
                         point.mTextNode, point.mOffset + 1);
    }
  }

  
  
  if (nsCRT::IsAsciiSpace(point.mChar)) {
    nsRefPtr<Text> startNodeText, endNodeText;
    int32_t startOffset, endOffset;
    GetAsciiWSBounds(eBoth, point.mTextNode, point.mOffset + 1,
                     getter_AddRefs(startNodeText), &startOffset,
                     getter_AddRefs(endNodeText), &endOffset);

    
    nsCOMPtr<nsINode> startNode = startNodeText.get();
    nsCOMPtr<nsINode> endNode = endNodeText.get();
    nsresult res =
      nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
                                          address_of(startNode), &startOffset,
                                          address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);

    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  } else if (point.mChar == nbsp) {
    nsCOMPtr<nsINode> node(point.mTextNode);
    
    int32_t startOffset = point.mOffset;
    int32_t endOffset = point.mOffset + 1;
    nsresult res =
      nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
                                          address_of(node), &startOffset,
                                          address_of(node), &endOffset);
    NS_ENSURE_SUCCESS(res, res);

    
    return DeleteChars(node, startOffset, node, endOffset);
  }
  return NS_OK;
}

nsresult
nsWSRunObject::DeleteWSForward()
{
  WSPoint point = GetCharAfter(mNode, mOffset);
  NS_ENSURE_TRUE(point.mTextNode, NS_OK); 

  if (mPRE) {
    
    if (nsCRT::IsAsciiSpace(point.mChar) || point.mChar == nbsp) {
      return DeleteChars(point.mTextNode, point.mOffset,
                         point.mTextNode, point.mOffset + 1);
    }
  }

  
  
  if (nsCRT::IsAsciiSpace(point.mChar)) {
    nsRefPtr<Text> startNodeText, endNodeText;
    int32_t startOffset, endOffset;
    GetAsciiWSBounds(eBoth, point.mTextNode, point.mOffset + 1,
                     getter_AddRefs(startNodeText), &startOffset,
                     getter_AddRefs(endNodeText), &endOffset);

    
    nsCOMPtr<nsINode> startNode(startNodeText), endNode(endNodeText);
    nsresult res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
        address_of(startNode), &startOffset, address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);

    
    return DeleteChars(startNode, startOffset, endNode, endOffset);
  } else if (point.mChar == nbsp) {
    nsCOMPtr<nsINode> node(point.mTextNode);
    
    int32_t startOffset = point.mOffset;
    int32_t endOffset = point.mOffset+1;
    nsresult res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
        address_of(node), &startOffset, address_of(node), &endOffset);
    NS_ENSURE_SUCCESS(res, res);

    
    return DeleteChars(node, startOffset, node, endOffset);
  }
  return NS_OK;
}

void
nsWSRunObject::PriorVisibleNode(nsINode* aNode,
                                int32_t aOffset,
                                nsCOMPtr<nsINode>* outVisNode,
                                int32_t* outVisOffset,
                                WSType* outType)
{
  
  
  
  MOZ_ASSERT(aNode && outVisNode && outVisOffset && outType);

  WSFragment* run;
  FindRun(aNode, aOffset, &run, false);

  
  for (; run; run = run->mLeft) {
    if (run->mType == WSType::normalWS) {
      WSPoint point = GetCharBefore(aNode, aOffset);
      if (point.mTextNode) {
        *outVisNode = point.mTextNode;
        *outVisOffset = point.mOffset + 1;
        if (nsCRT::IsAsciiSpace(point.mChar) || point.mChar == nbsp) {
          *outType = WSType::normalWS;
        } else if (!point.mChar) {
          
          *outType = WSType::none;
        } else {
          *outType = WSType::text;
        }
        return;
      }
      
    }
  }

  
  *outVisNode = mStartReasonNode;
  
  *outVisOffset = mStartOffset;
  *outType = mStartReason;
}


void
nsWSRunObject::NextVisibleNode(nsINode* aNode,
                               int32_t aOffset,
                               nsCOMPtr<nsINode>* outVisNode,
                               int32_t* outVisOffset,
                               WSType* outType)
{
  
  
  
  MOZ_ASSERT(aNode && outVisNode && outVisOffset && outType);

  WSFragment* run;
  FindRun(aNode, aOffset, &run, true);

  
  for (; run; run = run->mRight) {
    if (run->mType == WSType::normalWS) {
      WSPoint point = GetCharAfter(aNode, aOffset);
      if (point.mTextNode) {
        *outVisNode = point.mTextNode;
        *outVisOffset = point.mOffset;
        if (nsCRT::IsAsciiSpace(point.mChar) || point.mChar == nbsp) {
          *outType = WSType::normalWS;
        } else if (!point.mChar) {
          
          *outType = WSType::none;
        } else {
          *outType = WSType::text;
        }
        return;
      }
      
    }
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






already_AddRefed<nsINode>
nsWSRunObject::GetWSBoundingParent()
{
  NS_ENSURE_TRUE(mNode, nullptr);
  OwningNonNull<nsINode> wsBoundingParent = *mNode;
  while (!IsBlockNode(wsBoundingParent)) {
    nsCOMPtr<nsINode> parent = wsBoundingParent->GetParentNode();
    if (!parent || !mHTMLEditor->IsEditable(parent)) {
      break;
    }
    wsBoundingParent = parent;
  }
  return wsBoundingParent.forget();
}

nsresult
nsWSRunObject::GetWSNodes()
{
  
  
  
  ::DOMPoint start(mNode, mOffset), end(mNode, mOffset);
  nsCOMPtr<nsINode> wsBoundingParent = GetWSBoundingParent();

  
  if (nsRefPtr<Text> textNode = mNode->GetAsText()) {
    const nsTextFragment* textFrag = textNode->GetText();
    
    mNodeArray.InsertElementAt(0, textNode);
    if (mOffset) {
      for (int32_t pos = mOffset - 1; pos >= 0; pos--) {
        
        if (uint32_t(pos) >= textFrag->GetLength()) {
          NS_NOTREACHED("looking beyond end of text fragment");
          continue;
        }
        char16_t theChar = textFrag->CharAt(pos);
        if (!nsCRT::IsAsciiSpace(theChar)) {
          if (theChar != nbsp) {
            mStartNode = textNode;
            mStartOffset = pos + 1;
            mStartReason = WSType::text;
            mStartReasonNode = textNode;
            break;
          }
          
          mFirstNBSPNode = textNode;
          mFirstNBSPOffset = pos;
          
          if (!mLastNBSPNode) {
            mLastNBSPNode = textNode;
            mLastNBSPOffset = pos;
          }
        }
        start.node = textNode;
        start.offset = pos;
      }
    }
  }

  while (!mStartNode) {
    
    nsCOMPtr<nsIContent> priorNode = GetPreviousWSNode(start, wsBoundingParent);
    if (priorNode) {
      if (IsBlockNode(priorNode)) {
        mStartNode = start.node;
        mStartOffset = start.offset;
        mStartReason = WSType::otherBlock;
        mStartReasonNode = priorNode;
      } else if (nsRefPtr<Text> textNode = priorNode->GetAsText()) {
        mNodeArray.InsertElementAt(0, textNode);
        const nsTextFragment *textFrag;
        if (!textNode || !(textFrag = textNode->GetText())) {
          return NS_ERROR_NULL_POINTER;
        }
        uint32_t len = textNode->TextLength();

        if (len < 1) {
          
          
          start.SetPoint(priorNode, 0);
        } else {
          for (int32_t pos = len - 1; pos >= 0; pos--) {
            
            if (uint32_t(pos) >= textFrag->GetLength()) {
              NS_NOTREACHED("looking beyond end of text fragment");
              continue;
            }
            char16_t theChar = textFrag->CharAt(pos);
            if (!nsCRT::IsAsciiSpace(theChar)) {
              if (theChar != nbsp) {
                mStartNode = textNode;
                mStartOffset = pos + 1;
                mStartReason = WSType::text;
                mStartReasonNode = textNode;
                break;
              }
              
              mFirstNBSPNode = textNode;
              mFirstNBSPOffset = pos;
              
              if (!mLastNBSPNode) {
                mLastNBSPNode = textNode;
                mLastNBSPOffset = pos;
              }
            }
            start.SetPoint(textNode, pos);
          }
        }
      } else {
        
        
        mStartNode = start.node;
        mStartOffset = start.offset;
        if (nsTextEditUtils::IsBreak(priorNode)) {
          mStartReason = WSType::br;
        } else {
          mStartReason = WSType::special;
        }
        mStartReasonNode = priorNode;
      }
    } else {
      
      mStartNode = start.node;
      mStartOffset = start.offset;
      mStartReason = WSType::thisBlock;
      mStartReasonNode = wsBoundingParent;
    } 
  }
  
  
  if (nsRefPtr<Text> textNode = mNode->GetAsText()) {
    
    const nsTextFragment *textFrag = textNode->GetText();

    uint32_t len = textNode->TextLength();
    if (uint16_t(mOffset)<len) {
      for (uint32_t pos = mOffset; pos < len; pos++) {
        
        if (pos >= textFrag->GetLength()) {
          NS_NOTREACHED("looking beyond end of text fragment");
          continue;
        }
        char16_t theChar = textFrag->CharAt(pos);
        if (!nsCRT::IsAsciiSpace(theChar)) {
          if (theChar != nbsp) {
            mEndNode = textNode;
            mEndOffset = pos;
            mEndReason = WSType::text;
            mEndReasonNode = textNode;
            break;
          }
          
          mLastNBSPNode = textNode;
          mLastNBSPOffset = pos;
          
          if (!mFirstNBSPNode) {
            mFirstNBSPNode = textNode;
            mFirstNBSPOffset = pos;
          }
        }
        end.SetPoint(textNode, pos + 1);
      }
    }
  }

  while (!mEndNode) {
    
    nsCOMPtr<nsIContent> nextNode = GetNextWSNode(end, wsBoundingParent);
    if (nextNode) {
      if (IsBlockNode(nextNode)) {
        
        mEndNode = end.node;
        mEndOffset = end.offset;
        mEndReason = WSType::otherBlock;
        mEndReasonNode = nextNode;
      } else if (nsRefPtr<Text> textNode = nextNode->GetAsText()) {
        mNodeArray.AppendElement(textNode);
        const nsTextFragment *textFrag;
        if (!textNode || !(textFrag = textNode->GetText())) {
          return NS_ERROR_NULL_POINTER;
        }
        uint32_t len = textNode->TextLength();

        if (len < 1) {
          
          
          end.SetPoint(textNode, 0);
        } else {
          for (uint32_t pos = 0; pos < len; pos++) {
            
            if (pos >= textFrag->GetLength()) {
              NS_NOTREACHED("looking beyond end of text fragment");
              continue;
            }
            char16_t theChar = textFrag->CharAt(pos);
            if (!nsCRT::IsAsciiSpace(theChar)) {
              if (theChar != nbsp) {
                mEndNode = textNode;
                mEndOffset = pos;
                mEndReason = WSType::text;
                mEndReasonNode = textNode;
                break;
              }
              
              mLastNBSPNode = textNode;
              mLastNBSPOffset = pos;
              
              if (!mFirstNBSPNode) {
                mFirstNBSPNode = textNode;
                mFirstNBSPOffset = pos;
              }
            }
            end.SetPoint(textNode, pos + 1);
          }
        }
      } else {
        
        
        
        mEndNode = end.node;
        mEndOffset = end.offset;
        if (nsTextEditUtils::IsBreak(nextNode)) {
          mEndReason = WSType::br;
        } else {
          mEndReason = WSType::special;
        }
        mEndReasonNode = nextNode;
      }
    } else {
      
      mEndNode = end.node;
      mEndOffset = end.offset;
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
  
  
  mHTMLEditor->IsPreformatted(GetAsDOMNode(mNode), &mPRE);
  
  
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

nsIContent*
nsWSRunObject::GetPreviousWSNodeInner(nsINode* aStartNode,
                                      nsINode* aBlockParent)
{
  
  
  
  MOZ_ASSERT(aStartNode && aBlockParent);

  nsCOMPtr<nsIContent> priorNode = aStartNode->GetPreviousSibling();
  OwningNonNull<nsINode> curNode = *aStartNode;
  while (!priorNode) {
    
    nsCOMPtr<nsINode> curParent = curNode->GetParentNode();
    NS_ENSURE_TRUE(curParent, nullptr);
    if (curParent == aBlockParent) {
      
      
      return nullptr;
    }
    
    priorNode = curParent->GetPreviousSibling();
    curNode = curParent;
  }
  
  if (IsBlockNode(priorNode)) {
    return priorNode;
  }
  if (mHTMLEditor->IsContainer(priorNode)) {
    
    nsCOMPtr<nsIContent> child = mHTMLEditor->GetRightmostChild(priorNode);
    if (child) {
      return child;
    }
  }
  
  return priorNode;
}

nsIContent*
nsWSRunObject::GetPreviousWSNode(::DOMPoint aPoint,
                                 nsINode* aBlockParent)
{
  
  
  
  MOZ_ASSERT(aPoint.node && aBlockParent);

  if (aPoint.node->NodeType() == nsIDOMNode::TEXT_NODE) {
    return GetPreviousWSNodeInner(aPoint.node, aBlockParent);
  }
  if (!mHTMLEditor->IsContainer(aPoint.node)) {
    return GetPreviousWSNodeInner(aPoint.node, aBlockParent);
  }

  if (!aPoint.offset) {
    if (aPoint.node == aBlockParent) {
      
      return nullptr;
    }

    
    return GetPreviousWSNodeInner(aPoint.node, aBlockParent);
  }

  nsCOMPtr<nsIContent> startContent = do_QueryInterface(aPoint.node);
  NS_ENSURE_TRUE(startContent, nullptr);
  nsCOMPtr<nsIContent> priorNode = startContent->GetChildAt(aPoint.offset - 1);
  NS_ENSURE_TRUE(priorNode, nullptr);
  
  if (IsBlockNode(priorNode)) {
    return priorNode;
  }
  if (mHTMLEditor->IsContainer(priorNode)) {
    
    nsCOMPtr<nsIContent> child = mHTMLEditor->GetRightmostChild(priorNode);
    if (child) {
      return child;
    }
  }
  
  return priorNode;
}

nsIContent*
nsWSRunObject::GetNextWSNodeInner(nsINode* aStartNode,
                                  nsINode* aBlockParent)
{
  
  
  
  MOZ_ASSERT(aStartNode && aBlockParent);

  nsCOMPtr<nsIContent> nextNode = aStartNode->GetNextSibling();
  nsCOMPtr<nsINode> curNode = aStartNode;
  while (!nextNode) {
    
    nsCOMPtr<nsINode> curParent = curNode->GetParentNode();
    NS_ENSURE_TRUE(curParent, nullptr);
    if (curParent == aBlockParent) {
      
      
      return nullptr;
    }
    
    nextNode = curParent->GetNextSibling();
    curNode = curParent;
  }
  
  if (IsBlockNode(nextNode)) {
    return nextNode;
  }
  if (mHTMLEditor->IsContainer(nextNode)) {
    
    nsCOMPtr<nsIContent> child = mHTMLEditor->GetLeftmostChild(nextNode);
    if (child) {
      return child;
    }
  }
  
  return nextNode;
}

nsIContent*
nsWSRunObject::GetNextWSNode(::DOMPoint aPoint, nsINode* aBlockParent)
{
  
  
  
  MOZ_ASSERT(aPoint.node && aBlockParent);

  if (aPoint.node->NodeType() == nsIDOMNode::TEXT_NODE) {
    return GetNextWSNodeInner(aPoint.node, aBlockParent);
  }
  if (!mHTMLEditor->IsContainer(aPoint.node)) {
    return GetNextWSNodeInner(aPoint.node, aBlockParent);
  }

  nsCOMPtr<nsIContent> startContent = do_QueryInterface(aPoint.node);
  NS_ENSURE_TRUE(startContent, nullptr);

  nsCOMPtr<nsIContent> nextNode = startContent->GetChildAt(aPoint.offset);
  if (!nextNode) {
    if (aPoint.node == aBlockParent) {
      
      return nullptr;
    }

    
    return GetNextWSNodeInner(aPoint.node, aBlockParent);
  }

  
  if (IsBlockNode(nextNode)) {
    return nextNode;
  }
  if (mHTMLEditor->IsContainer(nextNode)) {
    
    nsCOMPtr<nsIContent> child = mHTMLEditor->GetLeftmostChild(nextNode);
    if (child) {
      return child;
    }
  }
  
  return nextNode;
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
    res = aEndObject->DeleteChars(aEndObject->mNode, aEndObject->mOffset,
                                  afterRun->mEndNode, afterRun->mEndOffset,
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
    res = DeleteChars(beforeRun->mStartNode, beforeRun->mStartOffset,
                      mNode, mOffset, eOutsideUserSelectAll);
    NS_ENSURE_SUCCESS(res, res);
  } else if (beforeRun && beforeRun->mType == WSType::normalWS && !mPRE) {
    if ((afterRun && (afterRun->mType & WSType::trailingWS)) ||
        (afterRun && afterRun->mType == WSType::normalWS) ||
        (!afterRun && (aEndObject->mEndReason & WSType::block))) {
      
      WSPoint point = GetCharBefore(mNode, mOffset);
      if (point.mTextNode && nsCRT::IsAsciiSpace(point.mChar))
      {
        nsRefPtr<Text> wsStartNode, wsEndNode;
        int32_t wsStartOffset, wsEndOffset;
        GetAsciiWSBounds(eBoth, mNode, mOffset,
                         getter_AddRefs(wsStartNode), &wsStartOffset,
                         getter_AddRefs(wsEndNode), &wsEndOffset);
        point.mTextNode = wsStartNode;
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
      nsRefPtr<Text> wsStartNode, wsEndNode;
      int32_t wsStartOffset, wsEndOffset;
      GetAsciiWSBounds(eBoth, mNode, mOffset,
                       getter_AddRefs(wsStartNode), &wsStartOffset,
                       getter_AddRefs(wsEndNode), &wsEndOffset);
      point.mTextNode = wsStartNode;
      point.mOffset = wsStartOffset;
      res = ConvertToNBSP(point);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return res;
}

nsresult
nsWSRunObject::DeleteChars(nsINode* aStartNode, int32_t aStartOffset,
                           nsINode* aEndNode, int32_t aEndOffset,
                           AreaRestriction aAR)
{
  
  
  NS_ENSURE_TRUE(aStartNode && aEndNode, NS_ERROR_NULL_POINTER);

  if (aAR == eOutsideUserSelectAll) {
    nsCOMPtr<nsIDOMNode> san =
      mHTMLEditor->FindUserSelectAllNode(GetAsDOMNode(aStartNode));
    if (san) {
      return NS_OK;
    }

    if (aStartNode != aEndNode) {
      san = mHTMLEditor->FindUserSelectAllNode(GetAsDOMNode(aEndNode));
      if (san) {
        return NS_OK;
      }
    }
  }

  if (aStartNode == aEndNode && aStartOffset == aEndOffset) {
    
    return NS_OK;
  }

  int32_t idx = mNodeArray.IndexOf(aStartNode);
  if (idx == -1) {
    
    
    idx = 0;
  }

  if (aStartNode == aEndNode && aStartNode->GetAsText()) {
    return mHTMLEditor->DeleteText(*aStartNode->GetAsText(),
        static_cast<uint32_t>(aStartOffset),
        static_cast<uint32_t>(aEndOffset - aStartOffset));
  }

  nsresult res;
  nsRefPtr<nsRange> range;
  int32_t count = mNodeArray.Length();
  for (; idx < count; idx++) {
    nsRefPtr<Text> node = mNodeArray[idx];
    if (!node) {
      
      return NS_OK;
    }
    if (node == aStartNode) {
      uint32_t len = node->Length();
      if (uint32_t(aStartOffset) < len) {
        res = mHTMLEditor->DeleteText(*node,
                                      AssertedCast<uint32_t>(aStartOffset),
                                      len - aStartOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
    } else if (node == aEndNode) {
      if (aEndOffset) {
        res = mHTMLEditor->DeleteText(*node, 0,
                                      AssertedCast<uint32_t>(aEndOffset));
        NS_ENSURE_SUCCESS(res, res);
      }
      break;
    } else {
      if (!range) {
        range = new nsRange(aStartNode);
        res = range->Set(aStartNode, aStartOffset, aEndNode, aEndOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      bool nodeBefore, nodeAfter;
      res = nsRange::CompareNodeToRange(node, range, &nodeBefore, &nodeAfter);
      NS_ENSURE_SUCCESS(res, res);
      if (nodeAfter) {
        break;
      }
      if (!nodeBefore) {
        res = mHTMLEditor->DeleteNode(node);
        NS_ENSURE_SUCCESS(res, res);
        mNodeArray.RemoveElement(node);
        --count;
        --idx;
      }
    }
  }
  return NS_OK;
}

nsWSRunObject::WSPoint
nsWSRunObject::GetCharAfter(nsINode* aNode, int32_t aOffset)
{
  MOZ_ASSERT(aNode);

  int32_t idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) {
    
    return GetWSPointAfter(aNode, aOffset);
  } else {
    
    return GetCharAfter(WSPoint(mNodeArray[idx], aOffset, 0));
  }
}

nsWSRunObject::WSPoint
nsWSRunObject::GetCharBefore(nsINode* aNode, int32_t aOffset)
{
  MOZ_ASSERT(aNode);

  int32_t idx = mNodeArray.IndexOf(aNode);
  if (idx == -1) {
    
    return GetWSPointBefore(aNode, aOffset);
  } else {
    
    return GetCharBefore(WSPoint(mNodeArray[idx], aOffset, 0));
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

  int32_t idx = mNodeArray.IndexOf(aPoint.mTextNode);
  if (idx == -1) {
    
    return outPoint;
  }
  int32_t numNodes = mNodeArray.Length();

  if (uint16_t(aPoint.mOffset) < aPoint.mTextNode->TextLength()) {
    outPoint = aPoint;
    outPoint.mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset);
    return outPoint;
  } else if (idx + 1 < numNodes) {
    outPoint.mTextNode = mNodeArray[idx + 1];
    MOZ_ASSERT(outPoint.mTextNode);
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

  int32_t idx = mNodeArray.IndexOf(aPoint.mTextNode);
  if (idx == -1) {
    
    return outPoint;
  }

  if (aPoint.mOffset != 0) {
    outPoint = aPoint;
    outPoint.mOffset--;
    outPoint.mChar = GetCharAt(aPoint.mTextNode, aPoint.mOffset - 1);
    return outPoint;
  } else if (idx) {
    outPoint.mTextNode = mNodeArray[idx - 1];

    uint32_t len = outPoint.mTextNode->TextLength();
    if (len) {
      outPoint.mOffset = len - 1;
      outPoint.mChar = GetCharAt(outPoint.mTextNode, len - 1);
    }
  }
  return outPoint;
}

nsresult
nsWSRunObject::ConvertToNBSP(WSPoint aPoint, AreaRestriction aAR)
{
  
  
  NS_ENSURE_TRUE(aPoint.mTextNode, NS_ERROR_NULL_POINTER);

  if (aAR == eOutsideUserSelectAll) {
    nsCOMPtr<nsIDOMNode> san =
      mHTMLEditor->FindUserSelectAllNode(GetAsDOMNode(aPoint.mTextNode));
    if (san) {
      return NS_OK;
    }
  }

  
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  nsAutoString nbspStr(nbsp);
  nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr,
      *aPoint.mTextNode, aPoint.mOffset, true);
  NS_ENSURE_SUCCESS(res, res);

  
  nsRefPtr<Text> startNode, endNode;
  int32_t startOffset = 0, endOffset = 0;

  GetAsciiWSBounds(eAfter, aPoint.mTextNode, aPoint.mOffset + 1,
                   getter_AddRefs(startNode), &startOffset,
                   getter_AddRefs(endNode), &endOffset);

  
  if (startNode) {
    res = DeleteChars(startNode, startOffset, endNode, endOffset);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}

void
nsWSRunObject::GetAsciiWSBounds(int16_t aDir, nsINode* aNode, int32_t aOffset,
                                Text** outStartNode, int32_t* outStartOffset,
                                Text** outEndNode, int32_t* outEndOffset)
{
  MOZ_ASSERT(aNode && outStartNode && outStartOffset && outEndNode &&
             outEndOffset);

  nsRefPtr<Text> startNode, endNode;
  int32_t startOffset = 0, endOffset = 0;

  if (aDir & eAfter) {
    WSPoint point = GetCharAfter(aNode, aOffset);
    if (point.mTextNode) {
      
      startNode = endNode = point.mTextNode;
      startOffset = endOffset = point.mOffset;

      
      for (; nsCRT::IsAsciiSpace(point.mChar) && point.mTextNode;
           point = GetCharAfter(point)) {
        endNode = point.mTextNode;
        
        point.mOffset++;
        endOffset = point.mOffset;
      }
    }
  }

  if (aDir & eBefore) {
    WSPoint point = GetCharBefore(aNode, aOffset);
    if (point.mTextNode) {
      
      startNode = point.mTextNode;
      startOffset = point.mOffset + 1;
      if (!endNode) {
        endNode = startNode;
        endOffset = startOffset;
      }

      
      for (; nsCRT::IsAsciiSpace(point.mChar) && point.mTextNode;
           point = GetCharBefore(point)) {
        startNode = point.mTextNode;
        startOffset = point.mOffset;
      }
    }
  }

  startNode.forget(outStartNode);
  *outStartOffset = startOffset;
  endNode.forget(outEndNode);
  *outEndOffset = endOffset;
}





void
nsWSRunObject::FindRun(nsINode* aNode, int32_t aOffset, WSFragment** outRun,
                       bool after)
{
  MOZ_ASSERT(aNode && outRun);
  *outRun = nullptr;

  for (WSFragment* run = mStartRun; run; run = run->mRight) {
    int32_t comp = run->mStartNode ? nsContentUtils::ComparePoints(aNode,
        aOffset, run->mStartNode, run->mStartOffset) : -1;
    if (comp <= 0) {
      if (after) {
        *outRun = run;
      } else {
        
        *outRun = nullptr;
      }
      return;
    }
    comp = run->mEndNode ? nsContentUtils::ComparePoints(aNode, aOffset,
        run->mEndNode, run->mEndOffset) : -1;
    if (comp < 0) {
      *outRun = run;
      return;
    } else if (comp == 0) {
      if (after) {
        *outRun = run->mRight;
      } else {
        
        *outRun = run;
      }
      return;
    }
    if (!run->mRight) {
      if (after) {
        *outRun = nullptr;
      } else {
        
        *outRun = run;
      }
      return;
    }
  }
}

char16_t 
nsWSRunObject::GetCharAt(Text* aTextNode, int32_t aOffset)
{
  
  NS_ENSURE_TRUE(aTextNode, 0);

  int32_t len = int32_t(aTextNode->TextLength());
  if (aOffset < 0 || aOffset >= len)
    return 0;
    
  return aTextNode->GetText()->CharAt(aOffset);
}

nsWSRunObject::WSPoint
nsWSRunObject::GetWSPointAfter(nsINode* aNode, int32_t aOffset)
{
  

  
  uint32_t numNodes = mNodeArray.Length();

  if (!numNodes) {
    
    WSPoint outPoint;
    return outPoint;
  }

  uint32_t firstNum = 0, curNum = numNodes/2, lastNum = numNodes;
  int16_t cmp = 0;
  nsRefPtr<Text> curNode;

  
  
  while (curNum != lastNum) {
    curNode = mNodeArray[curNum];
    cmp = nsContentUtils::ComparePoints(aNode, aOffset, curNode, 0);
    if (cmp < 0) {
      lastNum = curNum;
    } else {
      firstNum = curNum + 1;
    }
    curNum = (lastNum - firstNum)/2 + firstNum;
    MOZ_ASSERT(firstNum <= curNum && curNum <= lastNum, "Bad binary search");
  }

  
  
  
  if (curNum == mNodeArray.Length()) {
    
    
    nsRefPtr<Text> textNode(mNodeArray[curNum - 1]);
    WSPoint point(textNode, textNode->TextLength(), 0);
    return GetCharAfter(point);
  } else {
    
    nsRefPtr<Text> textNode(mNodeArray[curNum]);
    WSPoint point(textNode, 0, 0);
    return GetCharAfter(point);
  }
}

nsWSRunObject::WSPoint
nsWSRunObject::GetWSPointBefore(nsINode* aNode, int32_t aOffset)
{
  

  
  uint32_t numNodes = mNodeArray.Length();

  if (!numNodes) {
    
    WSPoint outPoint;
    return outPoint;
  }

  uint32_t firstNum = 0, curNum = numNodes/2, lastNum = numNodes;
  int16_t cmp = 0;
  nsRefPtr<Text>  curNode;

  
  
  while (curNum != lastNum) {
    curNode = mNodeArray[curNum];
    cmp = nsContentUtils::ComparePoints(aNode, aOffset, curNode, 0);
    if (cmp < 0) {
      lastNum = curNum;
    } else {
      firstNum = curNum + 1;
    }
    curNum = (lastNum - firstNum)/2 + firstNum;
    MOZ_ASSERT(firstNum <= curNum && curNum <= lastNum, "Bad binary search");
  }

  
  
  
  if (curNum == mNodeArray.Length()) {
    
    
    nsRefPtr<Text> textNode(mNodeArray[curNum - 1]);
    WSPoint point(textNode, textNode->TextLength(), 0);
    return GetCharBefore(point);
  } else {
    
    
    
    nsRefPtr<Text> textNode(mNodeArray[curNum]);
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
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) {
        leftCheck = true;
      } else {
        spaceNBSP = true;
      }
    } else if (aRun->mLeftType == WSType::text ||
               aRun->mLeftType == WSType::special) {
      leftCheck = true;
    }
    if (leftCheck || spaceNBSP) {
      
      
      if (aRun->mRightType == WSType::text ||
          aRun->mRightType == WSType::special ||
          aRun->mRightType == WSType::br) {
        rightCheck = true;
      }
      if ((aRun->mRightType & WSType::block) &&
          IsBlockNode(nsCOMPtr<nsINode>(GetWSBoundingParent()))) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<Element> brNode =
          mHTMLEditor->CreateBR(aRun->mEndNode, aRun->mEndOffset);
        NS_ENSURE_TRUE(brNode, NS_ERROR_FAILURE);

        
        thePoint = GetCharBefore(aRun->mEndNode, aRun->mEndOffset);
        prevPoint = GetCharBefore(thePoint);
        rightCheck = true;
      }
    }
    if (leftCheck && rightCheck) {
      
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString spaceStr(char16_t(32));
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr,
                                                    *thePoint.mTextNode,
                                                    thePoint.mOffset, true);
      NS_ENSURE_SUCCESS(res, res);

      
      res = DeleteChars(thePoint.mTextNode, thePoint.mOffset + 1,
                        thePoint.mTextNode, thePoint.mOffset + 2);
      NS_ENSURE_SUCCESS(res, res);
    } else if (!mPRE && spaceNBSP && rightCheck) {
      
      
      
      
      
      

      nsRefPtr<Text> startNode, endNode;
      int32_t startOffset, endOffset;
      GetAsciiWSBounds(eBoth, prevPoint.mTextNode, prevPoint.mOffset + 1,
                       getter_AddRefs(startNode), &startOffset,
                       getter_AddRefs(endNode), &endOffset);

      
      res = DeleteChars(thePoint.mTextNode, thePoint.mOffset,
                        thePoint.mTextNode, thePoint.mOffset + 1);
      NS_ENSURE_SUCCESS(res, res);

      
      nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
      nsAutoString nbspStr(nbsp);
      res = mHTMLEditor->InsertTextIntoTextNodeImpl(nbspStr, *startNode,
                                                    startOffset, true);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckTrailingNBSP(WSFragment* aRun, nsINode* aNode,
                                 int32_t aOffset)
{
  
  
  
  
  
  NS_ENSURE_TRUE(aRun && aNode, NS_ERROR_NULL_POINTER);
  bool canConvert = false;
  WSPoint thePoint = GetCharBefore(aNode, aOffset);
  if (thePoint.mTextNode && thePoint.mChar == nbsp) {
    WSPoint prevPoint = GetCharBefore(thePoint);
    if (prevPoint.mTextNode) {
      if (!nsCRT::IsAsciiSpace(prevPoint.mChar)) {
        canConvert = true;
      }
    } else if (aRun->mLeftType == WSType::text ||
               aRun->mLeftType == WSType::special) {
      canConvert = true;
    }
  }
  if (canConvert) {
    
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(char16_t(32));
    nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr,
        *thePoint.mTextNode, thePoint.mOffset, true);
    NS_ENSURE_SUCCESS(res, res);

    
    res = DeleteChars(thePoint.mTextNode, thePoint.mOffset + 1,
                      thePoint.mTextNode, thePoint.mOffset + 2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

nsresult
nsWSRunObject::CheckLeadingNBSP(WSFragment* aRun, nsINode* aNode,
                                int32_t aOffset)
{
  
  
  
  
  bool canConvert = false;
  WSPoint thePoint = GetCharAfter(aNode, aOffset);
  if (thePoint.mChar == nbsp) {
    WSPoint tmp = thePoint;
    
    tmp.mOffset++;
    WSPoint nextPoint = GetCharAfter(tmp);
    if (nextPoint.mTextNode) {
      if (!nsCRT::IsAsciiSpace(nextPoint.mChar)) {
        canConvert = true;
      }
    } else if (aRun->mRightType == WSType::text ||
               aRun->mRightType == WSType::special ||
               aRun->mRightType == WSType::br) {
      canConvert = true;
    }
  }
  if (canConvert) {
    
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString spaceStr(char16_t(32));
    nsresult res = mHTMLEditor->InsertTextIntoTextNodeImpl(spaceStr,
        *thePoint.mTextNode, thePoint.mOffset, true);
    NS_ENSURE_SUCCESS(res, res);

    
    res = DeleteChars(thePoint.mTextNode, thePoint.mOffset + 1,
                      thePoint.mTextNode, thePoint.mOffset + 2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}


nsresult
nsWSRunObject::Scrub()
{
  WSFragment *run = mStartRun;
  while (run)
  {
    if (run->mType & (WSType::leadingWS | WSType::trailingWS)) {
      nsresult res = DeleteChars(run->mStartNode, run->mStartOffset,
                                 run->mEndNode, run->mEndOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    run = run->mRight;
  }
  return NS_OK;
}
