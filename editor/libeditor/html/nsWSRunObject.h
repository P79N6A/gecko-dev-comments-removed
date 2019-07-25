




#ifndef __wsrunobject_h__
#define __wsrunobject_h__

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nsINode.h"
#include "nscore.h"
#include "prtypes.h"

class nsHTMLEditor;
class nsIDOMDocument;
class nsIDOMNode;
struct DOMPoint;


























class WSType {
public:
  enum Enum {
    none       = 0,
    leadingWS  = 1,      
    trailingWS = 1 << 1, 
    normalWS   = 1 << 2, 
    text       = 1 << 3, 
    special    = 1 << 4, 
    br         = 1 << 5, 
    otherBlock = 1 << 6, 
    thisBlock  = 1 << 7, 
    block      = otherBlock | thisBlock 
  };

  




  WSType(const Enum& aEnum = none) : mEnum(aEnum) {}
  
  friend bool operator==(const WSType& aLeft, const WSType& aRight);
  friend const WSType operator&(const WSType& aLeft, const WSType& aRight);
  friend const WSType operator|(const WSType& aLeft, const WSType& aRight);
  WSType& operator=(const WSType& aOther) {
    
    mEnum = aOther.mEnum;
    return *this;
  }
  WSType& operator&=(const WSType& aOther) {
    mEnum &= aOther.mEnum;
    return *this;
  }
  WSType& operator|=(const WSType& aOther) {
    mEnum |= aOther.mEnum;
    return *this;
  }
private:
  PRUint16 mEnum;
  void bool_conversion_helper() {};
public:
  
  typedef void (WSType::*bool_type)();
  operator bool_type() const
  {
    return mEnum ? &WSType::bool_conversion_helper : nullptr;
  }
};





inline bool operator==(const WSType& aLeft, const WSType& aRight)
{
  return aLeft.mEnum == aRight.mEnum;
}
inline bool operator!=(const WSType& aLeft, const WSType& aRight)
{
  return !(aLeft == aRight);
}
inline const WSType operator&(const WSType& aLeft, const WSType& aRight)
{
  WSType ret;
  ret.mEnum = aLeft.mEnum & aRight.mEnum;
  return ret;
}
inline const WSType operator|(const WSType& aLeft, const WSType& aRight)
{
  WSType ret;
  ret.mEnum = aLeft.mEnum | aRight.mEnum;
  return ret;
}





inline const WSType operator&(const WSType::Enum& aLeft,
                              const WSType::Enum& aRight)
{
  return WSType(aLeft) & WSType(aRight);
}
inline const WSType operator|(const WSType::Enum& aLeft,
                              const WSType::Enum& aRight)
{
  return WSType(aLeft) | WSType(aRight);
}


class NS_STACK_CLASS nsWSRunObject
{
  public:

    
    enum BlockBoundary
    {
      kBeforeBlock,
      kBlockStart,
      kBlockEnd,
      kAfterBlock
    };

    enum {eBefore = 1};
    enum {eAfter  = 1 << 1};
    enum {eBoth   = eBefore | eAfter};

    
    nsWSRunObject(nsHTMLEditor *aEd, nsIDOMNode *aNode, PRInt32 aOffset);
    ~nsWSRunObject();
    
    

    
    
    static nsresult ScrubBlockBoundary(nsHTMLEditor *aHTMLEd, 
                                       nsCOMPtr<nsIDOMNode> *aBlock,
                                       BlockBoundary aBoundary,
                                       PRInt32 *aOffset = 0);
    
    
    
    
    static nsresult PrepareToJoinBlocks(nsHTMLEditor *aEd, 
                                        nsIDOMNode *aLeftParent,
                                        nsIDOMNode *aRightParent);

    
    
    
    
    
    
    
    static nsresult PrepareToDeleteRange(nsHTMLEditor *aHTMLEd, 
                                         nsCOMPtr<nsIDOMNode> *aStartNode,
                                         PRInt32 *aStartOffset, 
                                         nsCOMPtr<nsIDOMNode> *aEndNode,
                                         PRInt32 *aEndOffset);

    
    
    
    static nsresult PrepareToDeleteNode(nsHTMLEditor *aHTMLEd, 
                                        nsIDOMNode *aNode);

    
    
    
    
    
    
    
    static nsresult PrepareToSplitAcrossBlocks(nsHTMLEditor *aHTMLEd, 
                                               nsCOMPtr<nsIDOMNode> *aSplitNode, 
                                               PRInt32 *aSplitOffset);

    
    
    
    
    nsresult InsertBreak(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                         PRInt32 *aInOutOffset, 
                         nsCOMPtr<nsIDOMNode> *outBRNode, 
                         nsIEditor::EDirection aSelect);

    
    
    
    
    nsresult InsertText(const nsAString& aStringToInsert, 
                        nsCOMPtr<nsIDOMNode> *aInOutNode, 
                        PRInt32 *aInOutOffset,
                        nsIDOMDocument *aDoc);

    
    
    
    
    nsresult DeleteWSBackward();

    
    
    
    
    nsresult DeleteWSForward();

    
    
    
    
    
    void PriorVisibleNode(nsIDOMNode *aNode,
                          PRInt32 aOffset,
                          nsCOMPtr<nsIDOMNode> *outVisNode,
                          PRInt32 *outVisOffset,
                          WSType *outType);

    
    
    
    
    
    void NextVisibleNode(nsIDOMNode *aNode,
                         PRInt32 aOffset,
                         nsCOMPtr<nsIDOMNode> *outVisNode,
                         PRInt32 *outVisOffset,
                         WSType *outType);
    
    
    
    nsresult AdjustWhitespace();

  protected:
    
    
    
    
    
    struct WSFragment
    {
      nsCOMPtr<nsIDOMNode> mStartNode;  
      nsCOMPtr<nsIDOMNode> mEndNode;    
      PRInt32 mStartOffset;             
      PRInt32 mEndOffset;               
      
      WSType mType, mLeftType, mRightType;
      
      WSFragment *mLeft, *mRight;

      WSFragment() : mStartNode(0), mEndNode(0),
                     mStartOffset(0), mEndOffset(0),
                     mType(), mLeftType(), mRightType(),
                     mLeft(0), mRight(0)
      {
      }
    };
    
    
    
    
    
    
    struct NS_STACK_CLASS WSPoint
    {
      nsCOMPtr<nsIContent> mTextNode;
      PRUint32 mOffset;
      PRUnichar mChar;

      WSPoint() : mTextNode(0),mOffset(0),mChar(0) {}
      WSPoint(nsIDOMNode *aNode, PRInt32 aOffset, PRUnichar aChar) : 
                     mTextNode(do_QueryInterface(aNode)),mOffset(aOffset),mChar(aChar)
      {
        if (!mTextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
          
          
          mTextNode = nullptr;
        }
      }
      WSPoint(nsIContent *aTextNode, PRInt32 aOffset, PRUnichar aChar) : 
                     mTextNode(aTextNode),mOffset(aOffset),mChar(aChar) {}
    };    

    enum AreaRestriction
    {
      eAnywhere, eOutsideUserSelectAll
    };    
    
    
    

    




    already_AddRefed<nsIDOMNode> GetWSBoundingParent();

    nsresult GetWSNodes();
    void     GetRuns();
    void     ClearRuns();
    void     MakeSingleWSRun(WSType aType);
    nsresult PrependNodeToList(nsIDOMNode *aNode);
    nsresult AppendNodeToList(nsIDOMNode *aNode);
    nsresult GetPreviousWSNode(nsIDOMNode *aStartNode, 
                               nsIDOMNode *aBlockParent, 
                               nsCOMPtr<nsIDOMNode> *aPriorNode);
    nsresult GetPreviousWSNode(nsIDOMNode *aStartNode,
                               PRInt32      aOffset,
                               nsIDOMNode  *aBlockParent, 
                               nsCOMPtr<nsIDOMNode> *aPriorNode);
    nsresult GetPreviousWSNode(DOMPoint aPoint,
                               nsIDOMNode  *aBlockParent, 
                               nsCOMPtr<nsIDOMNode> *aPriorNode);
    nsresult GetNextWSNode(nsIDOMNode *aStartNode, 
                           nsIDOMNode *aBlockParent, 
                           nsCOMPtr<nsIDOMNode> *aNextNode);
    nsresult GetNextWSNode(nsIDOMNode *aStartNode,
                           PRInt32     aOffset,
                           nsIDOMNode *aBlockParent, 
                           nsCOMPtr<nsIDOMNode> *aNextNode);
    nsresult GetNextWSNode(DOMPoint aPoint,
                           nsIDOMNode  *aBlockParent, 
                           nsCOMPtr<nsIDOMNode> *aNextNode);
    nsresult PrepareToDeleteRangePriv(nsWSRunObject* aEndObject);
    nsresult PrepareToSplitAcrossBlocksPriv();
    nsresult DeleteChars(nsIDOMNode *aStartNode, PRInt32 aStartOffset, 
                         nsIDOMNode *aEndNode, PRInt32 aEndOffset,
                         AreaRestriction aAR = eAnywhere);
    WSPoint  GetCharAfter(nsIDOMNode *aNode, PRInt32 aOffset);
    WSPoint  GetCharBefore(nsIDOMNode *aNode, PRInt32 aOffset);
    WSPoint  GetCharAfter(const WSPoint &aPoint);
    WSPoint  GetCharBefore(const WSPoint &aPoint);
    nsresult ConvertToNBSP(WSPoint aPoint,
                           AreaRestriction aAR = eAnywhere);
    void     GetAsciiWSBounds(PRInt16 aDir, nsIDOMNode *aNode, PRInt32 aOffset,
                                nsCOMPtr<nsIDOMNode> *outStartNode, PRInt32 *outStartOffset,
                                nsCOMPtr<nsIDOMNode> *outEndNode, PRInt32 *outEndOffset);
    void     FindRun(nsIDOMNode *aNode, PRInt32 aOffset, WSFragment **outRun, bool after);
    PRUnichar GetCharAt(nsIContent *aTextNode, PRInt32 aOffset);
    WSPoint  GetWSPointAfter(nsIDOMNode *aNode, PRInt32 aOffset);
    WSPoint  GetWSPointBefore(nsIDOMNode *aNode, PRInt32 aOffset);
    nsresult CheckTrailingNBSPOfRun(WSFragment *aRun);
    nsresult CheckTrailingNBSP(WSFragment *aRun, nsIDOMNode *aNode, PRInt32 aOffset);
    nsresult CheckLeadingNBSP(WSFragment *aRun, nsIDOMNode *aNode, PRInt32 aOffset);
    
    static nsresult ScrubBlockBoundaryInner(nsHTMLEditor *aHTMLEd, 
                                       nsCOMPtr<nsIDOMNode> *aBlock,
                                       BlockBoundary aBoundary);
    nsresult Scrub();
    
    
    
    nsCOMPtr<nsIDOMNode> mNode;           
    PRInt32 mOffset;                      
    
    
    bool    mPRE;                         
    nsCOMPtr<nsIDOMNode> mStartNode;      
    PRInt32 mStartOffset;                 
    WSType mStartReason;                  
    nsCOMPtr<nsIDOMNode> mStartReasonNode;
    
    nsCOMPtr<nsIDOMNode> mEndNode;        
    PRInt32 mEndOffset;                   
    WSType mEndReason;                    
    nsCOMPtr<nsIDOMNode> mEndReasonNode;  
    
    nsCOMPtr<nsIDOMNode> mFirstNBSPNode;  
    PRInt32 mFirstNBSPOffset;             
    
    nsCOMPtr<nsIDOMNode> mLastNBSPNode;   
    PRInt32 mLastNBSPOffset;              
    
    nsCOMArray<nsIDOMNode> mNodeArray;
    
    WSFragment *mStartRun;                
    WSFragment *mEndRun;                  
    
    nsHTMLEditor *mHTMLEditor;            
    
    friend class nsHTMLEditRules;  
    friend class nsHTMLEditor;     
};

#endif

