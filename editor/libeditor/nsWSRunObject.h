




#ifndef __wsrunobject_h__
#define __wsrunobject_h__

#include "nsCOMPtr.h"
#include "nsIEditor.h" 
#include "nsINode.h"
#include "nscore.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Text.h"

class nsHTMLEditor;
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

  




  MOZ_IMPLICIT WSType(const Enum& aEnum = none) : mEnum(aEnum) {}
  
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
  uint16_t mEnum;
  void bool_conversion_helper() {}
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


class MOZ_STACK_CLASS nsWSRunObject
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

    
    nsWSRunObject(nsHTMLEditor* aEd, nsINode* aNode, int32_t aOffset);
    nsWSRunObject(nsHTMLEditor *aEd, nsIDOMNode *aNode, int32_t aOffset);
    ~nsWSRunObject();
    
    

    
    
    static nsresult ScrubBlockBoundary(nsHTMLEditor* aHTMLEd,
                                       BlockBoundary aBoundary,
                                       nsINode* aBlock,
                                       int32_t aOffset = -1);
 
    
    
    
    static nsresult PrepareToJoinBlocks(nsHTMLEditor* aEd,
                                        mozilla::dom::Element* aLeftBlock,
                                        mozilla::dom::Element* aRightBlock);

    
    
    
    
    
    
    
    static nsresult PrepareToDeleteRange(nsHTMLEditor* aHTMLEd,
                                         nsCOMPtr<nsINode>* aStartNode,
                                         int32_t* aStartOffset,
                                         nsCOMPtr<nsINode>* aEndNode,
                                         int32_t* aEndOffset);

    
    
    
    static nsresult PrepareToDeleteNode(nsHTMLEditor *aHTMLEd,
                                        nsIContent* aContent);

    
    
    
    
    
    static nsresult PrepareToSplitAcrossBlocks(nsHTMLEditor* aHTMLEd,
                                               nsCOMPtr<nsINode>* aSplitNode,
                                               int32_t* aSplitOffset);

    
    
    
    
    already_AddRefed<mozilla::dom::Element>
      InsertBreak(nsCOMPtr<nsINode>* aInOutParent, int32_t* aInOutOffset,
                  nsIEditor::EDirection aSelect);

    
    
    
    nsresult InsertText(const nsAString& aStringToInsert,
                        nsCOMPtr<nsINode>* aInOutNode,
                        int32_t* aInOutOffset,
                        nsIDocument* aDoc);

    
    
    
    
    nsresult DeleteWSBackward();

    
    
    
    nsresult DeleteWSForward();

    
    
    
    
    void PriorVisibleNode(nsINode* aNode,
                          int32_t aOffset,
                          nsCOMPtr<nsINode>* outVisNode,
                          int32_t* outVisOffset,
                          WSType* outType);

    
    
    
    
    void NextVisibleNode(nsINode* aNode,
                         int32_t aOffset,
                         nsCOMPtr<nsINode>* outVisNode,
                         int32_t* outVisOffset,
                         WSType* outType);

    
    
    nsresult AdjustWhitespace();

  protected:
    
    
    
    
    
    struct WSFragment
    {
      nsCOMPtr<nsINode> mStartNode;  
      nsCOMPtr<nsINode> mEndNode;    
      int32_t mStartOffset;          
      int32_t mEndOffset;            
      
      WSType mType, mLeftType, mRightType;
      
      WSFragment *mLeft, *mRight;

      WSFragment() : mStartNode(0), mEndNode(0),
                     mStartOffset(0), mEndOffset(0),
                     mType(), mLeftType(), mRightType(),
                     mLeft(0), mRight(0)
      {
      }
    };
    
    
    
    
    
    
    struct MOZ_STACK_CLASS WSPoint
    {
      nsRefPtr<mozilla::dom::Text> mTextNode;
      uint32_t mOffset;
      char16_t mChar;

      WSPoint() : mTextNode(0),mOffset(0),mChar(0) {}
      WSPoint(mozilla::dom::Text* aTextNode, int32_t aOffset, char16_t aChar) :
                     mTextNode(aTextNode),mOffset(aOffset),mChar(aChar) {}
    };    

    enum AreaRestriction
    {
      eAnywhere, eOutsideUserSelectAll
    };    
    
    
    

    




    already_AddRefed<nsINode> GetWSBoundingParent();

    nsresult GetWSNodes();
    void     GetRuns();
    void     ClearRuns();
    void     MakeSingleWSRun(WSType aType);
    nsIContent* GetPreviousWSNodeInner(nsINode* aStartNode,
                                       nsINode* aBlockParent);
    nsIContent* GetPreviousWSNode(::DOMPoint aPoint, nsINode* aBlockParent);
    nsIContent* GetNextWSNodeInner(nsINode* aStartNode, nsINode* aBlockParent);
    nsIContent* GetNextWSNode(::DOMPoint aPoint, nsINode* aBlockParent);
    nsresult PrepareToDeleteRangePriv(nsWSRunObject* aEndObject);
    nsresult PrepareToSplitAcrossBlocksPriv();
    nsresult DeleteChars(nsINode* aStartNode, int32_t aStartOffset,
                         nsINode* aEndNode, int32_t aEndOffset,
                         AreaRestriction aAR = eAnywhere);
    WSPoint  GetCharAfter(nsINode* aNode, int32_t aOffset);
    WSPoint  GetCharBefore(nsINode* aNode, int32_t aOffset);
    WSPoint  GetCharAfter(const WSPoint& aPoint);
    WSPoint  GetCharBefore(const WSPoint& aPoint);
    nsresult ConvertToNBSP(WSPoint aPoint,
                           AreaRestriction aAR = eAnywhere);
    void     GetAsciiWSBounds(int16_t aDir, nsINode* aNode, int32_t aOffset,
                              mozilla::dom::Text** outStartNode,
                              int32_t* outStartOffset,
                              mozilla::dom::Text** outEndNode,
                              int32_t* outEndOffset);
    void     FindRun(nsINode* aNode, int32_t aOffset, WSFragment** outRun,
                     bool after);
    char16_t GetCharAt(mozilla::dom::Text* aTextNode, int32_t aOffset);
    WSPoint  GetWSPointAfter(nsINode* aNode, int32_t aOffset);
    WSPoint  GetWSPointBefore(nsINode* aNode, int32_t aOffset);
    nsresult CheckTrailingNBSPOfRun(WSFragment *aRun);
    nsresult CheckTrailingNBSP(WSFragment* aRun, nsINode* aNode,
                               int32_t aOffset);
    nsresult CheckLeadingNBSP(WSFragment* aRun, nsINode* aNode,
                              int32_t aOffset);
    
    nsresult Scrub();
    
    
    
    nsCOMPtr<nsINode> mNode;           
    int32_t mOffset;                   
    
    
    bool    mPRE;                      
    nsCOMPtr<nsINode> mStartNode;      
    int32_t mStartOffset;              
    WSType mStartReason;               
    nsCOMPtr<nsINode> mStartReasonNode;
    
    nsCOMPtr<nsINode> mEndNode;        
    int32_t mEndOffset;                
    WSType mEndReason;                 
    nsCOMPtr<nsINode> mEndReasonNode;  
    
    nsRefPtr<mozilla::dom::Text> mFirstNBSPNode; 
    int32_t mFirstNBSPOffset;          
    
    nsRefPtr<mozilla::dom::Text> mLastNBSPNode; 
    int32_t mLastNBSPOffset;           
    
    
    nsTArray<nsRefPtr<mozilla::dom::Text>> mNodeArray;
    
    WSFragment *mStartRun;             
    WSFragment *mEndRun;               
    
    nsHTMLEditor *mHTMLEditor;         
    
    friend class nsHTMLEditRules;  
    friend class nsHTMLEditor;     
};

#endif

