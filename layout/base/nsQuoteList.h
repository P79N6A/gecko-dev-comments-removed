






#ifndef nsQuoteList_h___
#define nsQuoteList_h___

#include "nsGenConList.h"

struct nsQuoteNode : public nsGenConNode {
  
  const nsStyleContentType mType;

  
  int32_t mDepthBefore;

  nsQuoteNode(nsStyleContentType& aType, uint32_t aContentIndex)
    : nsGenConNode(aContentIndex)
    , mType(aType)
    , mDepthBefore(0)
  {
    NS_ASSERTION(aType == eStyleContentType_OpenQuote ||
                 aType == eStyleContentType_CloseQuote ||
                 aType == eStyleContentType_NoOpenQuote ||
                 aType == eStyleContentType_NoCloseQuote,
                 "incorrect type");
    NS_ASSERTION(aContentIndex <= PR_INT32_MAX, "out of range");
  }

  virtual bool InitTextFrame(nsGenConList* aList, 
          nsIFrame* aPseudoFrame, nsIFrame* aTextFrame);

  
  bool IsOpenQuote() {
    return mType == eStyleContentType_OpenQuote ||
           mType == eStyleContentType_NoOpenQuote;
  }

  
  bool IsCloseQuote() {
    return !IsOpenQuote();
  }

  
  bool IsRealQuote() {
    return mType == eStyleContentType_OpenQuote ||
           mType == eStyleContentType_CloseQuote;
  }

  
  
  
  int32_t Depth() {
    return IsOpenQuote() ? mDepthBefore : mDepthBefore - 1;
  }

  
  int32_t DepthAfter() {
    return IsOpenQuote() ? mDepthBefore + 1
                         : (mDepthBefore == 0 ? 0 : mDepthBefore - 1);
  }

  
  const nsString* Text();
};

class nsQuoteList : public nsGenConList {
private:
  nsQuoteNode* FirstNode() { return static_cast<nsQuoteNode*>(mFirstNode); }
public:
  
  
  void Calc(nsQuoteNode* aNode);

  nsQuoteNode* Next(nsQuoteNode* aNode) {
    return static_cast<nsQuoteNode*>(nsGenConList::Next(aNode));
  }
  nsQuoteNode* Prev(nsQuoteNode* aNode) {
    return static_cast<nsQuoteNode*>(nsGenConList::Prev(aNode));
  }
  
  void RecalcAll();
#ifdef DEBUG
  void PrintChain();
#endif
};

#endif 
