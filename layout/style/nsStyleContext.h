








































#ifndef _nsStyleContext_h_
#define _nsStyleContext_h_

#include "nsRuleNode.h"
#include "nsIAtom.h"
#include "nsCSSPseudoElements.h"

class nsPresContext;























class nsStyleContext
{
public:
  nsStyleContext(nsStyleContext* aParent, nsIAtom* aPseudoTag,
                 nsCSSPseudoElements::Type aPseudoType,
                 nsRuleNode* aRuleNode, nsPresContext* aPresContext) NS_HIDDEN;
  ~nsStyleContext() NS_HIDDEN;

  NS_HIDDEN_(void*) operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW;
  NS_HIDDEN_(void) Destroy();

  nsrefcnt AddRef() {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking object");
      return mRefCnt;
    }
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsStyleContext", sizeof(nsStyleContext));
    return mRefCnt;
  }

  nsrefcnt Release() {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking object");
      return mRefCnt;
    }
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

  nsIAtom* GetPseudo() const { return mPseudoTag; }
  nsCSSPseudoElements::Type GetPseudoType() const {
    return static_cast<nsCSSPseudoElements::Type>(mBits >>
                                                  NS_STYLE_CONTEXT_TYPE_SHIFT);
  }

  
  
  
  
  
  
  
  NS_HIDDEN_(already_AddRefed<nsStyleContext>)
  FindChildWithRules(const nsIAtom* aPseudoTag, nsRuleNode* aRules,
                     nsRuleNode* aRulesIfVisited,
                     PRBool aRelevantLinkVisited);

  
  
  PRBool HasTextDecorations() const
    { return !!(mBits & NS_STYLE_HAS_TEXT_DECORATIONS); }

  
  
  
  
  PRBool HasPseudoElementData() const
    { return !!(mBits & NS_STYLE_HAS_PSEUDO_ELEMENT_DATA); }

  
  
  
  PRBool RelevantLinkVisited() const
    { return !!(mBits & NS_STYLE_RELEVANT_LINK_VISITED); }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsStyleContext* GetStyleIfVisited()
    { return mStyleIfVisited; }

  
  void SetStyleIfVisited(already_AddRefed<nsStyleContext> aStyleIfVisited)
  {
    NS_ASSERTION(!mStyleIfVisited, "should only be set once");
    mStyleIfVisited = aStyleIfVisited;

    NS_ASSERTION(GetStyleIfVisited()->GetPseudo() == GetPseudo(),
                 "pseudo tag mismatch");
    if (GetParent() && GetParent()->GetStyleIfVisited()) {
      NS_ASSERTION(GetStyleIfVisited()->GetParent() ==
                     GetParent()->GetStyleIfVisited() ||
                   GetStyleIfVisited()->GetParent() == GetParent(),
                   "parent mismatch");
    } else {
      NS_ASSERTION(GetStyleIfVisited()->GetParent() == GetParent(),
                   "parent mismatch");
    }
  }

  
  NS_HIDDEN_(void) SetStyle(nsStyleStructID aSID, void* aStruct);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)          \
    void SetStyle##name_ (nsStyle##name_ * aStruct) {                       \
      NS_ASSERTION(!mCachedInheritedData.m##name_##Data ||                  \
                   (mBits &                                                 \
                    nsCachedStyleData::GetBitForSID(eStyleStruct_##name_)), \
                   "Going to leak styledata");                              \
      mCachedInheritedData.m##name_##Data = aStruct;                        \
    }
#define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  nsRuleNode* GetRuleNode() { return mRuleNode; }
  void AddStyleBit(const PRUint32& aBit) { mBits |= aBit; }

  



  NS_HIDDEN_(void) Mark();

  















  NS_HIDDEN_(const void*) NS_FASTCALL GetStyleData(nsStyleStructID aSID);

  






  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)  \
    const nsStyle##name_ * GetStyle##name_() {            \
      return DoGetStyle##name_(PR_TRUE);                  \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  






  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)  \
    const nsStyle##name_ * PeekStyle##name_() {           \
      return DoGetStyle##name_(PR_FALSE);                 \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  NS_HIDDEN_(void*) GetUniqueStyleData(const nsStyleStructID& aSID);

  NS_HIDDEN_(nsChangeHint) CalcStyleDifference(nsStyleContext* aOther);

  










  NS_HIDDEN_(nscolor) GetVisitedDependentColor(nsCSSProperty aProperty);

  






  static nscolor CombineVisitedColors(nscolor *aColors,
                                      PRBool aLinkIsVisited);

#ifdef DEBUG
  NS_HIDDEN_(void) List(FILE* out, PRInt32 aIndent);
#endif

protected:
  NS_HIDDEN_(void) AddChild(nsStyleContext* aChild);
  NS_HIDDEN_(void) RemoveChild(nsStyleContext* aChild);

  NS_HIDDEN_(void) ApplyStyleFixups(nsPresContext* aPresContext);

  
  
  inline const void* GetCachedStyleData(nsStyleStructID aSID);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)      \
    const nsStyle##name_ * DoGetStyle##name_(PRBool aComputeData) {     \
      const nsStyle##name_ * cachedData =                               \
        mCachedInheritedData.m##name_##Data;                            \
      if (cachedData) /* Have it cached already, yay */                 \
        return cachedData;                                              \
      /* Have the rulenode deal */                                      \
      return mRuleNode->GetStyle##name_(this, aComputeData);            \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)          \
    const nsStyle##name_ * DoGetStyle##name_(PRBool aComputeData) {     \
      const nsStyle##name_ * cachedData =                               \
        mCachedResetData ? mCachedResetData->m##name_##Data : nsnull;   \
      if (cachedData) /* Have it cached already, yay */                 \
        return cachedData;                                              \
      /* Have the rulenode deal */                                      \
      return mRuleNode->GetStyle##name_(this, aComputeData);            \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  nsStyleContext* const mParent; 

  
  
  
  
  
  
  nsStyleContext* mChild;
  nsStyleContext* mEmptyChild;
  nsStyleContext* mPrevSibling;
  nsStyleContext* mNextSibling;

  
  
  
  nsRefPtr<nsStyleContext> mStyleIfVisited;

  
  
  nsCOMPtr<nsIAtom> mPseudoTag;

  
  
  
  
  
  
  nsRuleNode* const       mRuleNode;

  
  
  
  
  
  
  
  
  
  nsResetStyleData*       mCachedResetData; 
  nsInheritedStyleData    mCachedInheritedData; 
  PRUint32                mBits; 
                                 
  PRUint32                mRefCnt;
};

NS_HIDDEN_(already_AddRefed<nsStyleContext>)
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsCSSPseudoElements::Type aPseudoType,
                   nsRuleNode* aRuleNode,
                   nsPresContext* aPresContext);
#endif
