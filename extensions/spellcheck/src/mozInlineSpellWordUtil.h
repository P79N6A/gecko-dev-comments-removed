




































#include "nsCOMPtr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMViewCSS.h"
#include "nsIDocument.h"
#include "nsString.h"
#include "nsTArray.h"



class nsIDOMRange;
class nsIDOMNode;





















class mozInlineSpellWordUtil
{
public:
  struct NodeOffset {
    nsIDOMNode* mNode;
    PRInt32     mOffset;
    
    NodeOffset(nsIDOMNode* aNode, PRInt32 aOffset) :
      mNode(aNode), mOffset(aOffset) {}
  };

  mozInlineSpellWordUtil()
    : mRootNode(nsnull),
      mSoftBegin(nsnull, 0), mSoftEnd(nsnull, 0),
      mNextWordIndex(-1), mSoftTextValid(PR_FALSE) {}

  nsresult Init(nsWeakPtr aWeakEditor);

  nsresult SetEnd(nsIDOMNode* aEndNode, PRInt32 aEndOffset);

  
  
  nsresult SetPosition(nsIDOMNode* aNode, PRInt32 aOffset);

  
  
  
  
  
  
  
  
  
  nsresult GetRangeForWord(nsIDOMNode* aWordNode, PRInt32 aWordOffset,
                           nsIDOMRange** aRange);

  
  
  
  
  nsresult GetNextWord(nsAString& aText, nsIDOMRange** aRange,
                       PRBool* aSkipChecking);

  
  
  static void NormalizeWord(nsSubstring& aWord);

  nsIDOMDocumentRange* GetDocumentRange() const { return mDOMDocumentRange; }
  nsIDocument* GetDocument() const { return mDocument; }
  nsIDOMNode* GetRootNode() { return mRootNode; }

private:

  
  nsCOMPtr<nsIDOMDocumentRange> mDOMDocumentRange;
  nsCOMPtr<nsIDocument>         mDocument;
  nsCOMPtr<nsIDOMViewCSS>       mCSSView;

  
  nsIDOMNode* mRootNode;
  NodeOffset  mSoftBegin;
  NodeOffset  mSoftEnd;

  
  nsString mSoftText;
  
  
  struct DOMTextMapping {
    NodeOffset mNodeOffset;
    PRInt32    mSoftTextOffset;
    PRInt32    mLength;
    
    DOMTextMapping(NodeOffset aNodeOffset, PRInt32 aSoftTextOffset, PRInt32 aLength)
      : mNodeOffset(aNodeOffset), mSoftTextOffset(aSoftTextOffset),
        mLength(aLength) {}
  };
  nsTArray<DOMTextMapping> mSoftTextDOMMapping;
  
  
  struct RealWord {
    PRInt32      mSoftTextOffset;
    PRInt32      mLength;
    PRPackedBool mCheckableWord;
    
    RealWord(PRInt32 aOffset, PRInt32 aLength, PRPackedBool aCheckable)
      : mSoftTextOffset(aOffset), mLength(aLength), mCheckableWord(aCheckable) {}
    PRInt32 EndOffset() const { return mSoftTextOffset + mLength; }
  };
  nsTArray<RealWord> mRealWords;
  PRInt32            mNextWordIndex;

  PRPackedBool mSoftTextValid;

  void InvalidateWords() { mSoftTextValid = PR_FALSE; }
  void EnsureWords();
  
  PRInt32 MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset);
  
  
  
  
  
  
  enum DOMMapHint { HINT_BEGIN, HINT_END };
  NodeOffset MapSoftTextOffsetToDOMPosition(PRInt32 aSoftTextOffset,
                                            DOMMapHint aHint);
  
  
  
  
  
  
  PRInt32 FindRealWordContaining(PRInt32 aSoftTextOffset, DOMMapHint aHint,
                                 PRBool aSearchForward);
    
  
  void BuildSoftText();
  
  void BuildRealWords();

  void SplitDOMWord(PRInt32 aStart, PRInt32 aEnd);

  
  nsresult MakeRange(NodeOffset aBegin, NodeOffset aEnd, nsIDOMRange** aRange);
  nsresult MakeRangeForWord(const RealWord& aWord, nsIDOMRange** aRange);
};
