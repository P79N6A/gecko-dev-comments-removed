











































#ifndef nsRuleNode_h___
#define nsRuleNode_h___

#include "nsPresContext.h"
#include "nsStyleStruct.h"

#include "mozilla/StandardInteger.h"

class nsStyleContext;
struct PLDHashTable;
struct nsRuleData;
class nsIStyleRule;
struct nsCSSValueList;

class nsCSSValue;
struct nsCSSRect;

class nsStyleCoord;
class nsCSSValuePairList;

template <nsStyleStructID MinIndex, nsStyleStructID Count>
class FixedStyleStructArray
{
private:
  void* mArray[Count];
public:
  void*& operator[](nsStyleStructID aIndex) {
    NS_ABORT_IF_FALSE(MinIndex <= aIndex && aIndex < (MinIndex + Count),
                      "out of range");
    return mArray[aIndex - MinIndex];
  }

  const void* operator[](nsStyleStructID aIndex) const {
    NS_ABORT_IF_FALSE(MinIndex <= aIndex && aIndex < (MinIndex + Count),
                      "out of range");
    return mArray[aIndex - MinIndex];
  }
};

struct nsInheritedStyleData
{
  FixedStyleStructArray<nsStyleStructID_Inherited_Start,
                        nsStyleStructID_Inherited_Count> mStyleStructs;

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }

  void DestroyStructs(PRUint32 aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
    void *name##Data = mStyleStructs[eStyleStruct_##name]; \
    if (name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      static_cast<nsStyle##name*>(name##Data)->Destroy(aContext);
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
  }

  void Destroy(PRUint32 aBits, nsPresContext* aContext) {
    DestroyStructs(aBits, aContext);
    aContext->FreeToShell(sizeof(nsInheritedStyleData), this);
  }

  nsInheritedStyleData() {
    for (nsStyleStructID i = nsStyleStructID_Inherited_Start;
         i < nsStyleStructID_Inherited_Start + nsStyleStructID_Inherited_Count;
         i = nsStyleStructID(i + 1)) {
      mStyleStructs[i] = nsnull;
    }
  }
};

struct nsResetStyleData
{
  FixedStyleStructArray<nsStyleStructID_Reset_Start,
                        nsStyleStructID_Reset_Count> mStyleStructs;

  nsResetStyleData()
  {
    for (nsStyleStructID i = nsStyleStructID_Reset_Start;
         i < nsStyleStructID_Reset_Start + nsStyleStructID_Reset_Count;
         i = nsStyleStructID(i + 1)) {
      mStyleStructs[i] = nsnull;
    }
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }

  void Destroy(PRUint32 aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
    void *name##Data = mStyleStructs[eStyleStruct_##name]; \
    if (name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      static_cast<nsStyle##name*>(name##Data)->Destroy(aContext);
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED

    aContext->FreeToShell(sizeof(nsResetStyleData), this);
  }
};

struct nsCachedStyleData
{
  nsInheritedStyleData* mInheritedData;
  nsResetStyleData* mResetData;

  static bool IsReset(const nsStyleStructID aSID) {
    NS_ABORT_IF_FALSE(0 <= aSID && aSID < nsStyleStructID_Length,
                      "must be an inherited or reset SID");
    return nsStyleStructID_Reset_Start <= aSID;
  }

  static PRUint32 GetBitForSID(const nsStyleStructID aSID) {
    return 1 << aSID;
  }

  void* NS_FASTCALL GetStyleData(const nsStyleStructID aSID) {
    if (IsReset(aSID)) {
      if (mResetData) {
        return mResetData->mStyleStructs[aSID];
      }
    } else {
      if (mInheritedData) {
        return mInheritedData->mStyleStructs[aSID];
      }
    }
    return nsnull;
  }

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)       \
    nsStyle##name_ * NS_FASTCALL GetStyle##name_ () {                    \
      return mInheritedData ? static_cast<nsStyle##name_*>(              \
        mInheritedData->mStyleStructs[eStyleStruct_##name_]) : nsnull;   \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)           \
    nsStyle##name_ * NS_FASTCALL GetStyle##name_ () {                    \
      return mResetData ? static_cast<nsStyle##name_*>(                  \
        mResetData->mStyleStructs[eStyleStruct_##name_]) : nsnull;       \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  void Destroy(PRUint32 aBits, nsPresContext* aContext) {
    if (mResetData)
      mResetData->Destroy(aBits, aContext);
    if (mInheritedData)
      mInheritedData->Destroy(aBits, aContext);
    mResetData = nsnull;
    mInheritedData = nsnull;
  }

  nsCachedStyleData() :mInheritedData(nsnull), mResetData(nsnull) {}
  ~nsCachedStyleData() {}
};












































class nsRuleNode {
public:
  enum RuleDetail {
    eRuleNone, 
    eRulePartialReset, 
                       
                       
                       
    eRulePartialMixed, 
                       
                       
                       
    eRulePartialInherited, 
                           
                           
    eRuleFullReset, 
                    
    eRuleFullMixed, 
                    
    eRuleFullInherited  
                        
  };

private:
  nsPresContext* mPresContext; 

  nsRuleNode* mParent; 
                       
                       
                       
                       
  nsIStyleRule* mRule; 

  nsRuleNode* mNextSibling; 
                            
                            
                            
                            

  struct Key {
    nsIStyleRule* mRule;
    PRUint8 mLevel;
    bool mIsImportantRule;

    Key(nsIStyleRule* aRule, PRUint8 aLevel, bool aIsImportantRule)
      : mRule(aRule), mLevel(aLevel), mIsImportantRule(aIsImportantRule)
    {}

    bool operator==(const Key& aOther) const
    {
      return mRule == aOther.mRule &&
             mLevel == aOther.mLevel &&
             mIsImportantRule == aOther.mIsImportantRule;
    }

    bool operator!=(const Key& aOther) const
    {
      return !(*this == aOther);
    }
  };

  static PLDHashNumber
  ChildrenHashHashKey(PLDHashTable *aTable, const void *aKey);

  static bool
  ChildrenHashMatchEntry(PLDHashTable *aTable,
                         const PLDHashEntryHdr *aHdr,
                         const void *aKey);

  static PLDHashTableOps ChildrenHashOps;

  static PLDHashOperator
  EnqueueRuleNodeChildren(PLDHashTable *table, PLDHashEntryHdr *hdr,
                          PRUint32 number, void *arg);

  Key GetKey() const {
    return Key(mRule, GetLevel(), IsImportantRule());
  }

  
  
  
  

  union {
    void* asVoid;
    nsRuleNode* asList;
    PLDHashTable* asHash;
  } mChildren; 

  enum {
    kTypeMask = 0x1,
    kListType = 0x0,
    kHashType = 0x1
  };
  enum {
    
    
    kMaxChildrenInList = 32
  };

  bool HaveChildren() const {
    return mChildren.asVoid != nsnull;
  }
  bool ChildrenAreHashed() {
    return (intptr_t(mChildren.asVoid) & kTypeMask) == kHashType;
  }
  nsRuleNode* ChildrenList() {
    return mChildren.asList;
  }
  nsRuleNode** ChildrenListPtr() {
    return &mChildren.asList;
  }
  PLDHashTable* ChildrenHash() {
    return (PLDHashTable*) (intptr_t(mChildren.asHash) & ~intptr_t(kTypeMask));
  }
  void SetChildrenList(nsRuleNode *aList) {
    NS_ASSERTION(!(intptr_t(aList) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildren.asList = aList;
  }
  void SetChildrenHash(PLDHashTable *aHashtable) {
    NS_ASSERTION(!(intptr_t(aHashtable) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildren.asHash = (PLDHashTable*)(intptr_t(aHashtable) | kHashType);
  }
  void ConvertChildrenToHash();

  nsCachedStyleData mStyleData;   

  PRUint32 mDependentBits; 
                           

  PRUint32 mNoneBits; 
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      

  
  
  
  
  
  
  
  
  
  
  PRUint32 mRefCnt;

public:
  
  
  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy() { DestroyInternal(nsnull); }

  
  inline void AddRef();

  
  inline void Release();

protected:
  void DestroyInternal(nsRuleNode ***aDestroyQueueTail);
  void PropagateDependentBit(PRUint32 aBit, nsRuleNode* aHighestNode);
  void PropagateNoneBit(PRUint32 aBit, nsRuleNode* aHighestNode);

  const void* SetDefaultOnRoot(const nsStyleStructID aSID,
                               nsStyleContext* aContext);

  const void*
    WalkRuleTree(const nsStyleStructID aSID, nsStyleContext* aContext);

  const void*
    ComputeDisplayData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const bool aCanStoreInRuleTree);

  const void*
    ComputeVisibilityData(void* aStartStruct,
                          const nsRuleData* aRuleData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const bool aCanStoreInRuleTree);

  const void*
    ComputeFontData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const bool aCanStoreInRuleTree);

  const void*
    ComputeColorData(void* aStartStruct,
                     const nsRuleData* aRuleData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const bool aCanStoreInRuleTree);

  const void*
    ComputeBackgroundData(void* aStartStruct,
                          const nsRuleData* aRuleData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const bool aCanStoreInRuleTree);

  const void*
    ComputeMarginData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const bool aCanStoreInRuleTree);

  const void*
    ComputeBorderData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const bool aCanStoreInRuleTree);

  const void*
    ComputePaddingData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const bool aCanStoreInRuleTree);

  const void*
    ComputeOutlineData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const bool aCanStoreInRuleTree);

  const void*
    ComputeListData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const bool aCanStoreInRuleTree);

  const void*
    ComputePositionData(void* aStartStruct,
                        const nsRuleData* aRuleData,
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const bool aCanStoreInRuleTree);

  const void*
    ComputeTableData(void* aStartStruct,
                     const nsRuleData* aRuleData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const bool aCanStoreInRuleTree);

  const void*
    ComputeTableBorderData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext, nsRuleNode* aHighestNode,
                           RuleDetail aRuleDetail,
                           const bool aCanStoreInRuleTree);

  const void*
    ComputeContentData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const bool aCanStoreInRuleTree);

  const void*
    ComputeQuotesData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const bool aCanStoreInRuleTree);

  const void*
    ComputeTextData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const bool aCanStoreInRuleTree);

  const void*
    ComputeTextResetData(void* aStartStruct,
                         const nsRuleData* aRuleData,
                         nsStyleContext* aContext, nsRuleNode* aHighestNode,
                         RuleDetail aRuleDetail,
                         const bool aCanStoreInRuleTree);

  const void*
    ComputeUserInterfaceData(void* aStartStruct,
                             const nsRuleData* aRuleData,
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             RuleDetail aRuleDetail,
                             const bool aCanStoreInRuleTree);

  const void*
    ComputeUIResetData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const bool aCanStoreInRuleTree);

  const void*
    ComputeXULData(void* aStartStruct,
                   const nsRuleData* aRuleData,
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const bool aCanStoreInRuleTree);

  const void*
    ComputeColumnData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const bool aCanStoreInRuleTree);

  const void*
    ComputeSVGData(void* aStartStruct,
                   const nsRuleData* aRuleData,
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const bool aCanStoreInRuleTree);

  const void*
    ComputeSVGResetData(void* aStartStruct,
                        const nsRuleData* aRuleData,
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const bool aCanStoreInRuleTree);

  
  static void SetFontSize(nsPresContext* aPresContext,
                          const nsRuleData* aRuleData,
                          const nsStyleFont* aFont,
                          const nsStyleFont* aParentFont,
                          nscoord* aSize,
                          const nsFont& aSystemFont,
                          nscoord aParentSize,
                          nscoord aScriptLevelAdjustedParentSize,
                          bool aUsedStartStruct,
                          bool aAtRoot,
                          bool& aCanStoreInRuleTree);

  static void SetFont(nsPresContext* aPresContext,
                      nsStyleContext* aContext,
                      PRUint8 aGenericFontID,
                      const nsRuleData* aRuleData,
                      const nsStyleFont* aParentFont,
                      nsStyleFont* aFont,
                      bool aStartStruct,
                      bool& aCanStoreInRuleTree);

  static void SetGenericFont(nsPresContext* aPresContext,
                             nsStyleContext* aContext,
                             PRUint8 aGenericFontID,
                             nsStyleFont* aFont);

  void AdjustLogicalBoxProp(nsStyleContext* aContext,
                            const nsCSSValue& aLTRSource,
                            const nsCSSValue& aRTLSource,
                            const nsCSSValue& aLTRLogicalValue,
                            const nsCSSValue& aRTLLogicalValue,
                            mozilla::css::Side aSide,
                            nsCSSRect& aValueRect,
                            bool& aCanStoreInRuleTree);

  inline RuleDetail CheckSpecifiedProperties(const nsStyleStructID aSID,
                                             const nsRuleData* aRuleData);

  const void* GetParentData(const nsStyleStructID aSID);
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)  \
    const nsStyle##name_* GetParent##name_();
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  already_AddRefed<nsCSSShadowArray>
              GetShadowData(const nsCSSValueList* aList,
                            nsStyleContext* aContext,
                            bool aIsBoxShadow,
                            bool& inherited);

private:
  nsRuleNode(nsPresContext* aPresContext, nsRuleNode* aParent,
             nsIStyleRule* aRule, PRUint8 aLevel, bool aIsImportant);
  ~nsRuleNode();

public:
  static nsRuleNode* CreateRootNode(nsPresContext* aPresContext);

  
  nsRuleNode* Transition(nsIStyleRule* aRule, PRUint8 aLevel,
                         bool aIsImportantRule);
  nsRuleNode* GetParent() const { return mParent; }
  bool IsRoot() const { return mParent == nsnull; }

  
  PRUint8 GetLevel() const {
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_LEVEL_MASK) >>
             NS_RULE_NODE_LEVEL_SHIFT;
  }
  bool IsImportantRule() const {
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_IS_IMPORTANT) != 0;
  }

  
  nsIStyleRule* GetRule() const { return mRule; }
  
  nsPresContext* GetPresContext() const { return mPresContext; }

  const void* GetStyleData(nsStyleStructID aSID,
                           nsStyleContext* aContext,
                           bool aComputeData);

  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    const nsStyle##name_* GetStyle##name_(nsStyleContext* aContext,           \
                                          bool aComputeData);
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  





  void Mark();
  bool Sweep();

  static bool
    HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                            PRUint32 ruleTypeMask,
                            bool aAuthorColorsAllowed);

  
  static nscoord CalcLengthWithInitialFont(nsPresContext* aPresContext,
                                           const nsCSSValue& aValue);
  
  static nscoord CalcLength(const nsCSSValue& aValue,
                            nsStyleContext* aStyleContext,
                            nsPresContext* aPresContext,
                            bool& aCanStoreInRuleTree);

  struct ComputedCalc {
    nscoord mLength;
    float mPercent;

    ComputedCalc(nscoord aLength, float aPercent)
      : mLength(aLength), mPercent(aPercent) {}
  };
  static ComputedCalc
  SpecifiedCalcToComputedCalc(const nsCSSValue& aValue,
                              nsStyleContext* aStyleContext,
                              nsPresContext* aPresContext,
                              bool& aCanStoreInRuleTree);

  
  
  
  static nscoord ComputeComputedCalc(const nsStyleCoord& aCoord,
                                     nscoord aPercentageBasis);

  
  
  static nscoord ComputeCoordPercentCalc(const nsStyleCoord& aCoord,
                                         nscoord aPercentageBasis);

  
  
  
  
  bool TreeHasCachedData() const {
    NS_ASSERTION(IsRoot(), "should only be called on root of rule tree");
    return HaveChildren() || mStyleData.mInheritedData || mStyleData.mResetData;
  }

  bool NodeHasCachedData(const nsStyleStructID aSID) {
    return !!mStyleData.GetStyleData(aSID);
  }

  static void ComputeFontFeatures(const nsCSSValuePairList *aFeaturesList,
                                  nsTArray<gfxFontFeature>& aFeatureSettings);
};

#endif
