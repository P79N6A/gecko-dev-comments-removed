











































#ifndef nsRuleNode_h___
#define nsRuleNode_h___

#include "nsPresContext.h"
#include "nsStyleStruct.h"

class nsStyleContext;
struct PLDHashTable;
class nsILanguageAtomService;
struct nsRuleData;
class nsIStyleRule;
struct nsCSSStruct;
struct nsCSSValueList;

typedef nsCSSStruct nsRuleDataStruct;

struct nsRuleDataFont;
class nsCSSValue;
struct nsCSSRect;

struct nsInheritedStyleData
{

#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
  nsStyle##name * m##name##Data;
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }

  void ClearInheritedData(PRUint32 aBits) {
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
    if (m##name##Data && (aBits & NS_STYLE_INHERIT_BIT(name))) \
      m##name##Data = nsnull;
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
  }

  void Destroy(PRUint32 aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
    if (m##name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      m##name##Data->Destroy(aContext);
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

    aContext->FreeToShell(sizeof(nsInheritedStyleData), this);
  }

  nsInheritedStyleData() {
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
    m##name##Data = nsnull;
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

  }
};

struct nsResetStyleData
{
  nsResetStyleData()
  {
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
    m##name##Data = nsnull;
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }

  void ClearInheritedData(PRUint32 aBits) {
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
    if (m##name##Data && (aBits & NS_STYLE_INHERIT_BIT(name))) \
      m##name##Data = nsnull;
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED
  }

  void Destroy(PRUint32 aBits, nsPresContext* aContext) {
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
    if (m##name##Data && !(aBits & NS_STYLE_INHERIT_BIT(name))) \
      m##name##Data->Destroy(aContext);
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED

    aContext->FreeToShell(sizeof(nsResetStyleData), this);
  }

#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
  nsStyle##name * m##name##Data;
#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_RESET
#undef STYLE_STRUCT_INHERITED

};

struct nsCachedStyleData
{
  struct StyleStructInfo {
    ptrdiff_t mCachedStyleDataOffset;
    ptrdiff_t mInheritResetOffset;
    PRBool    mIsReset;
  };

  static StyleStructInfo gInfo[];

  nsInheritedStyleData* mInheritedData;
  nsResetStyleData* mResetData;

  static PRBool IsReset(const nsStyleStructID& aSID) {
    return gInfo[aSID].mIsReset;
  }

  static PRUint32 GetBitForSID(const nsStyleStructID& aSID) {
    return 1 << aSID;
  }

  NS_HIDDEN_(void*) NS_FASTCALL GetStyleData(const nsStyleStructID& aSID) {
    
    
    
    
    
    
    
    
    

    

    const StyleStructInfo& info = gInfo[aSID];

    
    char* resetOrInheritSlot = reinterpret_cast<char*>(this) + info.mCachedStyleDataOffset;

    
    char* resetOrInherit = reinterpret_cast<char*>(*reinterpret_cast<void**>(resetOrInheritSlot));

    void* data = nsnull;
    if (resetOrInherit) {
      
      
      char* dataSlot = resetOrInherit + info.mInheritResetOffset;
      data = *reinterpret_cast<void**>(dataSlot);
    }
    return data;
  }

  
  #define STYLE_STRUCT_INHERITED(name_, checkdata_cb_, ctor_args_)       \
    NS_HIDDEN_(nsStyle##name_ *) NS_FASTCALL GetStyle##name_ () {        \
      return mInheritedData ? mInheritedData->m##name_##Data : nsnull;   \
    }
  #define STYLE_STRUCT_RESET(name_, checkdata_cb_, ctor_args_)           \
    NS_HIDDEN_(nsStyle##name_ *) NS_FASTCALL GetStyle##name_ () {        \
      return mResetData ? mResetData->m##name_##Data : nsnull;           \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT_RESET
  #undef STYLE_STRUCT_INHERITED

  NS_HIDDEN_(void) ClearInheritedData(PRUint32 aBits) {
    if (mResetData)
      mResetData->ClearInheritedData(aBits);
    if (mInheritedData)
      mInheritedData->ClearInheritedData(aBits);
  }

  NS_HIDDEN_(void) Destroy(PRUint32 aBits, nsPresContext* aContext) {
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
    PRPackedBool mIsImportantRule;

    Key(nsIStyleRule* aRule, PRUint8 aLevel, PRPackedBool aIsImportantRule)
      : mRule(aRule), mLevel(aLevel), mIsImportantRule(aIsImportantRule)
    {}

    PRBool operator==(const Key& aOther) const
    {
      return mRule == aOther.mRule &&
             mLevel == aOther.mLevel &&
             mIsImportantRule == aOther.mIsImportantRule;
    }

    PRBool operator!=(const Key& aOther) const
    {
      return !(*this == aOther);
    }
  };

  static PLDHashNumber
  ChildrenHashHashKey(PLDHashTable *aTable, const void *aKey);

  static PRBool
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

  PRBool HaveChildren() const {
    return mChildren.asVoid != nsnull;
  }
  PRBool ChildrenAreHashed() {
    return (PRWord(mChildren.asVoid) & kTypeMask) == kHashType;
  }
  nsRuleNode* ChildrenList() {
    return mChildren.asList;
  }
  nsRuleNode** ChildrenListPtr() {
    return &mChildren.asList;
  }
  PLDHashTable* ChildrenHash() {
    return (PLDHashTable*) (PRWord(mChildren.asHash) & ~PRWord(kTypeMask));
  }
  void SetChildrenList(nsRuleNode *aList) {
    NS_ASSERTION(!(PRWord(aList) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildren.asList = aList;
  }
  void SetChildrenHash(PLDHashTable *aHashtable) {
    NS_ASSERTION(!(PRWord(aHashtable) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildren.asHash = (PLDHashTable*)(PRWord(aHashtable) | kHashType);
  }
  void ConvertChildrenToHash();

  nsCachedStyleData mStyleData;   

  PRUint32 mDependentBits; 
                           

  PRUint32 mNoneBits; 
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      

  
  
  
  PRUint32 mRefCnt;

public:
  
  
  NS_HIDDEN_(void*) operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  NS_HIDDEN_(void) Destroy() { DestroyInternal(nsnull); }
  static NS_HIDDEN_(nsILanguageAtomService*) gLangService;

  
  inline NS_HIDDEN_(void) AddRef();

  
  inline NS_HIDDEN_(void) Release();

protected:
  NS_HIDDEN_(void) DestroyInternal(nsRuleNode ***aDestroyQueueTail);
  NS_HIDDEN_(void) PropagateDependentBit(PRUint32 aBit,
                                         nsRuleNode* aHighestNode);
  NS_HIDDEN_(void) PropagateNoneBit(PRUint32 aBit, nsRuleNode* aHighestNode);
  
  NS_HIDDEN_(const void*) SetDefaultOnRoot(const nsStyleStructID aSID,
                                                 nsStyleContext* aContext);

  NS_HIDDEN_(const void*)
    WalkRuleTree(const nsStyleStructID aSID, nsStyleContext* aContext, 
                 nsRuleData* aRuleData, nsRuleDataStruct* aSpecificData);

  NS_HIDDEN_(const void*)
    ComputeDisplayData(void* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeVisibilityData(void* aStartStruct,
                          const nsRuleDataStruct& aData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeFontData(void* aStartStruct,
                    const nsRuleDataStruct& aData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeColorData(void* aStartStruct,
                     const nsRuleDataStruct& aData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeBackgroundData(void* aStartStruct,
                          const nsRuleDataStruct& aData, 
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          RuleDetail aRuleDetail,
                          const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeMarginData(void* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeBorderData(void* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputePaddingData(void* aStartStruct,
                       const nsRuleDataStruct& aData, 
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeOutlineData(void* aStartStruct,
                       const nsRuleDataStruct& aData, 
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeListData(void* aStartStruct,
                    const nsRuleDataStruct& aData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputePositionData(void* aStartStruct,
                        const nsRuleDataStruct& aData, 
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeTableData(void* aStartStruct,
                     const nsRuleDataStruct& aData, 
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     RuleDetail aRuleDetail,
                     const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeTableBorderData(void* aStartStruct,
                           const nsRuleDataStruct& aData, 
                           nsStyleContext* aContext, nsRuleNode* aHighestNode,
                           RuleDetail aRuleDetail,
                           const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeContentData(void* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeQuotesData(void* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeTextData(void* aStartStruct,
                    const nsRuleDataStruct& aData, 
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    RuleDetail aRuleDetail,
                    const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeTextResetData(void* aStartStruct,
                         const nsRuleDataStruct& aData,
                         nsStyleContext* aContext, nsRuleNode* aHighestNode,
                         RuleDetail aRuleDetail,
                         const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeUserInterfaceData(void* aStartStruct,
                             const nsRuleDataStruct& aData, 
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             RuleDetail aRuleDetail,
                             const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeUIResetData(void* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       RuleDetail aRuleDetail,
                       const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeXULData(void* aStartStruct,
                   const nsRuleDataStruct& aData, 
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeColumnData(void* aStartStruct,
                      const nsRuleDataStruct& aData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      RuleDetail aRuleDetail,
                      const PRBool aCanStoreInRuleTree);

#ifdef MOZ_SVG
  NS_HIDDEN_(const void*)
    ComputeSVGData(void* aStartStruct,
                   const nsRuleDataStruct& aData, 
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   RuleDetail aRuleDetail,
                   const PRBool aCanStoreInRuleTree);

  NS_HIDDEN_(const void*)
    ComputeSVGResetData(void* aStartStruct,
                        const nsRuleDataStruct& aData, 
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        RuleDetail aRuleDetail,
                        const PRBool aCanStoreInRuleTree);
#endif

  
  static NS_HIDDEN_(void) SetFontSize(nsPresContext* aPresContext,
                                      const nsRuleDataFont& aFontData,
                                      const nsStyleFont* aFont,
                                      const nsStyleFont* aParentFont,
                                      nscoord* aSize,
                                      const nsFont& aSystemFont,
                                      nscoord aParentSize,
                                      nscoord aScriptLevelAdjustedParentSize,
                                      PRBool aUsedStartStruct,
                                      PRBool aAtRoot,
                                      PRBool& aCanStoreInRuleTree);

  static NS_HIDDEN_(void) SetFont(nsPresContext* aPresContext,
                                  nsStyleContext* aContext,
                                  nscoord aMinFontSize,
                                  PRUint8 aGenericFontID,
                                  const nsRuleDataFont& aFontData,
                                  const nsStyleFont* aParentFont,
                                  nsStyleFont* aFont,
                                  PRBool aStartStruct,
                                  PRBool& aCanStoreInRuleTree);

  static NS_HIDDEN_(void) SetGenericFont(nsPresContext* aPresContext,
                                         nsStyleContext* aContext,
                                         PRUint8 aGenericFontID,
                                         nscoord aMinFontSize,
                                         nsStyleFont* aFont);

  NS_HIDDEN_(void) AdjustLogicalBoxProp(nsStyleContext* aContext,
                                        const nsCSSValue& aLTRSource,
                                        const nsCSSValue& aRTLSource,
                                        const nsCSSValue& aLTRLogicalValue,
                                        const nsCSSValue& aRTLLogicalValue,
                                        PRUint8 aSide,
                                        nsCSSRect& aValueRect,
                                        PRBool& aCanStoreInRuleTree);

  inline RuleDetail CheckSpecifiedProperties(const nsStyleStructID aSID, const nsRuleDataStruct& aRuleDataStruct);

  NS_HIDDEN_(const void*) GetParentData(const nsStyleStructID aSID);
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)  \
    NS_HIDDEN_(const nsStyle##name_*) GetParent##name_();
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT  

  NS_HIDDEN_(const void*) GetDisplayData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetVisibilityData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetFontData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetColorData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetBackgroundData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetMarginData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetBorderData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetPaddingData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetOutlineData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetListData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetPositionData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetTableData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetTableBorderData(nsStyleContext* aContext);

  NS_HIDDEN_(const void*) GetContentData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetQuotesData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetTextData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetTextResetData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetUserInterfaceData(nsStyleContext* aContext);

  NS_HIDDEN_(const void*) GetUIResetData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetXULData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetColumnData(nsStyleContext* aContext);
#ifdef MOZ_SVG
  NS_HIDDEN_(const void*) GetSVGData(nsStyleContext* aContext);
  NS_HIDDEN_(const void*) GetSVGResetData(nsStyleContext* aContext);
#endif

  NS_HIDDEN_(already_AddRefed<nsCSSShadowArray>)
                          GetShadowData(nsCSSValueList* aList,
                                        nsStyleContext* aContext,
                                        PRBool aIsBoxShadow,
                                        PRBool& inherited);

private:
  nsRuleNode(nsPresContext* aPresContext, nsRuleNode* aParent,
             nsIStyleRule* aRule, PRUint8 aLevel, PRBool aIsImportant)
    NS_HIDDEN;
  ~nsRuleNode() NS_HIDDEN;

public:
  static NS_HIDDEN_(nsRuleNode*) CreateRootNode(nsPresContext* aPresContext);

  
  NS_HIDDEN_(nsRuleNode*) Transition(nsIStyleRule* aRule, PRUint8 aLevel,
                                     PRPackedBool aIsImportantRule);
  nsRuleNode* GetParent() const { return mParent; }
  PRBool IsRoot() const { return mParent == nsnull; }

  
  PRUint8 GetLevel() const { 
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_LEVEL_MASK) >>
             NS_RULE_NODE_LEVEL_SHIFT;
  }
  PRBool IsImportantRule() const {
    NS_ASSERTION(!IsRoot(), "can't call on root");
    return (mDependentBits & NS_RULE_NODE_IS_IMPORTANT) != 0;
  }

  
  nsIStyleRule* GetRule() const { return mRule; }
  
  nsPresContext* GetPresContext() const { return mPresContext; }

  NS_HIDDEN_(const void*) GetStyleData(nsStyleStructID aSID, 
                                       nsStyleContext* aContext,
                                       PRBool aComputeData);

  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    NS_HIDDEN_(const nsStyle##name_*)                                         \
      GetStyle##name_(nsStyleContext* aContext,                               \
                      PRBool aComputeData);
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT  

  





  NS_HIDDEN_(void) Mark();
  NS_HIDDEN_(PRBool) Sweep();

  static PRBool
    HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                            PRUint32 ruleTypeMask,
                            PRBool aAuthorColorsAllowed);

  
  static nscoord CalcLengthWithInitialFont(nsPresContext* aPresContext,
                                           const nsCSSValue& aValue);
  
  static nscoord CalcLength(const nsCSSValue& aValue,
                            nsStyleContext* aStyleContext,
                            nsPresContext* aPresContext,
                            PRBool& aCanStoreInRuleTree);

  
  
  
  
  PRBool TreeHasCachedData() const {
    NS_ASSERTION(IsRoot(), "should only be called on root of rule tree");
    return HaveChildren() || mStyleData.mInheritedData || mStyleData.mResetData;
  }
};

#endif
