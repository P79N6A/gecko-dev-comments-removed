








































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
                 nsRuleNode* aRuleNode, nsPresContext* aPresContext);
  ~nsStyleContext();

  void* operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW;
  void Destroy();

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

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  FindChildWithRules(const nsIAtom* aPseudoTag, nsRuleNode* aRules,
                     nsRuleNode* aRulesIfVisited,
                     PRBool aRelevantLinkVisited);

  
  
  PRBool HasTextDecorationLines() const
    { return !!(mBits & NS_STYLE_HAS_TEXT_DECORATION_LINES); }

  
  
  
  
  PRBool HasPseudoElementData() const
    { return !!(mBits & NS_STYLE_HAS_PSEUDO_ELEMENT_DATA); }

  
  
  
  PRBool RelevantLinkVisited() const
    { return !!(mBits & NS_STYLE_RELEVANT_LINK_VISITED); }

  
  PRBool IsLinkContext() const {
    return
      GetStyleIfVisited() && GetStyleIfVisited()->GetParent() == GetParent();
  }

  
  
  PRBool IsStyleIfVisited() const
    { return !!(mBits & NS_STYLE_IS_STYLE_IF_VISITED); }

  
  
  void SetIsStyleIfVisited()
    { mBits |= NS_STYLE_IS_STYLE_IF_VISITED; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsStyleContext* GetStyleIfVisited() const
    { return mStyleIfVisited; }

  
  void SetStyleIfVisited(already_AddRefed<nsStyleContext> aStyleIfVisited)
  {
    NS_ABORT_IF_FALSE(!IsStyleIfVisited(), "this context is not visited data");
    NS_ABORT_IF_FALSE(aStyleIfVisited.get()->IsStyleIfVisited(),
                      "other context is visited data");
    NS_ABORT_IF_FALSE(!aStyleIfVisited.get()->GetStyleIfVisited(),
                      "other context does not have visited data");
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

  
  void SetStyle(nsStyleStructID aSID, void* aStruct);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)          \
    void SetStyle##name_ (nsStyle##name_ * aStruct) {                       \
      void *& slot =                                                        \
        mCachedInheritedData.mStyleStructs[eStyleStruct_##name_];           \
      NS_ASSERTION(!slot ||                                                 \
                   (mBits &                                                 \
                    nsCachedStyleData::GetBitForSID(eStyleStruct_##name_)), \
                   "Going to leak styledata");                              \
      slot = aStruct;                                                       \
    }
#define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  nsRuleNode* GetRuleNode() { return mRuleNode; }
  void AddStyleBit(const PRUint32& aBit) { mBits |= aBit; }

  



  void Mark();

  















  const void* NS_FASTCALL GetStyleData(nsStyleStructID aSID);

  






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

  void* GetUniqueStyleData(const nsStyleStructID& aSID);

  nsChangeHint CalcStyleDifference(nsStyleContext* aOther);

  










  nscolor GetVisitedDependentColor(nsCSSProperty aProperty);

  






  static nscolor CombineVisitedColors(nscolor *aColors,
                                      PRBool aLinkIsVisited);

  


















  void* Alloc(size_t aSize);

  


  void StartBackgroundImageLoads() {
    
    GetStyleBackground();
  }

#ifdef DEBUG
  void List(FILE* out, PRInt32 aIndent);
#endif

protected:
  void AddChild(nsStyleContext* aChild);
  void RemoveChild(nsStyleContext* aChild);

  void ApplyStyleFixups(nsPresContext* aPresContext);

  void FreeAllocations(nsPresContext* aPresContext);

  
  
  inline const void* GetCachedStyleData(nsStyleStructID aSID);

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)      \
    const nsStyle##name_ * DoGetStyle##name_(PRBool aComputeData) {     \
      const nsStyle##name_ * cachedData =                               \
        static_cast<nsStyle##name_*>(                                   \
          mCachedInheritedData.mStyleStructs[eStyleStruct_##name_]);    \
      if (cachedData) /* Have it cached already, yay */                 \
        return cachedData;                                              \
      /* Have the rulenode deal */                                      \
      return mRuleNode->GetStyle##name_(this, aComputeData);            \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)          \
    const nsStyle##name_ * DoGetStyle##name_(PRBool aComputeData) {     \
      const nsStyle##name_ * cachedData = mCachedResetData              \
        ? static_cast<nsStyle##name_*>(                                 \
            mCachedResetData->mStyleStructs[eStyleStruct_##name_])      \
        : nsnull;                                                       \
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

  
  struct AllocationHeader {
    AllocationHeader* mNext;
    size_t mSize;

    void* mStorageStart; 
  };
  AllocationHeader*       mAllocations;

  
  
  
  
  
  
  
  
  
  nsResetStyleData*       mCachedResetData; 
  nsInheritedStyleData    mCachedInheritedData; 
  PRUint32                mBits; 
                                 
  PRUint32                mRefCnt;
};

already_AddRefed<nsStyleContext>
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsCSSPseudoElements::Type aPseudoType,
                   nsRuleNode* aRuleNode,
                   nsPresContext* aPresContext);
#endif
