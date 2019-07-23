










































#ifndef nsRuleNode_h___
#define nsRuleNode_h___

#include "nsPresContext.h"
#include "nsStyleStruct.h"

class nsStyleContext;
struct nsRuleList;
struct PLDHashTable;
class nsILanguageAtomService;
struct nsRuleData;
class nsIStyleRule;
struct nsCSSStruct;

typedef nsCSSStruct nsRuleDataStruct;

struct nsRuleDataFont;
class nsCSSValue;

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

  NS_HIDDEN_(nsStyleStruct*) NS_FASTCALL GetStyleData(const nsStyleStructID& aSID) {
    
    
    
    
    
    
    
    
    

    

    const StyleStructInfo& info = gInfo[aSID];

    
    char* resetOrInheritSlot = NS_REINTERPRET_CAST(char*, this) + info.mCachedStyleDataOffset;

    
    char* resetOrInherit = NS_REINTERPRET_CAST(char*, *NS_REINTERPRET_CAST(void**, resetOrInheritSlot));

    nsStyleStruct* data = nsnull;
    if (resetOrInherit) {
      
      
      char* dataSlot = resetOrInherit + info.mInheritResetOffset;
      data = *NS_REINTERPRET_CAST(nsStyleStruct**, dataSlot);
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

  static PR_CALLBACK PLDHashNumber
  ChildrenHashHashKey(PLDHashTable *aTable, const void *aKey);

  static PR_CALLBACK PRBool
  ChildrenHashMatchEntry(PLDHashTable *aTable,
                         const PLDHashEntryHdr *aHdr,
                         const void *aKey);

  static PLDHashTableOps ChildrenHashOps;

  Key GetKey() const {
    return Key(mRule, GetLevel(), IsImportantRule());
  }

  
  
  
  

  void *mChildrenTaggedPtr; 

  enum {
    kTypeMask = 0x1,
    kListType = 0x0,
    kHashType = 0x1
  };
  enum {
    
    
    kMaxChildrenInList = 32
  };

  PRBool HaveChildren() {
    return mChildrenTaggedPtr != nsnull;
  }
  PRBool ChildrenAreHashed() {
    return (PRWord(mChildrenTaggedPtr) & kTypeMask) == kHashType;
  }
  nsRuleList* ChildrenList() {
    return NS_REINTERPRET_CAST(nsRuleList*, mChildrenTaggedPtr);
  }
  nsRuleList** ChildrenListPtr() {
    return NS_REINTERPRET_CAST(nsRuleList**, &mChildrenTaggedPtr);
  }
  PLDHashTable* ChildrenHash() {
    return (PLDHashTable*) (PRWord(mChildrenTaggedPtr) & ~PRWord(kTypeMask));
  }
  void SetChildrenList(nsRuleList *aList) {
    NS_ASSERTION(!(PRWord(aList) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildrenTaggedPtr = aList;
  }
  void SetChildrenHash(PLDHashTable *aHashtable) {
    NS_ASSERTION(!(PRWord(aHashtable) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildrenTaggedPtr = (void*)(PRWord(aHashtable) | kHashType);
  }
  void ConvertChildrenToHash();

  nsCachedStyleData mStyleData;   

  PRUint32 mDependentBits; 
                           

  PRUint32 mNoneBits; 
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      
                      

friend struct nsRuleList;

public:
  
  
  NS_HIDDEN_(void*) operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  NS_HIDDEN_(void) Destroy();
  static NS_HIDDEN_(nsILanguageAtomService*) gLangService;

protected:
  NS_HIDDEN_(void) PropagateDependentBit(PRUint32 aBit,
                                         nsRuleNode* aHighestNode);
  NS_HIDDEN_(void) PropagateNoneBit(PRUint32 aBit, nsRuleNode* aHighestNode);
  
  NS_HIDDEN_(const nsStyleStruct*) SetDefaultOnRoot(const nsStyleStructID aSID,
                                                    nsStyleContext* aContext);

  NS_HIDDEN_(const nsStyleStruct*)
    WalkRuleTree(const nsStyleStructID aSID, nsStyleContext* aContext, 
                 nsRuleData* aRuleData, nsRuleDataStruct* aSpecificData);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeDisplayData(nsStyleStruct* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeVisibilityData(nsStyleStruct* aStartStruct,
                          const nsRuleDataStruct& aData,
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeFontData(nsStyleStruct* aStartStruct,
                    const nsRuleDataStruct& aData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeColorData(nsStyleStruct* aStartStruct,
                     const nsRuleDataStruct& aData,
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeBackgroundData(nsStyleStruct* aStartStruct,
                          const nsRuleDataStruct& aData, 
                          nsStyleContext* aContext, nsRuleNode* aHighestNode,
                          const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeMarginData(nsStyleStruct* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeBorderData(nsStyleStruct* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputePaddingData(nsStyleStruct* aStartStruct,
                       const nsRuleDataStruct& aData, 
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeOutlineData(nsStyleStruct* aStartStruct,
                       const nsRuleDataStruct& aData, 
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeListData(nsStyleStruct* aStartStruct,
                    const nsRuleDataStruct& aData,
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputePositionData(nsStyleStruct* aStartStruct,
                        const nsRuleDataStruct& aData, 
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeTableData(nsStyleStruct* aStartStruct,
                     const nsRuleDataStruct& aData, 
                     nsStyleContext* aContext, nsRuleNode* aHighestNode,
                     const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeTableBorderData(nsStyleStruct* aStartStruct,
                           const nsRuleDataStruct& aData, 
                           nsStyleContext* aContext, nsRuleNode* aHighestNode,
                           const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeContentData(nsStyleStruct* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeQuotesData(nsStyleStruct* aStartStruct,
                      const nsRuleDataStruct& aData, 
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeTextData(nsStyleStruct* aStartStruct,
                    const nsRuleDataStruct& aData, 
                    nsStyleContext* aContext, nsRuleNode* aHighestNode,
                    const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeTextResetData(nsStyleStruct* aStartStruct,
                         const nsRuleDataStruct& aData,
                         nsStyleContext* aContext, nsRuleNode* aHighestNode,
                         const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeUserInterfaceData(nsStyleStruct* aStartStruct,
                             const nsRuleDataStruct& aData, 
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeUIResetData(nsStyleStruct* aStartStruct,
                       const nsRuleDataStruct& aData,
                       nsStyleContext* aContext, nsRuleNode* aHighestNode,
                       const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeXULData(nsStyleStruct* aStartStruct,
                   const nsRuleDataStruct& aData, 
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeColumnData(nsStyleStruct* aStartStruct,
                      const nsRuleDataStruct& aData,
                      nsStyleContext* aContext, nsRuleNode* aHighestNode,
                      const RuleDetail& aRuleDetail, PRBool aInherited);

#ifdef MOZ_SVG
  NS_HIDDEN_(const nsStyleStruct*)
    ComputeSVGData(nsStyleStruct* aStartStruct,
                   const nsRuleDataStruct& aData, 
                   nsStyleContext* aContext, nsRuleNode* aHighestNode,
                   const RuleDetail& aRuleDetail, PRBool aInherited);

  NS_HIDDEN_(const nsStyleStruct*)
    ComputeSVGResetData(nsStyleStruct* aStartStruct,
                        const nsRuleDataStruct& aData, 
                        nsStyleContext* aContext, nsRuleNode* aHighestNode,
                        const RuleDetail& aRuleDetail, PRBool aInherited);
#endif

  
  static NS_HIDDEN_(void) SetFont(nsPresContext* aPresContext,
                                  nsStyleContext* aContext,
                                  nscoord aMinFontSize,
                                  PRBool aIsGeneric,
                                  const nsRuleDataFont& aFontData,
                                  const nsFont& aDefaultFont,
                                  const nsStyleFont* aParentFont,
                                  nsStyleFont* aFont, PRBool& aInherited);

  static NS_HIDDEN_(void) SetGenericFont(nsPresContext* aPresContext,
                                         nsStyleContext* aContext,
                                         const nsRuleDataFont& aFontData,
                                         PRUint8 aGenericFontID,
                                         nscoord aMinFontSize,
                                         nsStyleFont* aFont);

  NS_HIDDEN_(void) AdjustLogicalBoxProp(nsStyleContext* aContext,
                                        const nsCSSValue& aLTRSource,
                                        const nsCSSValue& aRTLSource,
                                        const nsCSSValue& aLTRLogicalValue,
                                        const nsCSSValue& aRTLLogicalValue,
                                        const nsStyleSides& aParentRect,
                                        nsStyleSides& aRect,
                                        PRUint8 aSide,
                                        PRInt32 aMask,
                                        PRBool& aInherited);
  
  inline RuleDetail CheckSpecifiedProperties(const nsStyleStructID aSID, const nsRuleDataStruct& aRuleDataStruct);

  NS_HIDDEN_(const nsStyleStruct*) GetParentData(const nsStyleStructID aSID);
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)  \
    NS_HIDDEN_(const nsStyle##name_*) GetParent##name_();
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT  

  NS_HIDDEN_(const nsStyleStruct*) GetDisplayData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetVisibilityData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetFontData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetColorData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetBackgroundData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetMarginData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetBorderData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetPaddingData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetOutlineData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetListData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetPositionData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetTableData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*)
    GetTableBorderData(nsStyleContext* aContext);

  NS_HIDDEN_(const nsStyleStruct*) GetContentData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetQuotesData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetTextData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetTextResetData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*)
    GetUserInterfaceData(nsStyleContext* aContext);

  NS_HIDDEN_(const nsStyleStruct*) GetUIResetData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetXULData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetColumnData(nsStyleContext* aContext);
#ifdef MOZ_SVG
  NS_HIDDEN_(const nsStyleStruct*) GetSVGData(nsStyleContext* aContext);
  NS_HIDDEN_(const nsStyleStruct*) GetSVGResetData(nsStyleContext* aContext);
#endif

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

  NS_HIDDEN_(nsresult) ClearStyleData();
  NS_HIDDEN_(const nsStyleStruct*) GetStyleData(nsStyleStructID aSID, 
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
};

#endif
