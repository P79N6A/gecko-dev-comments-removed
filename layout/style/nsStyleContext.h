








































#ifndef _nsStyleContext_h_
#define _nsStyleContext_h_

#include "nsRuleNode.h"
#include "nsIAtom.h"

class nsPresContext;























class nsStyleContext
{
public:
  nsStyleContext(nsStyleContext* aParent, nsIAtom* aPseudoTag, 
                 nsRuleNode* aRuleNode, nsPresContext* aPresContext) NS_HIDDEN;
  ~nsStyleContext() NS_HIDDEN;

  NS_HIDDEN_(void*) operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW;
  NS_HIDDEN_(void) Destroy();

  nsrefcnt AddRef() {
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsStyleContext", sizeof(nsStyleContext));
    return mRefCnt;
  }

  nsrefcnt Release() {
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsStyleContext");
    if (mRefCnt == 0) {
      Destroy();
      return 0;
    }
    return mRefCnt;
  }

  nsPresContext* PresContext() const { return mRuleNode->GetPresContext(); }

  nsStyleContext* GetParent() const { return mParent; }

  nsStyleContext* GetFirstChild() const { return mChild; }

  nsIAtom* GetPseudoType() const { return mPseudoTag; }

  NS_HIDDEN_(already_AddRefed<nsStyleContext>)
  FindChildWithRules(const nsIAtom* aPseudoTag, nsRuleNode* aRules);

  NS_HIDDEN_(PRBool)    Equals(const nsStyleContext* aOther) const;
  PRBool    HasTextDecorations() { return mBits & NS_STYLE_HAS_TEXT_DECORATIONS; };

  NS_HIDDEN_(void) SetStyle(nsStyleStructID aSID, nsStyleStruct* aStruct);

  nsRuleNode* GetRuleNode() { return mRuleNode; }
  void AddStyleBit(const PRUint32& aBit) { mBits |= aBit; }

  



  NS_HIDDEN_(void) Mark();

  

















  NS_HIDDEN_(const nsStyleStruct*) NS_FASTCALL GetStyleData(nsStyleStructID aSID);

  







  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    NS_HIDDEN_(const nsStyle##name_ *) NS_FASTCALL GetStyle##name_();
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT


  NS_HIDDEN_(const nsStyleStruct*) PeekStyleData(nsStyleStructID aSID);

  NS_HIDDEN_(nsStyleStruct*) GetUniqueStyleData(const nsStyleStructID& aSID);

  NS_HIDDEN_(void) ClearStyleData(nsPresContext* aPresContext);

  NS_HIDDEN_(nsChangeHint) CalcStyleDifference(nsStyleContext* aOther);

#ifdef DEBUG
  NS_HIDDEN_(void) DumpRegressionData(nsPresContext* aPresContext, FILE* out,
                                      PRInt32 aIndent);

  NS_HIDDEN_(void) List(FILE* out, PRInt32 aIndent);
#endif

protected:
  NS_HIDDEN_(void) AddChild(nsStyleContext* aChild);
  NS_HIDDEN_(void) RemoveChild(nsStyleContext* aChild);

  NS_HIDDEN_(void) ApplyStyleFixups(nsPresContext* aPresContext);

  nsStyleContext* mParent;

  
  
  
  
  
  
  nsStyleContext* mChild;
  nsStyleContext* mEmptyChild;
  nsStyleContext* mPrevSibling;
  nsStyleContext* mNextSibling;

  
  
  nsCOMPtr<nsIAtom> mPseudoTag;

  
  
  
  
  
  
  nsRuleNode* const       mRuleNode;

  
  
  
  
  
  
  nsCachedStyleData       mCachedStyleData; 
  PRUint32                mBits; 
                                 
  PRUint32                mRefCnt;
};

NS_HIDDEN_(already_AddRefed<nsStyleContext>)
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsRuleNode* aRuleNode,
                   nsPresContext* aPresContext);
#endif
