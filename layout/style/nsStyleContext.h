






#ifndef _nsStyleContext_h_
#define _nsStyleContext_h_

#include "nsRuleNode.h"
#include "nsCSSPseudoElements.h"

class nsIAtom;
class nsPresContext;























class nsStyleContext MOZ_FINAL
{
public:
  























  nsStyleContext(nsStyleContext* aParent, nsIAtom* aPseudoTag,
                 nsCSSPseudoElements::Type aPseudoType,
                 nsRuleNode* aRuleNode,
                 bool aSkipParentDisplayBasedStyleFixup);

  void* operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW;
  void Destroy();

  nsrefcnt AddRef() {
    if (mRefCnt == UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking object");
      return mRefCnt;
    }
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsStyleContext", sizeof(nsStyleContext));
    return mRefCnt;
  }

  nsrefcnt Release() {
    if (mRefCnt == UINT32_MAX) {
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

  nsPresContext* PresContext() const { return mRuleNode->PresContext(); }

  nsStyleContext* GetParent() const { return mParent; }

  nsIAtom* GetPseudo() const { return mPseudoTag; }
  nsCSSPseudoElements::Type GetPseudoType() const {
    return static_cast<nsCSSPseudoElements::Type>(mBits >>
                                                  NS_STYLE_CONTEXT_TYPE_SHIFT);
  }

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  FindChildWithRules(const nsIAtom* aPseudoTag, nsRuleNode* aRules,
                     nsRuleNode* aRulesIfVisited,
                     bool aRelevantLinkVisited);

  
  
  bool HasTextDecorationLines() const
    { return !!(mBits & NS_STYLE_HAS_TEXT_DECORATION_LINES); }

  
  
  
  
  bool HasPseudoElementData() const
    { return !!(mBits & NS_STYLE_HAS_PSEUDO_ELEMENT_DATA); }

  
  
  
  bool RelevantLinkVisited() const
    { return !!(mBits & NS_STYLE_RELEVANT_LINK_VISITED); }

  
  bool IsLinkContext() const {
    return
      GetStyleIfVisited() && GetStyleIfVisited()->GetParent() == GetParent();
  }

  
  
  bool IsStyleIfVisited() const
    { return !!(mBits & NS_STYLE_IS_STYLE_IF_VISITED); }

  
  
  void SetIsStyleIfVisited()
    { mBits |= NS_STYLE_IS_STYLE_IF_VISITED; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsStyleContext* GetStyleIfVisited() const
    { return mStyleIfVisited; }

  
  void SetStyleIfVisited(already_AddRefed<nsStyleContext> aStyleIfVisited)
  {
    NS_ABORT_IF_FALSE(!IsStyleIfVisited(), "this context is not visited data");
    NS_ASSERTION(!mStyleIfVisited, "should only be set once");

    mStyleIfVisited = aStyleIfVisited;

    NS_ABORT_IF_FALSE(mStyleIfVisited->IsStyleIfVisited(),
                      "other context is visited data");
    NS_ABORT_IF_FALSE(!mStyleIfVisited->GetStyleIfVisited(),
                      "other context does not have visited data");
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

  
  
  
  bool UsesGrandancestorStyle() const
    { return !!(mBits & NS_STYLE_USES_GRANDANCESTOR_STYLE); }

  
  void SetStyle(nsStyleStructID aSID, void* aStruct);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)                      \
    void SetStyle##name_ (nsStyle##name_ * aStruct) {                       \
      void *& slot =                                                        \
        mCachedInheritedData.mStyleStructs[eStyleStruct_##name_];           \
      NS_ASSERTION(!slot ||                                                 \
                   (mBits &                                                 \
                    nsCachedStyleData::GetBitForSID(eStyleStruct_##name_)), \
                   "Going to leak styledata");                              \
      slot = aStruct;                                                       \
    }
#define STYLE_STRUCT_RESET(name_, checkdata_cb_)
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  nsRuleNode* RuleNode() { return mRuleNode; }
  void AddStyleBit(const uint64_t& aBit) { mBits |= aBit; }

  



  void Mark();

  















  const void* NS_FASTCALL StyleData(nsStyleStructID aSID);

  






  #define STYLE_STRUCT(name_, checkdata_cb_)              \
    const nsStyle##name_ * Style##name_() {               \
      return DoGetStyle##name_(true);                     \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  






  #define STYLE_STRUCT(name_, checkdata_cb_)              \
    const nsStyle##name_ * PeekStyle##name_() {           \
      return DoGetStyle##name_(false);                    \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  void* GetUniqueStyleData(const nsStyleStructID& aSID);

  















  nsChangeHint CalcStyleDifference(nsStyleContext* aOther,
                                   nsChangeHint aParentHintsNotHandledForDescendants);

  










  nscolor GetVisitedDependentColor(nsCSSProperty aProperty);

  






  static nscolor CombineVisitedColors(nscolor *aColors,
                                      bool aLinkIsVisited);

  


  void StartBackgroundImageLoads() {
    
    StyleBackground();
  }

#ifdef DEBUG
  void List(FILE* out, int32_t aIndent);
  static void AssertStyleStructMaxDifferenceValid();
#endif

private:
  
  ~nsStyleContext();

  void AddChild(nsStyleContext* aChild);
  void RemoveChild(nsStyleContext* aChild);

  void ApplyStyleFixups(bool aSkipParentDisplayBasedStyleFixup);

  
  
  inline const void* GetCachedStyleData(nsStyleStructID aSID);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)                  \
    const nsStyle##name_ * DoGetStyle##name_(bool aComputeData) {       \
      const nsStyle##name_ * cachedData =                               \
        static_cast<nsStyle##name_*>(                                   \
          mCachedInheritedData.mStyleStructs[eStyleStruct_##name_]);    \
      if (cachedData) /* Have it cached already, yay */                 \
        return cachedData;                                              \
      /* Have the rulenode deal */                                      \
      return mRuleNode->GetStyle##name_(this, aComputeData);            \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_)                      \
    const nsStyle##name_ * DoGetStyle##name_(bool aComputeData) {       \
      const nsStyle##name_ * cachedData = mCachedResetData              \
        ? static_cast<nsStyle##name_*>(                                 \
            mCachedResetData->mStyleStructs[eStyleStruct_##name_])      \
        : nullptr;                                                      \
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
  uint64_t                mBits; 
                                 
  uint32_t                mRefCnt;
};

already_AddRefed<nsStyleContext>
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsCSSPseudoElements::Type aPseudoType,
                   nsRuleNode* aRuleNode,
                   bool aSkipParentDisplayBasedStyleFixup);
#endif
