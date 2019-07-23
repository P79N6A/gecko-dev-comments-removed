






































#ifndef nsQuoteList_h___
#define nsQuoteList_h___

#include "nsGenConList.h"

struct nsQuoteNode : public nsGenConNode {
  
  const nsStyleContentType mType;

  
  PRInt32 mDepthBefore;

  nsQuoteNode(nsStyleContentType& aType, PRUint32 aContentIndex)
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

  virtual PRBool InitTextFrame(nsGenConList* aList, 
          nsIFrame* aPseudoFrame, nsIFrame* aTextFrame);

  
  PRBool IsOpenQuote() {
    return mType == eStyleContentType_OpenQuote ||
           mType == eStyleContentType_NoOpenQuote;
  }

  
  PRBool IsCloseQuote() {
    return !IsOpenQuote();
  }

  
  PRBool IsRealQuote() {
    return mType == eStyleContentType_OpenQuote ||
           mType == eStyleContentType_CloseQuote;
  }

  
  
  
  PRInt32 Depth() {
    return IsOpenQuote() ? mDepthBefore : mDepthBefore - 1;
  }

  
  PRInt32 DepthAfter() {
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
