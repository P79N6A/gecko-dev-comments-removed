




































#include "nsCOMPtr.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsString.h"
#include "nsTArray.h"



class nsRange;
class nsINode;





















class mozInlineSpellWordUtil
{
public:
  struct NodeOffset {
    nsINode* mNode;
    PRInt32  mOffset;
    
    NodeOffset(nsINode* aNode, PRInt32 aOffset) :
      mNode(aNode), mOffset(aOffset) {}
  };

  mozInlineSpellWordUtil()
    : mRootNode(nsnull),
      mSoftBegin(nsnull, 0), mSoftEnd(nsnull, 0),
      mNextWordIndex(-1), mSoftTextValid(false) {}

  nsresult Init(nsWeakPtr aWeakEditor);

  nsresult SetEnd(nsINode* aEndNode, PRInt32 aEndOffset);

  
  
  nsresult SetPosition(nsINode* aNode, PRInt32 aOffset);

  
  
  
  
  
  
  
  
  
  nsresult GetRangeForWord(nsIDOMNode* aWordNode, PRInt32 aWordOffset,
                           nsRange** aRange);

  
  
  
  
  nsresult GetNextWord(nsAString& aText, nsRange** aRange,
                       bool* aSkipChecking);

  
  
  static void NormalizeWord(nsSubstring& aWord);

  nsIDOMDocument* GetDOMDocument() const { return mDOMDocument; }
  nsIDocument* GetDocument() const { return mDocument; }
  nsINode* GetRootNode() { return mRootNode; }
  
private:

  
  nsCOMPtr<nsIDOMDocument> mDOMDocument;
  nsCOMPtr<nsIDocument>         mDocument;

  
  nsINode*    mRootNode;
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
    bool mCheckableWord;
    
    RealWord(PRInt32 aOffset, PRInt32 aLength, bool aCheckable)
      : mSoftTextOffset(aOffset), mLength(aLength), mCheckableWord(aCheckable) {}
    PRInt32 EndOffset() const { return mSoftTextOffset + mLength; }
  };
  nsTArray<RealWord> mRealWords;
  PRInt32            mNextWordIndex;

  bool mSoftTextValid;

  void InvalidateWords() { mSoftTextValid = false; }
  void EnsureWords();
  
  PRInt32 MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset);
  
  
  
  
  
  
  enum DOMMapHint { HINT_BEGIN, HINT_END };
  NodeOffset MapSoftTextOffsetToDOMPosition(PRInt32 aSoftTextOffset,
                                            DOMMapHint aHint);
  
  
  
  
  
  
  PRInt32 FindRealWordContaining(PRInt32 aSoftTextOffset, DOMMapHint aHint,
                                 bool aSearchForward);
    
  
  void BuildSoftText();
  
  void BuildRealWords();

  void SplitDOMWord(PRInt32 aStart, PRInt32 aEnd);

  
  nsresult MakeRange(NodeOffset aBegin, NodeOffset aEnd, nsRange** aRange);
  nsresult MakeRangeForWord(const RealWord& aWord, nsRange** aRange);
};
