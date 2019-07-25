




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
                          PRInt16 *outType);

    
    
    
    
    
    void NextVisibleNode(nsIDOMNode *aNode,
                         PRInt32 aOffset,
                         nsCOMPtr<nsIDOMNode> *outVisNode,
                         PRInt32 *outVisOffset,
                         PRInt16 *outType);
    
    
    
    nsresult AdjustWhitespace();
    
    
    enum {eNone = 0};
    enum {eLeadingWS  = 1};          
    enum {eTrailingWS = 1 << 1};     
    enum {eNormalWS   = 1 << 2};     
    enum {eText       = 1 << 3};     
    enum {eSpecial    = 1 << 4};     
    enum {eBreak      = 1 << 5};     
    enum {eOtherBlock = 1 << 6};     
    enum {eThisBlock  = 1 << 7};     
    enum {eBlock      = eOtherBlock | eThisBlock};   
    
    enum {eBefore = 1};
    enum {eAfter  = 1 << 1};
    enum {eBoth   = eBefore | eAfter};
    
  protected:
    
    
    
    
    
    struct WSFragment
    {
      nsCOMPtr<nsIDOMNode> mStartNode;  
      nsCOMPtr<nsIDOMNode> mEndNode;    
      PRInt32 mStartOffset;             
      PRInt32 mEndOffset;               
      PRInt16 mType, mLeftType, mRightType;  
      WSFragment *mLeft, *mRight;            

      WSFragment() : mStartNode(0),mEndNode(0),mStartOffset(0),
                     mEndOffset(0),mType(0),mLeftType(0),
                     mRightType(0),mLeft(0),mRight(0) {}
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
          
          
          mTextNode = nsnull;
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
    void     MakeSingleWSRun(PRInt16 aType);
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
    PRInt16 mStartReason;                 
    nsCOMPtr<nsIDOMNode> mStartReasonNode;
    
    nsCOMPtr<nsIDOMNode> mEndNode;        
    PRInt32 mEndOffset;                   
    PRInt16 mEndReason;                   
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

