









#ifndef nsRuleNode_h___
#define nsRuleNode_h___

#include "mozilla/PodOperations.h"
#include "mozilla/RangedArray.h"
#include "mozilla/RuleNodeCacheConditions.h"
#include "nsPresContext.h"
#include "nsStyleStruct.h"

class nsCSSPropertySet;
class nsCSSValue;
class nsIStyleRule;
class nsStyleContext;
class nsStyleCoord;
struct nsCSSRect;
struct nsCSSValueList;
struct nsCSSValuePairList;
struct nsRuleData;

struct nsInheritedStyleData
{
  mozilla::RangedArray<void*,
                       nsStyleStructID_Inherited_Start,
                       nsStyleStructID_Inherited_Count> mStyleStructs;

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsInheritedStyleData_id, sz);
  }

  void DestroyStructs(uint64_t aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_INHERITED(name, checkdata_cb) \
    void *name##Data = mStyleStructs[eStyleStruct_##name]; \
    if (name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      static_cast<nsStyle##name*>(name##Data)->Destroy(aContext);
#define STYLE_STRUCT_RESET(name, checkdata_cb)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
  }

  void Destroy(uint64_t aBits, nsPresContext* aContext) {
    DestroyStructs(aBits, aContext);
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsInheritedStyleData_id, this);
  }

  nsInheritedStyleData() {
    for (nsStyleStructID i = nsStyleStructID_Inherited_Start;
         i < nsStyleStructID_Inherited_Start + nsStyleStructID_Inherited_Count;
         i = nsStyleStructID(i + 1)) {
      mStyleStructs[i] = nullptr;
    }
  }
};

struct nsResetStyleData
{
  mozilla::RangedArray<void*,
                       nsStyleStructID_Reset_Start,
                       nsStyleStructID_Reset_Count> mStyleStructs;

  nsResetStyleData()
  {
    for (nsStyleStructID i = nsStyleStructID_Reset_Start;
         i < nsStyleStructID_Reset_Start + nsStyleStructID_Reset_Count;
         i = nsStyleStructID(i + 1)) {
      mStyleStructs[i] = nullptr;
    }
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsResetStyleData_id, sz);
  }

  void Destroy(uint64_t aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_RESET(name, checkdata_cb) \
    void *name##Data = mStyleStructs[eStyleStruct_##name]; \
    if (name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      static_cast<nsStyle##name*>(name##Data)->Destroy(aContext);
#define STYLE_STRUCT_INHERITED(name, checkdata_cb)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED

    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsResetStyleData_id, this);
  }
};

struct nsConditionalResetStyleData
{
  static uint32_t GetBitForSID(const nsStyleStructID aSID) {
    return 1 << aSID;
  }

  struct Entry
  {
    Entry(const mozilla::RuleNodeCacheConditions& aConditions,
          void* aStyleStruct,
          Entry* aNext)
      : mConditions(aConditions), mStyleStruct(aStyleStruct), mNext(aNext) {}

    void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
      return aContext->PresShell()->
        AllocateByObjectID(nsPresArena::nsConditionalResetStyleDataEntry_id, sz);
    }

    const mozilla::RuleNodeCacheConditions mConditions;
    void* const mStyleStruct;
    Entry* const mNext;
  };

  
  
  
  mozilla::RangedArray<void*,
                       nsStyleStructID_Reset_Start,
                       nsStyleStructID_Reset_Count> mEntries;

  uint32_t mConditionalBits;

  nsConditionalResetStyleData()
  {
    for (nsStyleStructID i = nsStyleStructID_Reset_Start;
         i < nsStyleStructID_Reset_Start + nsStyleStructID_Reset_Count;
         i = nsStyleStructID(i + 1)) {
      mEntries[i] = nullptr;
    }
    mConditionalBits = 0;
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsConditionalResetStyleData_id, sz);
  }

  void* GetStyleData(nsStyleStructID aSID) const {
    if (mConditionalBits & GetBitForSID(aSID)) {
      return nullptr;
    }
    return mEntries[aSID];
  }

  void* GetStyleData(nsStyleStructID aSID,
                     nsStyleContext* aStyleContext) const {
    if (!(mConditionalBits & GetBitForSID(aSID))) {
      return mEntries[aSID];
    }
    Entry* e = static_cast<Entry*>(mEntries[aSID]);
    MOZ_ASSERT(e, "if mConditionalBits bit is set, we must have at least one "
                  "conditional style struct");
    do {
      if (e->mConditions.Matches(aStyleContext)) {
        return e->mStyleStruct;
      }
      e = e->mNext;
    } while (e);
    return nullptr;
  }

  void SetStyleData(nsStyleStructID aSID, void* aStyleStruct) {
    MOZ_ASSERT(!(mConditionalBits & GetBitForSID(aSID)),
               "rule node should not have unconditional and conditional style "
               "data for a given struct");
    mEntries[aSID] = aStyleStruct;
  }

  void SetStyleData(nsStyleStructID aSID,
                    nsPresContext* aPresContext,
                    void* aStyleStruct,
                    const mozilla::RuleNodeCacheConditions& aConditions) {
    MOZ_ASSERT((mConditionalBits & GetBitForSID(aSID)) ||
               !mEntries[aSID],
               "rule node should not have unconditional and conditional style "
               "data for a given struct");
    MOZ_ASSERT(aConditions.CacheableWithDependencies(),
               "don't call SetStyleData with a cache key that has no "
               "conditions or is uncacheable");
#ifdef DEBUG
    for (Entry* e = static_cast<Entry*>(mEntries[aSID]); e; e = e->mNext) {
      NS_WARN_IF_FALSE(e->mConditions != aConditions,
                       "wasteful to have duplicate conditional style data");
    }
#endif

    mConditionalBits |= GetBitForSID(aSID);
    mEntries[aSID] =
      new (aPresContext) Entry(aConditions, aStyleStruct,
                               static_cast<Entry*>(mEntries[aSID]));
  }

  void Destroy(uint64_t aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_RESET(name, checkdata_cb)                                 \
    void* name##Ptr = mEntries[eStyleStruct_##name];                           \
    if (name##Ptr) {                                                           \
      if (!(mConditionalBits & NS_STYLE_INHERIT_BIT(name))) {                  \
        if (!(aBits & NS_STYLE_INHERIT_BIT(name))) {                           \
          static_cast<nsStyle##name*>(name##Ptr)->Destroy(aContext);           \
        }                                                                      \
      } else {                                                                 \
        Entry* e = static_cast<Entry*>(name##Ptr);                             \
        MOZ_ASSERT(e, "if mConditionalBits bit is set, we must have at least " \
                      "one conditional style struct");                         \
        do {                                                                   \
          static_cast<nsStyle##name*>(e->mStyleStruct)->Destroy(aContext);     \
          Entry* next = e->mNext;                                              \
          aContext->PresShell()->FreeByObjectID(                               \
              nsPresArena::nsConditionalResetStyleDataEntry_id, e);            \
          e = next;                                                            \
        } while (e);                                                           \
      }                                                                        \
    }
#define STYLE_STRUCT_INHERITED(name, checkdata_cb)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED

    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsConditionalResetStyleData_id, this);
  }

};

struct nsCachedStyleData
{
  nsInheritedStyleData* mInheritedData;
  nsConditionalResetStyleData* mResetData;

  static bool IsReset(const nsStyleStructID aSID) {
    MOZ_ASSERT(0 <= aSID && aSID < nsStyleStructID_Length,
               "must be an inherited or reset SID");
    return nsStyleStructID_Reset_Start <= aSID;
  }

  static bool IsInherited(const nsStyleStructID aSID) {
    return !IsReset(aSID);
  }

  static uint32_t GetBitForSID(const nsStyleStructID aSID) {
    return nsConditionalResetStyleData::GetBitForSID(aSID);
  }

  void* NS_FASTCALL GetStyleData(const nsStyleStructID aSID) {
    if (IsReset(aSID)) {
      if (mResetData) {
        return mResetData->GetStyleData(aSID);
      }
    } else {
      if (mInheritedData) {
        return mInheritedData->mStyleStructs[aSID];
      }
    }
    return nullptr;
  }

  void* NS_FASTCALL GetStyleData(const nsStyleStructID aSID,
                                 nsStyleContext* aStyleContext) {
    if (IsReset(aSID)) {
      if (mResetData) {
        return mResetData->GetStyleData(aSID, aStyleContext);
      }
    } else {
      if (mInheritedData) {
        return mInheritedData->mStyleStructs[aSID];
      }
    }
    return nullptr;
  }

  void NS_FASTCALL SetStyleData(const nsStyleStructID aSID,
                                nsPresContext *aPresContext, void *aData) {
    if (IsReset(aSID)) {
      if (!mResetData) {
        mResetData = new (aPresContext) nsConditionalResetStyleData;
      }
      mResetData->SetStyleData(aSID, aData);
    } else {
      if (!mInheritedData) {
        mInheritedData = new (aPresContext) nsInheritedStyleData;
      }
      mInheritedData->mStyleStructs[aSID] = aData;
    }
  }

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)                         \
    nsStyle##name_ * NS_FASTCALL GetStyle##name_ () {                          \
      return mInheritedData ? static_cast<nsStyle##name_*>(                    \
        mInheritedData->mStyleStructs[eStyleStruct_##name_]) : nullptr;        \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_)                             \
    nsStyle##name_ * NS_FASTCALL GetStyle##name_ (nsStyleContext* aContext) {  \
      return mResetData ? static_cast<nsStyle##name_*>(                        \
        mResetData->GetStyleData(eStyleStruct_##name_, aContext)) : nullptr;   \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  void Destroy(uint64_t aBits, nsPresContext* aContext) {
    if (mResetData)
      mResetData->Destroy(aBits, aContext);
    if (mInheritedData)
      mInheritedData->Destroy(aBits, aContext);
    mResetData = nullptr;
    mInheritedData = nullptr;
  }

  nsCachedStyleData() :mInheritedData(nullptr), mResetData(nullptr) {}
  ~nsCachedStyleData() {}
};












































enum nsFontSizeType {
  eFontSize_HTML = 1,
  eFontSize_CSS = 2
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
  nsPresContext* const mPresContext; 

  nsRuleNode* const mParent; 
                             
                             
                             
                             
  nsIStyleRule* const mRule; 

  nsRuleNode* mNextSibling; 
                            
                            
                            
                            

  struct Key {
    nsIStyleRule* mRule;
    uint8_t mLevel;
    bool mIsImportantRule;

    Key(nsIStyleRule* aRule, uint8_t aLevel, bool aIsImportantRule)
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

  static PLDHashOperator
  SweepHashEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                 uint32_t number, void *arg);
  void SweepChildren(nsTArray<nsRuleNode*>& aSweepQueue);
  bool DestroyIfNotMarked();

  static const PLDHashTableOps ChildrenHashOps;

  static PLDHashOperator
  EnqueueRuleNodeChildren(PLDHashTable *table, PLDHashEntryHdr *hdr,
                          uint32_t number, void *arg);

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
    return mChildren.asVoid != nullptr;
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
  void ConvertChildrenToHash(int32_t aNumKids);

  nsCachedStyleData mStyleData;   

  uint32_t mDependentBits; 
                           

  uint32_t mNoneBits; 
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      

  
  
  
  
  
  
  
  
  
  
  uint32_t mRefCnt;

public:
  
  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy() { DestroyInternal(nullptr); }

  
  inline void AddRef();

  
  inline void Release();

protected:
  void DestroyInternal(nsRuleNode ***aDestroyQueueTail);
  void PropagateDependentBit(nsStyleStructID aSID, nsRuleNode* aHighestNode,
                             void* aStruct);
  void PropagateNoneBit(uint32_t aBit, nsRuleNode* aHighestNode);
  static void PropagateGrandancestorBit(nsStyleContext* aContext,
                                        nsStyleContext* aContextInheritedFrom);

  const void* SetDefaultOnRoot(const nsStyleStructID aSID,
                               nsStyleContext* aContext);

  







  static bool ResolveVariableReferences(const nsStyleStructID aSID,
                                        nsRuleData* aRuleData,
                                        nsStyleContext* aContext);

  const void*
    WalkRuleTree(const nsStyleStructID aSID, nsStyleContext* aContext);

  const void*
    ComputeDisplayData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeVisibilityData(void* aStartStruct,
                          const nsRuleData* aRuleData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeFontData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeColorData(void* aStartStruct,
                     const nsRuleData* aRuleData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeBackgroundData(void* aStartStruct,
                          const nsRuleData* aRuleData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeMarginData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeBorderData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputePaddingData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeOutlineData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeListData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputePositionData(void* aStartStruct,
                        const nsRuleData* aRuleData,
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeTableData(void* aStartStruct,
                     const nsRuleData* aRuleData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeTableBorderData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext, nsRuleNode* aHighestNode,
                           RuleDetail aRuleDetail,
                           const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeContentData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeQuotesData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeTextData(void* aStartStruct,
                    const nsRuleData* aRuleData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeTextResetData(void* aStartStruct,
                         const nsRuleData* aRuleData,
                         nsStyleContext* aContext, nsRuleNode* aHighestNode,
                         RuleDetail aRuleDetail,
                         const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeUserInterfaceData(void* aStartStruct,
                             const nsRuleData* aRuleData,
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             RuleDetail aRuleDetail,
                             const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeUIResetData(void* aStartStruct,
                       const nsRuleData* aRuleData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeXULData(void* aStartStruct,
                   const nsRuleData* aRuleData,
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeColumnData(void* aStartStruct,
                      const nsRuleData* aRuleData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeSVGData(void* aStartStruct,
                   const nsRuleData* aRuleData,
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeSVGResetData(void* aStartStruct,
                        const nsRuleData* aRuleData,
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const mozilla::RuleNodeCacheConditions aConditions);

  const void*
    ComputeVariablesData(void* aStartStruct,
                         const nsRuleData* aRuleData,
                         nsStyleContext* aContext, nsRuleNode* aHighestNode,
                         RuleDetail aRuleDetail,
                         const mozilla::RuleNodeCacheConditions aConditions);

  
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
                          mozilla::RuleNodeCacheConditions& aConditions);

  static void SetFont(nsPresContext* aPresContext,
                      nsStyleContext* aContext,
                      uint8_t aGenericFontID,
                      const nsRuleData* aRuleData,
                      const nsStyleFont* aParentFont,
                      nsStyleFont* aFont,
                      bool aStartStruct,
                      mozilla::RuleNodeCacheConditions& aConditions);

  static void SetGenericFont(nsPresContext* aPresContext,
                             nsStyleContext* aContext,
                             uint8_t aGenericFontID,
                             nsStyleFont* aFont);

  inline RuleDetail CheckSpecifiedProperties(const nsStyleStructID aSID,
                                             const nsRuleData* aRuleData);

  already_AddRefed<nsCSSShadowArray>
              GetShadowData(const nsCSSValueList* aList,
                            nsStyleContext* aContext,
                            bool aIsBoxShadow,
                            mozilla::RuleNodeCacheConditions& aConditions);
  bool SetStyleFilterToCSSValue(nsStyleFilter* aStyleFilter,
                                const nsCSSValue& aValue,
                                nsStyleContext* aStyleContext,
                                nsPresContext* aPresContext,
                                mozilla::RuleNodeCacheConditions& aConditions);
  void SetStyleClipPathToCSSValue(nsStyleClipPath* aStyleClipPath,
                                  const nsCSSValue* aValue,
                                  nsStyleContext* aStyleContext,
                                  nsPresContext* aPresContext,
                                  mozilla::RuleNodeCacheConditions& aConditions);

private:
  nsRuleNode(nsPresContext* aPresContext, nsRuleNode* aParent,
             nsIStyleRule* aRule, uint8_t aLevel, bool aIsImportant);
  ~nsRuleNode();

public:
  
  static nsRuleNode* CreateRootNode(nsPresContext* aPresContext);

  static void EnsureBlockDisplay(uint8_t& display,
                                 bool aConvertListItem = false);
  static void EnsureInlineDisplay(uint8_t& display);

  
  nsRuleNode* Transition(nsIStyleRule* aRule, uint8_t aLevel,
                         bool aIsImportantRule);
  nsRuleNode* GetParent() const { return mParent; }
  bool IsRoot() const { return mParent == nullptr; }

  
  nsRuleNode* RuleTree();
  const nsRuleNode* RuleTree() const {
    return const_cast<nsRuleNode*>(this)->RuleTree();
  }

  
  uint8_t GetLevel() const {
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_LEVEL_MASK) >>
             NS_RULE_NODE_LEVEL_SHIFT;
  }
  bool IsImportantRule() const {
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_IS_IMPORTANT) != 0;
  }

  




  void SetUsedDirectly();
  bool IsUsedDirectly() const {
    return (mDependentBits & NS_RULE_NODE_USED_DIRECTLY) != 0;
  }

  
  nsIStyleRule* GetRule() const { return mRule; }
  
  nsPresContext* PresContext() const { return mPresContext; }

  const void* GetStyleData(nsStyleStructID aSID,
                           nsStyleContext* aContext,
                           bool aComputeData);


  
  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)                        \
  template<bool aComputeData>                                                 \
  const nsStyle##name_*                                                       \
  GetStyle##name_(nsStyleContext* aContext)                                   \
  {                                                                           \
    NS_ASSERTION(IsUsedDirectly(),                                            \
                 "if we ever call this on rule nodes that aren't used "       \
                 "directly, we should adjust handling of mDependentBits "     \
                 "in some way.");                                             \
                                                                              \
    const nsStyle##name_ *data;                                               \
    data = mStyleData.GetStyle##name_();                                      \
    if (MOZ_LIKELY(data != nullptr))                                          \
      return data;                                                            \
                                                                              \
    if (!aComputeData)                                                        \
      return nullptr;                                                         \
                                                                              \
    data = static_cast<const nsStyle##name_ *>                                \
             (WalkRuleTree(eStyleStruct_##name_, aContext));                  \
                                                                              \
    MOZ_ASSERT(data, "should have aborted on out-of-memory");                 \
    return data;                                                              \
  }

  #define STYLE_STRUCT_RESET(name_, checkdata_cb_)                            \
  template<bool aComputeData>                                                 \
  const nsStyle##name_*                                                       \
  GetStyle##name_(nsStyleContext* aContext)                                   \
  {                                                                           \
    NS_ASSERTION(IsUsedDirectly(),                                            \
                 "if we ever call this on rule nodes that aren't used "       \
                 "directly, we should adjust handling of mDependentBits "     \
                 "in some way.");                                             \
                                                                              \
    const nsStyle##name_ *data;                                               \
    data = mStyleData.GetStyle##name_(aContext);                              \
    if (MOZ_LIKELY(data != nullptr))                                          \
      return data;                                                            \
                                                                              \
    if (!aComputeData)                                                        \
      return nullptr;                                                         \
                                                                              \
    data = static_cast<const nsStyle##name_ *>                                \
             (WalkRuleTree(eStyleStruct_##name_, aContext));                  \
                                                                              \
    MOZ_ASSERT(data, "should have aborted on out-of-memory");                 \
    return data;                                                              \
  }

  #include "nsStyleStructList.h"

  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  







  void Mark();
  bool Sweep();

  static bool
    HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                            uint32_t ruleTypeMask,
                            bool aAuthorColorsAllowed);

  




  static void
  ComputePropertiesOverridingAnimation(
                              const nsTArray<nsCSSProperty>& aProperties,
                              nsStyleContext* aStyleContext,
                              nsCSSPropertySet& aPropertiesOverridden);

  
  static nscoord CalcLengthWithInitialFont(nsPresContext* aPresContext,
                                           const nsCSSValue& aValue);
  
  static nscoord CalcLength(const nsCSSValue& aValue,
                            nsStyleContext* aStyleContext,
                            nsPresContext* aPresContext,
                            mozilla::RuleNodeCacheConditions& aConditions);

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
                              mozilla::RuleNodeCacheConditions& aConditions);

  
  
  
  static nscoord ComputeComputedCalc(const nsStyleCoord& aCoord,
                                     nscoord aPercentageBasis);

  
  
  static nscoord ComputeCoordPercentCalc(const nsStyleCoord& aCoord,
                                         nscoord aPercentageBasis);

  
  
  
  
  bool TreeHasCachedData() const {
    NS_ASSERTION(IsRoot(), "should only be called on root of rule tree");
    return HaveChildren() || mStyleData.mInheritedData || mStyleData.mResetData;
  }

  
  
  bool NodeHasCachedUnconditionalData(const nsStyleStructID aSID) {
    return !!mStyleData.GetStyleData(aSID);
  }

  static void ComputeFontFeatures(const nsCSSValuePairList *aFeaturesList,
                                  nsTArray<gfxFontFeature>& aFeatureSettings);

  static nscoord CalcFontPointSize(int32_t aHTMLSize, int32_t aBasePointSize, 
                                   nsPresContext* aPresContext,
                                   nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextSmallerFontSize(nscoord aFontSize, int32_t aBasePointSize, 
                                         nsPresContext* aPresContext,
                                         nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextLargerFontSize(nscoord aFontSize, int32_t aBasePointSize, 
                                        nsPresContext* aPresContext,
                                        nsFontSizeType aFontSizeType = eFontSize_HTML);

  












  static bool ComputeColor(const nsCSSValue& aValue,
                           nsPresContext* aPresContext,
                           nsStyleContext* aStyleContext,
                           nscolor& aResult);
};

#endif
