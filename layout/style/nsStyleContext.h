






#ifndef _nsStyleContext_h_
#define _nsStyleContext_h_

#include "mozilla/RestyleLogging.h"
#include "mozilla/Assertions.h"
#include "nsRuleNode.h"
#include "nsCSSPseudoElements.h"

class nsIAtom;
class nsPresContext;























class nsStyleContext final
{
public:
  























  nsStyleContext(nsStyleContext* aParent, nsIAtom* aPseudoTag,
                 nsCSSPseudoElements::Type aPseudoType,
                 nsRuleNode* aRuleNode,
                 bool aSkipParentDisplayBasedStyleFixup);

  void* operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW;
  void Destroy();

#ifdef DEBUG
  


  static void Initialize();
#endif

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

#ifdef DEBUG
  void FrameAddRef() {
    ++mFrameRefCnt;
  }

  void FrameRelease() {
    --mFrameRefCnt;
  }

  uint32_t FrameRefCnt() const {
    return mFrameRefCnt;
  }
#endif

  bool HasSingleReference() const {
    NS_ASSERTION(mRefCnt != 0,
                 "do not call HasSingleReference on a newly created "
                 "nsStyleContext with no references yet");
    return mRefCnt == 1;
  }

  nsPresContext* PresContext() const { return mRuleNode->PresContext(); }

  nsStyleContext* GetParent() const { return mParent; }

  nsIAtom* GetPseudo() const { return mPseudoTag; }
  nsCSSPseudoElements::Type GetPseudoType() const {
    return static_cast<nsCSSPseudoElements::Type>(mBits >>
                                                  NS_STYLE_CONTEXT_TYPE_SHIFT);
  }

  enum {
    eRelevantLinkVisited = 1 << 0,
    eSuppressLineBreak = 1 << 1
  };

  
  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  FindChildWithRules(const nsIAtom* aPseudoTag, nsRuleNode* aRules,
                     nsRuleNode* aRulesIfVisited, uint32_t aFlags);

  
  
  bool HasTextDecorationLines() const
    { return !!(mBits & NS_STYLE_HAS_TEXT_DECORATION_LINES); }

  
  
  
  
  
  bool ShouldSuppressLineBreak() const
    { return !!(mBits & NS_STYLE_SUPPRESS_LINEBREAK); }

  
  bool IsInDisplayNoneSubtree() const
    { return !!(mBits & NS_STYLE_IN_DISPLAY_NONE_SUBTREE); }

  
  
  
  
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
    MOZ_ASSERT(!IsStyleIfVisited(), "this context is not visited data");
    NS_ASSERTION(!mStyleIfVisited, "should only be set once");

    mStyleIfVisited = aStyleIfVisited;

    MOZ_ASSERT(mStyleIfVisited->IsStyleIfVisited(),
               "other context is visited data");
    MOZ_ASSERT(!mStyleIfVisited->GetStyleIfVisited(),
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

  
  
  bool IsShared() const
    { return !!(mBits & NS_STYLE_IS_SHARED); }

  
  
  bool HasChildThatUsesGrandancestorStyle() const;

  
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

  



  bool HasSameCachedStyleData(nsStyleContext* aOther, nsStyleStructID aSID);

  



  bool HasCachedInheritedStyleData(nsStyleStructID aSID)
    { return mBits & nsCachedStyleData::GetBitForSID(aSID); }

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

  


















  nsChangeHint CalcStyleDifference(nsStyleContext* aOther,
                                   nsChangeHint aParentHintsNotHandledForDescendants,
                                   uint32_t* aEqualStructs);

  










  nscolor GetVisitedDependentColor(nsCSSProperty aProperty);

  






  static nscolor CombineVisitedColors(nscolor *aColors,
                                      bool aLinkIsVisited);

  


  void StartBackgroundImageLoads() {
    
    StyleBackground();
  }

  






  void MoveTo(nsStyleContext* aNewParent);

  

















  void SwapStyleData(nsStyleContext* aNewContext, uint32_t aStructs);

  



  void ClearCachedInheritedStyleDataOnDescendants(uint32_t aStructs);

  




  void SetIneligibleForSharing();

#ifdef DEBUG
  void List(FILE* out, int32_t aIndent, bool aListDescendants = true);
  static void AssertStyleStructMaxDifferenceValid();
  static const char* StructName(nsStyleStructID aSID);
  static bool LookupStruct(const nsACString& aName, nsStyleStructID& aResult);
#endif

#ifdef RESTYLE_LOGGING
  nsCString GetCachedStyleDataAsString(uint32_t aStructs);
  void LogStyleContextTree(int32_t aLoggingDepth, uint32_t aStructs);
  int32_t& LoggingDepth();
#endif

private:
  
  ~nsStyleContext();

  void AddChild(nsStyleContext* aChild);
  void RemoveChild(nsStyleContext* aChild);

  void* GetUniqueStyleData(const nsStyleStructID& aSID);
  void* CreateEmptyStyleData(const nsStyleStructID& aSID);

  void ApplyStyleFixups(bool aSkipParentDisplayBasedStyleFixup);

  
  static bool ListContainsStyleContextThatUsesGrandancestorStyle(
                                                   const nsStyleContext* aHead);

  
  
  inline const void* GetCachedStyleData(nsStyleStructID aSID);

#ifdef DEBUG
  struct AutoCheckDependency {

    nsStyleContext* mStyleContext;
    nsStyleStructID mOuterSID;

    AutoCheckDependency(nsStyleContext* aContext, nsStyleStructID aInnerSID)
      : mStyleContext(aContext)
    {
      mOuterSID = aContext->mComputingStruct;
      MOZ_ASSERT(mOuterSID == nsStyleStructID_None ||
                 DependencyAllowed(mOuterSID, aInnerSID),
                 "Undeclared dependency, see generate-stylestructlist.py");
      aContext->mComputingStruct = aInnerSID;
    }

    ~AutoCheckDependency()
    {
      mStyleContext->mComputingStruct = mOuterSID;
    }

  };

#define AUTO_CHECK_DEPENDENCY(sid_) \
  AutoCheckDependency checkNesting_(this, sid_)
#else
#define AUTO_CHECK_DEPENDENCY(sid_)
#endif

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)                  \
    const nsStyle##name_ * DoGetStyle##name_(bool aComputeData) {       \
      const nsStyle##name_ * cachedData =                               \
        static_cast<nsStyle##name_*>(                                   \
          mCachedInheritedData.mStyleStructs[eStyleStruct_##name_]);    \
      if (cachedData) /* Have it cached already, yay */                 \
        return cachedData;                                              \
      /* Have the rulenode deal */                                      \
      AUTO_CHECK_DEPENDENCY(eStyleStruct_##name_);                      \
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
      AUTO_CHECK_DEPENDENCY(eStyleStruct_##name_);                      \
      return mRuleNode->GetStyle##name_(this, aComputeData);            \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  
  void DoClearCachedInheritedStyleDataOnDescendants(uint32_t aStructs);

#ifdef DEBUG
  void AssertStructsNotUsedElsewhere(nsStyleContext* aDestroyingContext,
                                     int32_t aLevels) const;
#endif

#ifdef RESTYLE_LOGGING
  void LogStyleContextTree(bool aFirst, uint32_t aStructs);

  
  
  
  bool ShouldLogRestyle() { return true; }
#endif

  nsStyleContext* mParent; 

  
  
  
  
  
  
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

#ifdef DEBUG
  uint32_t                mFrameRefCnt; 
                                        

  nsStyleStructID         mComputingStruct;

  static bool DependencyAllowed(nsStyleStructID aOuterSID,
                                nsStyleStructID aInnerSID)
  {
    return !!(sDependencyTable[aOuterSID] &
              nsCachedStyleData::GetBitForSID(aInnerSID));
  }

  static const uint32_t sDependencyTable[];
#endif
};

already_AddRefed<nsStyleContext>
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsCSSPseudoElements::Type aPseudoType,
                   nsRuleNode* aRuleNode,
                   bool aSkipParentDisplayBasedStyleFixup);
#endif
