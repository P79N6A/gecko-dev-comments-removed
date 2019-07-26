




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
    int32_t  mOffset;
    
    NodeOffset(nsINode* aNode, int32_t aOffset) :
      mNode(aNode), mOffset(aOffset) {}
  };

  mozInlineSpellWordUtil()
    : mRootNode(nullptr),
      mSoftBegin(nullptr, 0), mSoftEnd(nullptr, 0),
      mNextWordIndex(-1), mSoftTextValid(false) {}

  nsresult Init(nsWeakPtr aWeakEditor);

  nsresult SetEnd(nsINode* aEndNode, int32_t aEndOffset);

  
  
  nsresult SetPosition(nsINode* aNode, int32_t aOffset);

  
  
  
  
  
  
  
  
  
  nsresult GetRangeForWord(nsIDOMNode* aWordNode, int32_t aWordOffset,
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
    int32_t    mSoftTextOffset;
    int32_t    mLength;
    
    DOMTextMapping(NodeOffset aNodeOffset, int32_t aSoftTextOffset, int32_t aLength)
      : mNodeOffset(aNodeOffset), mSoftTextOffset(aSoftTextOffset),
        mLength(aLength) {}
  };
  nsTArray<DOMTextMapping> mSoftTextDOMMapping;
  
  
  struct RealWord {
    int32_t      mSoftTextOffset;
    int32_t      mLength;
    bool mCheckableWord;
    
    RealWord(int32_t aOffset, int32_t aLength, bool aCheckable)
      : mSoftTextOffset(aOffset), mLength(aLength), mCheckableWord(aCheckable) {}
    int32_t EndOffset() const { return mSoftTextOffset + mLength; }
  };
  nsTArray<RealWord> mRealWords;
  int32_t            mNextWordIndex;

  bool mSoftTextValid;

  void InvalidateWords() { mSoftTextValid = false; }
  void EnsureWords();
  
  int32_t MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset);
  
  
  
  
  
  
  enum DOMMapHint { HINT_BEGIN, HINT_END };
  NodeOffset MapSoftTextOffsetToDOMPosition(int32_t aSoftTextOffset,
                                            DOMMapHint aHint);
  
  
  
  
  
  
  int32_t FindRealWordContaining(int32_t aSoftTextOffset, DOMMapHint aHint,
                                 bool aSearchForward);
    
  
  void BuildSoftText();
  
  void BuildRealWords();

  void SplitDOMWord(int32_t aStart, int32_t aEnd);

  
  nsresult MakeRange(NodeOffset aBegin, NodeOffset aEnd, nsRange** aRange);
  nsresult MakeRangeForWord(const RealWord& aWord, nsRange** aRange);
};
