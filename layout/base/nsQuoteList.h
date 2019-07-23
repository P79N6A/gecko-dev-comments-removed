






































#ifndef nsQuoteList_h___
#define nsQuoteList_h___

#include "nsGenConList.h"
#include "nsStyleContext.h"

struct nsQuoteNode : public nsGenConNode {
  
  const nsStyleContentType mType;

  
  PRInt32 mDepthBefore;

  nsRefPtr<nsStyleContext> mStyleContext;

  nsQuoteNode(nsStyleContentType& aType, nsIContent* aContentParent,
              PRUint32 aContentIndex, nsStyleContext* aStyleContext)
    : nsGenConNode(aContentParent, aStyleContext, aContentIndex)
    , mType(aType)
    , mDepthBefore(0)
    , mStyleContext(aStyleContext)
  {
    NS_ASSERTION(aType == eStyleContentType_OpenQuote ||
                 aType == eStyleContentType_CloseQuote ||
                 aType == eStyleContentType_NoOpenQuote ||
                 aType == eStyleContentType_NoCloseQuote,
                 "incorrect type");
    NS_ASSERTION(aContentIndex >= 0, "out of range");
  }

  
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

  
  PRBool IsHiddenQuote() {
    return !IsRealQuote();
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
