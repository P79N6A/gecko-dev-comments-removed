













































#include "nsCSSRuleProcessor.h"

#define PL_ARENA_CONST_ALIGN_MASK 7
#define NS_RULEHASH_ARENA_BLOCK_SIZE (256)
#include "plarena.h"

#include "nsCRT.h"
#include "nsIAtom.h"
#include "pldhash.h"
#include "nsHashtable.h"
#include "nsICSSPseudoComparator.h"
#include "nsCSSRuleProcessor.h"
#include "nsICSSStyleRule.h"
#include "nsICSSGroupRule.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsGkAtoms.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsVoidArray.h"
#include "nsDOMError.h"
#include "nsRuleWalker.h"
#include "nsCSSPseudoClasses.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsStyleUtil.h"
#include "nsQuickSort.h"
#include "nsAttrValue.h"
#include "nsAttrName.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "nsIMediaList.h"

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static nsTArray< nsCOMPtr<nsIAtom> >* sSystemMetrics = 0;

struct RuleValue {
  









  RuleValue(nsICSSStyleRule* aRule, nsCSSSelector* aSelector)
    : mRule(aRule), mSelector(aSelector) {}

  RuleValue* Add(PRInt32 aBackwardIndex, RuleValue *aNext)
  {
    mBackwardIndex = aBackwardIndex;
    mNext = aNext;
    return this;
  }
    
  
  
  
  
  ~RuleValue()
  {
    
  }

  
  void *operator new(size_t aSize, PLArenaPool &aArena) CPP_THROW_NEW {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &aArena, aSize);
    return mem;
  }

  nsICSSStyleRule*  mRule;
  nsCSSSelector*    mSelector; 
  PRInt32           mBackwardIndex; 
  RuleValue*        mNext;
};






struct RuleHashTableEntry : public PLDHashEntryHdr {
  RuleValue *mRules; 
};

PR_STATIC_CALLBACK(PLDHashNumber)
RuleHash_CIHashKey(PLDHashTable *table, const void *key)
{
  nsIAtom *atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>(key));

  nsAutoString str;
  atom->ToString(str);
  ToUpperCase(str);
  return HashString(str);
}

typedef nsIAtom*
(* PR_CALLBACK RuleHashGetKey)    (PLDHashTable *table,
                                   const PLDHashEntryHdr *entry);

struct RuleHashTableOps {
  PLDHashTableOps ops;
  
  
  
  RuleHashGetKey getKey;
};

inline const RuleHashTableOps*
ToLocalOps(const PLDHashTableOps *aOps)
{
  return (const RuleHashTableOps*)
           (((const char*) aOps) - offsetof(RuleHashTableOps, ops));
}

PR_STATIC_CALLBACK(PRBool)
RuleHash_CIMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  
  nsIAtom *entry_atom = ToLocalOps(table->ops)->getKey(table, hdr);

  
  if (match_atom == entry_atom)
    return PR_TRUE;

  const char *match_str, *entry_str;
  match_atom->GetUTF8String(&match_str);
  entry_atom->GetUTF8String(&entry_str);

  return (nsCRT::strcasecmp(entry_str, match_str) == 0);
}

PR_STATIC_CALLBACK(PRBool)
RuleHash_CSMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  
  nsIAtom *entry_atom = ToLocalOps(table->ops)->getKey(table, hdr);

  return match_atom == entry_atom;
}

PR_STATIC_CALLBACK(nsIAtom*)
RuleHash_TagTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules->mSelector->mTag;
}

PR_STATIC_CALLBACK(nsIAtom*)
RuleHash_ClassTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules->mSelector->mClassList->mAtom;
}

PR_STATIC_CALLBACK(nsIAtom*)
RuleHash_IdTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules->mSelector->mIDList->mAtom;
}

PR_STATIC_CALLBACK(PLDHashNumber)
RuleHash_NameSpaceTable_HashKey(PLDHashTable *table, const void *key)
{
  return NS_PTR_TO_INT32(key);
}

PR_STATIC_CALLBACK(PRBool)
RuleHash_NameSpaceTable_MatchEntry(PLDHashTable *table,
                                   const PLDHashEntryHdr *hdr,
                                   const void *key)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);

  return NS_PTR_TO_INT32(key) ==
         entry->mRules->mSelector->mNameSpace;
}

static const RuleHashTableOps RuleHash_TagTable_Ops = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
  },
  RuleHash_TagTable_GetKey
};


static const RuleHashTableOps RuleHash_ClassTable_CSOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
  },
  RuleHash_ClassTable_GetKey
};


static const RuleHashTableOps RuleHash_ClassTable_CIOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
  },
  RuleHash_ClassTable_GetKey
};


static const RuleHashTableOps RuleHash_IdTable_CSOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
  },
  RuleHash_IdTable_GetKey
};


static const RuleHashTableOps RuleHash_IdTable_CIOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
  },
  RuleHash_IdTable_GetKey
};

static const PLDHashTableOps RuleHash_NameSpaceTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_NameSpaceTable_HashKey,
  RuleHash_NameSpaceTable_MatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL,
};

#undef RULE_HASH_STATS
#undef PRINT_UNIVERSAL_RULES

#ifdef DEBUG_dbaron
#define RULE_HASH_STATS
#define PRINT_UNIVERSAL_RULES
#endif

#ifdef RULE_HASH_STATS
#define RULE_HASH_STAT_INCREMENT(var_) PR_BEGIN_MACRO ++(var_); PR_END_MACRO
#else
#define RULE_HASH_STAT_INCREMENT(var_) PR_BEGIN_MACRO PR_END_MACRO
#endif


typedef void (*RuleEnumFunc)(nsICSSStyleRule* aRule,
                             nsCSSSelector* aSelector,
                             void *aData);

class RuleHash {
public:
  RuleHash(PRBool aQuirksMode);
  ~RuleHash();
  void PrependRule(RuleValue *aRuleInfo);
  void EnumerateAllRules(PRInt32 aNameSpace, nsIAtom* aTag, nsIAtom* aID,
                         const nsAttrValue* aClassList,
                         RuleEnumFunc aFunc, void* aData);
  void EnumerateTagRules(nsIAtom* aTag,
                         RuleEnumFunc aFunc, void* aData);
  PLArenaPool& Arena() { return mArena; }

protected:
  void PrependRuleToTable(PLDHashTable* aTable, const void* aKey,
                          RuleValue* aRuleInfo);
  void PrependUniversalRule(RuleValue* aRuleInfo);

  
  PRInt32     mRuleCount;
  PLDHashTable mIdTable;
  PLDHashTable mClassTable;
  PLDHashTable mTagTable;
  PLDHashTable mNameSpaceTable;
  RuleValue *mUniversalRules;

  RuleValue** mEnumList;
  PRInt32     mEnumListSize;

  PLArenaPool mArena;

#ifdef RULE_HASH_STATS
  PRUint32    mUniversalSelectors;
  PRUint32    mNameSpaceSelectors;
  PRUint32    mTagSelectors;
  PRUint32    mClassSelectors;
  PRUint32    mIdSelectors;

  PRUint32    mElementsMatched;
  PRUint32    mPseudosMatched;

  PRUint32    mElementUniversalCalls;
  PRUint32    mElementNameSpaceCalls;
  PRUint32    mElementTagCalls;
  PRUint32    mElementClassCalls;
  PRUint32    mElementIdCalls;

  PRUint32    mPseudoTagCalls;
#endif 
};

RuleHash::RuleHash(PRBool aQuirksMode)
  : mRuleCount(0),
    mUniversalRules(nsnull),
    mEnumList(nsnull), mEnumListSize(0)
#ifdef RULE_HASH_STATS
    ,
    mUniversalSelectors(0),
    mNameSpaceSelectors(0),
    mTagSelectors(0),
    mClassSelectors(0),
    mIdSelectors(0),
    mElementsMatched(0),
    mPseudosMatched(0),
    mElementUniversalCalls(0),
    mElementNameSpaceCalls(0),
    mElementTagCalls(0),
    mElementClassCalls(0),
    mElementIdCalls(0),
    mPseudoTagCalls(0)
#endif
{
  
  PL_INIT_ARENA_POOL(&mArena, "RuleHashArena", NS_RULEHASH_ARENA_BLOCK_SIZE);

  PL_DHashTableInit(&mTagTable, &RuleHash_TagTable_Ops.ops, nsnull,
                    sizeof(RuleHashTableEntry), 64);
  PL_DHashTableInit(&mIdTable,
                    aQuirksMode ? &RuleHash_IdTable_CIOps.ops
                                : &RuleHash_IdTable_CSOps.ops,
                    nsnull, sizeof(RuleHashTableEntry), 16);
  PL_DHashTableInit(&mClassTable,
                    aQuirksMode ? &RuleHash_ClassTable_CIOps.ops
                                : &RuleHash_ClassTable_CSOps.ops,
                    nsnull, sizeof(RuleHashTableEntry), 16);
  PL_DHashTableInit(&mNameSpaceTable, &RuleHash_NameSpaceTable_Ops, nsnull,
                    sizeof(RuleHashTableEntry), 16);
}

RuleHash::~RuleHash()
{
#ifdef RULE_HASH_STATS
  printf(
"RuleHash(%p):\n"
"  Selectors: Universal (%u) NameSpace(%u) Tag(%u) Class(%u) Id(%u)\n"
"  Content Nodes: Elements(%u) Pseudo-Elements(%u)\n"
"  Element Calls: Universal(%u) NameSpace(%u) Tag(%u) Class(%u) Id(%u)\n"
"  Pseudo-Element Calls: Tag(%u)\n",
         static_cast<void*>(this),
         mUniversalSelectors, mNameSpaceSelectors, mTagSelectors,
           mClassSelectors, mIdSelectors,
         mElementsMatched,
         mPseudosMatched,
         mElementUniversalCalls, mElementNameSpaceCalls, mElementTagCalls,
           mElementClassCalls, mElementIdCalls,
         mPseudoTagCalls);
#ifdef PRINT_UNIVERSAL_RULES
  {
    RuleValue* value = mUniversalRules;
    if (value) {
      printf("  Universal rules:\n");
      do {
        nsAutoString selectorText;
        PRUint32 lineNumber = value->mRule->GetLineNumber();
        nsCOMPtr<nsIStyleSheet> sheet;
        value->mRule->GetStyleSheet(*getter_AddRefs(sheet));
        nsCOMPtr<nsICSSStyleSheet> cssSheet = do_QueryInterface(sheet);
        value->mSelector->ToString(selectorText, cssSheet);

        printf("    line %d, %s\n",
               lineNumber, NS_ConvertUTF16toUTF8(selectorText).get());
        value = value->mNext;
      } while (value);
    }
  }
#endif 
#endif 
  
  
  
  if (nsnull != mEnumList) {
    delete [] mEnumList;
  }
  
  PL_DHashTableFinish(&mIdTable);
  PL_DHashTableFinish(&mClassTable);
  PL_DHashTableFinish(&mTagTable);
  PL_DHashTableFinish(&mNameSpaceTable);
  PL_FinishArenaPool(&mArena);
}

void RuleHash::PrependRuleToTable(PLDHashTable* aTable, const void* aKey,
                                  RuleValue* aRuleInfo)
{
  
  RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                         (PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
  if (!entry)
    return;
  entry->mRules = aRuleInfo->Add(mRuleCount++, entry->mRules);
}

void RuleHash::PrependUniversalRule(RuleValue *aRuleInfo)
{
  mUniversalRules = aRuleInfo->Add(mRuleCount++, mUniversalRules);
}

void RuleHash::PrependRule(RuleValue *aRuleInfo)
{
  nsCSSSelector *selector = aRuleInfo->mSelector;
  if (nsnull != selector->mIDList) {
    PrependRuleToTable(&mIdTable, selector->mIDList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mIdSelectors);
  }
  else if (nsnull != selector->mClassList) {
    PrependRuleToTable(&mClassTable, selector->mClassList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mClassSelectors);
  }
  else if (nsnull != selector->mTag) {
    PrependRuleToTable(&mTagTable, selector->mTag, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mTagSelectors);
  }
  else if (kNameSpaceID_Unknown != selector->mNameSpace) {
    PrependRuleToTable(&mNameSpaceTable,
                       NS_INT32_TO_PTR(selector->mNameSpace), aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mNameSpaceSelectors);
  }
  else {  
    PrependUniversalRule(aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mUniversalSelectors);
  }
}


#define MIN_ENUM_LIST_SIZE 8

#ifdef RULE_HASH_STATS
#define RULE_HASH_STAT_INCREMENT_LIST_COUNT(list_, var_) \
  do { ++(var_); (list_) = (list_)->mNext; } while (list_)
#else
#define RULE_HASH_STAT_INCREMENT_LIST_COUNT(list_, var_) \
  PR_BEGIN_MACRO PR_END_MACRO
#endif

void RuleHash::EnumerateAllRules(PRInt32 aNameSpace, nsIAtom* aTag,
                                 nsIAtom* aID, const nsAttrValue* aClassList,
                                 RuleEnumFunc aFunc, void* aData)
{
  PRInt32 classCount = aClassList ? aClassList->GetAtomCount() : 0;

  
  
  PRInt32 testCount = classCount + 4;

  if (mEnumListSize < testCount) {
    delete [] mEnumList;
    mEnumListSize = PR_MAX(testCount, MIN_ENUM_LIST_SIZE);
    mEnumList = new RuleValue*[mEnumListSize];
  }

  PRInt32 valueCount = 0;
  RULE_HASH_STAT_INCREMENT(mElementsMatched);

  { 
    RuleValue* value = mUniversalRules;
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementUniversalCalls);
    }
  }
  
  if (kNameSpaceID_Unknown != aNameSpace) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mNameSpaceTable, NS_INT32_TO_PTR(aNameSpace),
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementNameSpaceCalls);
    }
  }
  if (nsnull != aTag) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mTagTable, aTag, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementTagCalls);
    }
  }
  if (nsnull != aID) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mIdTable, aID, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementIdCalls);
    }
  }
  { 
    for (PRInt32 index = 0; index < classCount; ++index) {
      RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                             (PL_DHashTableOperate(&mClassTable, aClassList->AtomAt(index),
                             PL_DHASH_LOOKUP));
      if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        RuleValue *value = entry->mRules;
        mEnumList[valueCount++] = value;
        RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementClassCalls);
      }
    }
  }
  NS_ASSERTION(valueCount <= testCount, "values exceeded list size");

  if (valueCount > 0) {
    
    while (valueCount > 1) {
      PRInt32 valueIndex = 0;
      PRInt32 highestRuleIndex = mEnumList[valueIndex]->mBackwardIndex;
      for (PRInt32 index = 1; index < valueCount; ++index) {
        PRInt32 ruleIndex = mEnumList[index]->mBackwardIndex;
        if (ruleIndex > highestRuleIndex) {
          valueIndex = index;
          highestRuleIndex = ruleIndex;
        }
      }
      RuleValue *cur = mEnumList[valueIndex];
      (*aFunc)(cur->mRule, cur->mSelector, aData);
      RuleValue *next = cur->mNext;
      mEnumList[valueIndex] = next ? next : mEnumList[--valueCount];
    }

    
    RuleValue* value = mEnumList[0];
    do {
      (*aFunc)(value->mRule, value->mSelector, aData);
      value = value->mNext;
    } while (value);
  }
}

void RuleHash::EnumerateTagRules(nsIAtom* aTag, RuleEnumFunc aFunc, void* aData)
{
  RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                         (PL_DHashTableOperate(&mTagTable, aTag, PL_DHASH_LOOKUP));

  RULE_HASH_STAT_INCREMENT(mPseudosMatched);
  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
    RuleValue *tagValue = entry->mRules;
    do {
      RULE_HASH_STAT_INCREMENT(mPseudoTagCalls);
      (*aFunc)(tagValue->mRule, tagValue->mSelector, aData);
      tagValue = tagValue->mNext;
    } while (tagValue);
  }
}




struct AttributeSelectorEntry : public PLDHashEntryHdr {
  nsIAtom *mAttribute;
  nsVoidArray *mSelectors;
};

PR_STATIC_CALLBACK(void)
AttributeSelectorClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  AttributeSelectorEntry *entry = static_cast<AttributeSelectorEntry*>(hdr);
  delete entry->mSelectors;
  memset(entry, 0, table->entrySize);
}

static const PLDHashTableOps AttributeSelectorOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PL_DHashMatchEntryStub,
  PL_DHashMoveEntryStub,
  AttributeSelectorClearEntry,
  PL_DHashFinalizeStub,
  NULL
};




struct RuleCascadeData {
  RuleCascadeData(nsIAtom *aMedium, PRBool aQuirksMode)
    : mRuleHash(aQuirksMode),
      mStateSelectors(),
      mCacheKey(aMedium),
      mNext(nsnull)
  {
    PL_DHashTableInit(&mAttributeSelectors, &AttributeSelectorOps, nsnull,
                      sizeof(AttributeSelectorEntry), 16);
  }

  ~RuleCascadeData()
  {
    PL_DHashTableFinish(&mAttributeSelectors);
  }
  RuleHash          mRuleHash;
  nsVoidArray       mStateSelectors;
  nsVoidArray       mClassSelectors;
  nsVoidArray       mIDSelectors;
  PLDHashTable      mAttributeSelectors; 

  
  
  nsVoidArray* AttributeListFor(nsIAtom* aAttribute);

  nsMediaQueryResultCacheKey mCacheKey;
  RuleCascadeData*  mNext; 
};

nsVoidArray*
RuleCascadeData::AttributeListFor(nsIAtom* aAttribute)
{
  AttributeSelectorEntry *entry = static_cast<AttributeSelectorEntry*>
                                             (PL_DHashTableOperate(&mAttributeSelectors, aAttribute, PL_DHASH_ADD));
  if (!entry)
    return nsnull;
  if (!entry->mSelectors) {
    if (!(entry->mSelectors = new nsVoidArray)) {
      PL_DHashTableRawRemove(&mAttributeSelectors, entry);
      return nsnull;
    }
    entry->mAttribute = aAttribute;
  }
  return entry->mSelectors;
}





nsCSSRuleProcessor::nsCSSRuleProcessor(const nsCOMArray<nsICSSStyleSheet>& aSheets)
  : mSheets(aSheets)
  , mRuleCascades(nsnull)
  , mLastPresContext(nsnull)
{
  for (PRInt32 i = mSheets.Count() - 1; i >= 0; --i)
    mSheets[i]->AddRuleProcessor(this);
}

nsCSSRuleProcessor::~nsCSSRuleProcessor()
{
  for (PRInt32 i = mSheets.Count() - 1; i >= 0; --i)
    mSheets[i]->DropRuleProcessor(this);
  mSheets.Clear();
  ClearRuleCascades();
}

NS_IMPL_ISUPPORTS1(nsCSSRuleProcessor, nsIStyleRuleProcessor)

static PRBool
InitSystemMetrics()
{
  NS_ASSERTION(!sSystemMetrics, "already initialized");

  sSystemMetrics = new nsTArray< nsCOMPtr<nsIAtom> >;
  NS_ENSURE_TRUE(sSystemMetrics, PR_FALSE);

  nsresult rv;
  nsCOMPtr<nsILookAndFeel> lookAndFeel(do_GetService(kLookAndFeelCID, &rv));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRInt32 metricResult;
  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ScrollArrowStyle, metricResult);
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowStartBackward) {
    sSystemMetrics->AppendElement(do_GetAtom("scrollbar-start-backward"));
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowStartForward) {
    sSystemMetrics->AppendElement(do_GetAtom("scrollbar-start-forward"));
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowEndBackward) {
    sSystemMetrics->AppendElement(do_GetAtom("scrollbar-end-backward"));
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowEndForward) {
    sSystemMetrics->AppendElement(do_GetAtom("scrollbar-end-forward"));
  }

  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ScrollSliderStyle, metricResult);
  if (metricResult != nsILookAndFeel::eMetric_ScrollThumbStyleNormal) {
    sSystemMetrics->AppendElement(do_GetAtom("scrollbar-thumb-proportional"));
  }

  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ImagesInMenus, metricResult);
  if (metricResult) {
    sSystemMetrics->AppendElement(do_GetAtom("images-in-menus"));
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_WindowsDefaultTheme, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(do_GetAtom("windows-default-theme"));
  }

  return PR_TRUE;
}

 void
nsCSSRuleProcessor::Shutdown()
{
  delete sSystemMetrics;
  sSystemMetrics = nsnull;
}

RuleProcessorData::RuleProcessorData(nsPresContext* aPresContext,
                                     nsIContent* aContent, 
                                     nsRuleWalker* aRuleWalker,
                                     nsCompatibility* aCompat )
{
  MOZ_COUNT_CTOR(RuleProcessorData);

  NS_ASSERTION(!aContent || aContent->IsNodeOfType(nsINode::eELEMENT),
               "non-element leaked into SelectorMatches");

  mPresContext = aPresContext;
  mContent = aContent;
  mParentContent = nsnull;
  mRuleWalker = aRuleWalker;
  mScopedRoot = nsnull;

  mContentTag = nsnull;
  mContentID = nsnull;
  mHasAttributes = PR_FALSE;
  mIsHTMLContent = PR_FALSE;
  mIsLink = PR_FALSE;
  mLinkState = eLinkState_Unknown;
  mEventState = 0;
  mNameSpaceID = kNameSpaceID_Unknown;
  mPreviousSiblingData = nsnull;
  mParentData = nsnull;
  mLanguage = nsnull;
  mClasses = nsnull;
  mNthIndices[0][0] = -2;
  mNthIndices[0][1] = -2;
  mNthIndices[1][0] = -2;
  mNthIndices[1][1] = -2;

  
  
  if (aCompat) {
    mCompatMode = *aCompat;
  } else if (NS_LIKELY(mPresContext)) {
    mCompatMode = mPresContext->CompatibilityMode();
  } else {
    NS_ASSERTION(aContent, "Must have content");
    NS_ASSERTION(aContent->GetOwnerDoc(), "Must have document");
    mCompatMode = aContent->GetOwnerDoc()->GetCompatibilityMode();
  }

  if (aContent) {
    NS_ASSERTION(aContent->GetOwnerDoc(), "Document-less node here?");
    
    
    mContentTag = aContent->Tag();
    mParentContent = aContent->GetParent();

    
    if (mPresContext) {
      mPresContext->EventStateManager()->GetContentState(aContent, mEventState);
    } else {
      mEventState = aContent->IntrinsicState();
    }

    
    mContentID = aContent->GetID();
    mClasses = aContent->GetClasses();

    
    mHasAttributes = aContent->GetAttrCount() > 0;

    
    if (aContent->IsNodeOfType(nsINode::eHTML)) {
      mIsHTMLContent = PR_TRUE;
      
      
      mNameSpaceID = kNameSpaceID_XHTML;
    } else {
      
      mNameSpaceID = aContent->GetNameSpaceID();
    }

    
    
    nsILinkHandler* linkHandler =
      mPresContext ? mPresContext->GetLinkHandler() : nsnull;
    if (mIsHTMLContent && mHasAttributes) {
      
      if(nsStyleUtil::IsHTMLLink(aContent, mContentTag,
                                 linkHandler, aRuleWalker != nsnull,
                                 &mLinkState)) {
        mIsLink = PR_TRUE;
      }
    } 

    
    
    if(!mIsLink &&
       mHasAttributes && 
       !(mIsHTMLContent || aContent->IsNodeOfType(nsINode::eXUL)) && 
       nsStyleUtil::IsLink(aContent, linkHandler,
                           aRuleWalker != nsnull, &mLinkState)) {
      mIsLink = PR_TRUE;
    } 
  }
}

RuleProcessorData::~RuleProcessorData()
{
  MOZ_COUNT_DTOR(RuleProcessorData);

  
  
  if (mPreviousSiblingData || mParentData) {
    nsAutoVoidArray destroyQueue;
    destroyQueue.AppendElement(this);

    do {
      RuleProcessorData *d = static_cast<RuleProcessorData*>
                                        (destroyQueue.FastElementAt(destroyQueue.Count() - 1));
      destroyQueue.RemoveElementAt(destroyQueue.Count() - 1);

      if (d->mPreviousSiblingData) {
        destroyQueue.AppendElement(d->mPreviousSiblingData);
        d->mPreviousSiblingData = nsnull;
      }
      if (d->mParentData) {
        destroyQueue.AppendElement(d->mParentData);
        d->mParentData = nsnull;
      }

      if (d != this)
        d->Destroy();
    } while (destroyQueue.Count());
  }

  delete mLanguage;
}

const nsString* RuleProcessorData::GetLang()
{
  if (!mLanguage) {
    mLanguage = new nsAutoString();
    if (!mLanguage)
      return nsnull;
    for (nsIContent* content = mContent; content;
         content = content->GetParent()) {
      if (content->GetAttrCount() > 0) {
        
        
        nsAutoString value;
        PRBool hasAttr = content->GetAttr(kNameSpaceID_XML, nsGkAtoms::lang,
                                          value);
        if (!hasAttr && content->IsNodeOfType(nsINode::eHTML)) {
          hasAttr = content->GetAttr(kNameSpaceID_None, nsGkAtoms::lang,
                                     value);
        }
        if (hasAttr) {
          *mLanguage = value;
          break;
        }
      }
    }
  }
  return mLanguage;
}

static inline PRInt32
CSSNameSpaceID(nsIContent *aContent)
{
  return aContent->IsNodeOfType(nsINode::eHTML)
           ? kNameSpaceID_XHTML
           : aContent->GetNameSpaceID();
}

PRInt32
RuleProcessorData::GetNthIndex(PRBool aIsOfType, PRBool aIsFromEnd,
                               PRBool aCheckEdgeOnly)
{
  NS_ASSERTION(mParentContent, "caller should check mParentContent");
  NS_ASSERTION(!mPreviousSiblingData ||
               mPreviousSiblingData->mContent->IsNodeOfType(nsINode::eELEMENT),
               "Unexpected previous sibling data");

  PRInt32 &slot = mNthIndices[aIsOfType][aIsFromEnd];
  if (slot != -2 && (slot != -1 || aCheckEdgeOnly))
    return slot;

  if (mPreviousSiblingData &&
      (!aIsOfType ||
       (mPreviousSiblingData->mNameSpaceID == mNameSpaceID &&
        mPreviousSiblingData->mContentTag == mContentTag))) {
    slot = mPreviousSiblingData->mNthIndices[aIsOfType][aIsFromEnd];
    if (slot > 0) {
      slot += (aIsFromEnd ? -1 : 1);
      NS_ASSERTION(slot > 0, "How did that happen?");
      return slot;
    }
  }

  PRInt32 result = 1;
  nsIContent* parent = mParentContent;

  PRUint32 cur;
  PRInt32 increment;
  if (aIsFromEnd) {
    cur = parent->GetChildCount() - 1;
    increment = -1;
  } else {
    cur = 0;
    increment = 1;
  }

  for (;;) {
    nsIContent* child = parent->GetChildAt(cur);
    if (!child) {
      
      result = 0; 
      break;
    }
    cur += increment;
    if (child == mContent)
      break;
    if (child->IsNodeOfType(nsINode::eELEMENT) &&
        (!aIsOfType ||
         (child->Tag() == mContentTag &&
          CSSNameSpaceID(child) == mNameSpaceID))) {
      if (aCheckEdgeOnly) {
        
        
        result = -1;
        break;
      }
      ++result;
    }
  }

  slot = result;
  return result;
}

static PRBool ValueIncludes(const nsSubstring& aValueList,
                            const nsSubstring& aValue,
                            const nsStringComparator& aComparator)
{
  const PRUnichar *p = aValueList.BeginReading(),
              *p_end = aValueList.EndReading();

  while (p < p_end) {
    
    while (p != p_end && nsContentUtils::IsHTMLWhitespace(*p))
      ++p;

    const PRUnichar *val_start = p;

    
    while (p != p_end && !nsContentUtils::IsHTMLWhitespace(*p))
      ++p;

    const PRUnichar *val_end = p;

    if (val_start < val_end &&
        aValue.Equals(Substring(val_start, val_end), aComparator))
      return PR_TRUE;

    ++p; 
  }
  return PR_FALSE;
}

inline PRBool IsLinkPseudo(nsIAtom* aAtom)
{
  return PRBool ((nsCSSPseudoClasses::link == aAtom) || 
                 (nsCSSPseudoClasses::visited == aAtom) ||
                 (nsCSSPseudoClasses::mozAnyLink == aAtom));
}





inline PRBool IsQuirkEventSensitive(nsIAtom *aContentTag)
{
  return PRBool ((nsGkAtoms::button == aContentTag) ||
                 (nsGkAtoms::img == aContentTag)    ||
                 (nsGkAtoms::input == aContentTag)  ||
                 (nsGkAtoms::label == aContentTag)  ||
                 (nsGkAtoms::select == aContentTag) ||
                 (nsGkAtoms::textarea == aContentTag));
}


static inline PRBool
IsSignificantChild(nsIContent* aChild, PRBool aTextIsSignificant,
                   PRBool aWhitespaceIsSignificant)
{
  return nsStyleUtil::IsSignificantChild(aChild, aTextIsSignificant,
                                         aWhitespaceIsSignificant);
}




static PRBool AttrMatchesValue(const nsAttrSelector* aAttrSelector,
                               const nsString& aValue)
{
  NS_PRECONDITION(aAttrSelector, "Must have an attribute selector");

  
  
  
  if (aAttrSelector->mValue.IsEmpty() &&
      (aAttrSelector->mFunction == NS_ATTR_FUNC_INCLUDES ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_ENDSMATCH ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_BEGINSMATCH ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_CONTAINSMATCH))
    return PR_FALSE;

  const nsDefaultStringComparator defaultComparator;
  const nsCaseInsensitiveStringComparator ciComparator;
  const nsStringComparator& comparator = aAttrSelector->mCaseSensitive
                ? static_cast<const nsStringComparator&>(defaultComparator)
                : static_cast<const nsStringComparator&>(ciComparator);
  switch (aAttrSelector->mFunction) {
    case NS_ATTR_FUNC_EQUALS: 
      return aValue.Equals(aAttrSelector->mValue, comparator);
    case NS_ATTR_FUNC_INCLUDES: 
      return ValueIncludes(aValue, aAttrSelector->mValue, comparator);
    case NS_ATTR_FUNC_DASHMATCH: 
      return nsStyleUtil::DashMatchCompare(aValue, aAttrSelector->mValue, comparator);
    case NS_ATTR_FUNC_ENDSMATCH:
      return StringEndsWith(aValue, aAttrSelector->mValue, comparator);
    case NS_ATTR_FUNC_BEGINSMATCH:
      return StringBeginsWith(aValue, aAttrSelector->mValue, comparator);
    case NS_ATTR_FUNC_CONTAINSMATCH:
      return FindInReadable(aAttrSelector->mValue, aValue, comparator);
    default:
      NS_NOTREACHED("Shouldn't be ending up here");
      return PR_FALSE;
  }
}
















static PRBool SelectorMatches(RuleProcessorData &data,
                              nsCSSSelector* aSelector,
                              PRInt32 aStateMask, 
                              nsIAtom* aAttribute, 
                              PRBool aForStyling,
                              PRBool* const aDependence = nsnull) 

{
  
  if ((kNameSpaceID_Unknown != aSelector->mNameSpace &&
       data.mNameSpaceID != aSelector->mNameSpace) ||
      (aSelector->mTag && aSelector->mTag != data.mContentTag)) {
    
    return PR_FALSE;
  }

  PRBool result = PR_TRUE;
  const PRBool isNegated = (aDependence != nsnull);
  
  
  
  
  
  const PRBool setNodeFlags = aForStyling && aStateMask == 0 && !aAttribute;

  
  
  
  for (nsPseudoClassList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass && result; pseudoClass = pseudoClass->mNext) {
    PRInt32 stateToCheck = 0;
    if ((nsCSSPseudoClasses::firstChild == pseudoClass->mAtom) ||
        (nsCSSPseudoClasses::firstNode == pseudoClass->mAtom) ) {
      nsIContent *firstChild = nsnull;
      nsIContent *parent = data.mParentContent;
      if (parent) {
        if (setNodeFlags)
          parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

        PRBool acceptNonWhitespace =
          nsCSSPseudoClasses::firstNode == pseudoClass->mAtom;
        PRInt32 index = -1;
        do {
          firstChild = parent->GetChildAt(++index);
          
          
        } while (firstChild &&
                 !IsSignificantChild(firstChild, acceptNonWhitespace, PR_FALSE));
      }
      result = (data.mContent == firstChild);
    }
    else if ((nsCSSPseudoClasses::lastChild == pseudoClass->mAtom) ||
             (nsCSSPseudoClasses::lastNode == pseudoClass->mAtom)) {
      nsIContent *lastChild = nsnull;
      nsIContent *parent = data.mParentContent;
      if (parent) {
        if (setNodeFlags)
          parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

        PRBool acceptNonWhitespace =
          nsCSSPseudoClasses::lastNode == pseudoClass->mAtom;
        PRUint32 index = parent->GetChildCount();
        do {
          lastChild = parent->GetChildAt(--index);
          
          
        } while (lastChild &&
                 !IsSignificantChild(lastChild, acceptNonWhitespace, PR_FALSE));
      }
      result = (data.mContent == lastChild);
    }
    else if (nsCSSPseudoClasses::onlyChild == pseudoClass->mAtom) {
      nsIContent *onlyChild = nsnull;
      nsIContent *moreChild = nsnull;
      nsIContent *parent = data.mParentContent;
      if (parent) {
        if (setNodeFlags)
          parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

        PRInt32 index = -1;
        do {
          onlyChild = parent->GetChildAt(++index);
          
        } while (onlyChild &&
                 !IsSignificantChild(onlyChild, PR_FALSE, PR_FALSE));
        if (data.mContent == onlyChild) {
          
          do {
            moreChild = parent->GetChildAt(++index);
          } while (moreChild && !IsSignificantChild(moreChild, PR_FALSE, PR_FALSE));
        }
      }
      result = (data.mContent == onlyChild && moreChild == nsnull);
    }
    else if (nsCSSPseudoClasses::nthChild == pseudoClass->mAtom ||
             nsCSSPseudoClasses::nthLastChild == pseudoClass->mAtom ||
             nsCSSPseudoClasses::nthOfType == pseudoClass->mAtom ||
             nsCSSPseudoClasses::nthLastOfType == pseudoClass->mAtom) {
      nsIContent *parent = data.mParentContent;
      if (parent) {
        PRBool isOfType =
          nsCSSPseudoClasses::nthOfType == pseudoClass->mAtom ||
          nsCSSPseudoClasses::nthLastOfType == pseudoClass->mAtom;
        PRBool isFromEnd =
          nsCSSPseudoClasses::nthLastChild == pseudoClass->mAtom ||
          nsCSSPseudoClasses::nthLastOfType == pseudoClass->mAtom;
        if (setNodeFlags) {
          if (isFromEnd)
            parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
          else
            parent->SetFlags(NODE_HAS_SLOW_SELECTOR_NOAPPEND);
        }

        const PRInt32 index = data.GetNthIndex(isOfType, isFromEnd, PR_FALSE);
        if (index <= 0) {
          
          result = PR_FALSE;
        } else {
          const PRInt32 a = pseudoClass->u.mNumbers[0];
          const PRInt32 b = pseudoClass->u.mNumbers[1];
          
          
          if (a == 0) {
            result = b == index;
          } else {
            
            
            
            const PRInt32 n = (index - b) / a;
            result = n >= 0 && (a * n == index - b);
          }
        }
      } else {
        result = PR_FALSE;
      }
    }
    else if (nsCSSPseudoClasses::firstOfType == pseudoClass->mAtom ||
             nsCSSPseudoClasses::lastOfType == pseudoClass->mAtom ||
             nsCSSPseudoClasses::onlyOfType == pseudoClass->mAtom) {
      nsIContent *parent = data.mParentContent;
      if (parent) {
        const PRBool checkFirst =
          pseudoClass->mAtom != nsCSSPseudoClasses::lastOfType;
        const PRBool checkLast =
          pseudoClass->mAtom != nsCSSPseudoClasses::firstOfType;
        if (setNodeFlags) {
          if (checkLast)
            parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
          else
            parent->SetFlags(NODE_HAS_SLOW_SELECTOR_NOAPPEND);
        }

        result = (!checkFirst ||
                  data.GetNthIndex(PR_TRUE, PR_FALSE, PR_TRUE) == 1) &&
                 (!checkLast ||
                  data.GetNthIndex(PR_TRUE, PR_TRUE, PR_TRUE) == 1);
      } else {
        result = PR_FALSE;
      }
    }
    else if (nsCSSPseudoClasses::empty == pseudoClass->mAtom ||
             nsCSSPseudoClasses::mozOnlyWhitespace == pseudoClass->mAtom) {
      nsIContent *child = nsnull;
      nsIContent *element = data.mContent;
      const PRBool isWhitespaceSignificant =
        nsCSSPseudoClasses::empty == pseudoClass->mAtom;
      PRInt32 index = -1;

      if (setNodeFlags)
        element->SetFlags(NODE_HAS_EMPTY_SELECTOR);

      do {
        child = element->GetChildAt(++index);
        
        
      } while (child && !IsSignificantChild(child, PR_TRUE, isWhitespaceSignificant));
      result = (child == nsnull);
    }
    else if (nsCSSPseudoClasses::mozEmptyExceptChildrenWithLocalname == pseudoClass->mAtom) {
      NS_ASSERTION(pseudoClass->u.mString, "Must have string!");
      nsIContent *child = nsnull;
      nsIContent *element = data.mContent;
      PRInt32 index = -1;

      if (setNodeFlags)
        element->SetFlags(NODE_HAS_SLOW_SELECTOR);

      do {
        child = element->GetChildAt(++index);
      } while (child &&
               (!IsSignificantChild(child, PR_TRUE, PR_FALSE) ||
                (child->GetNameSpaceID() == element->GetNameSpaceID() &&
                 child->Tag()->Equals(nsDependentString(pseudoClass->u.mString)))));
      result = (child == nsnull);
    }
    else if (nsCSSPseudoClasses::mozSystemMetric == pseudoClass->mAtom) {
      if (!sSystemMetrics && !InitSystemMetrics()) {
        return PR_FALSE;
      }
      NS_ASSERTION(pseudoClass->u.mString, "Must have string!");
      nsCOMPtr<nsIAtom> metric = do_GetAtom(pseudoClass->u.mString);
      result = sSystemMetrics->IndexOf(metric) !=
               sSystemMetrics->NoIndex;
    }
    else if (nsCSSPseudoClasses::mozHasHandlerRef == pseudoClass->mAtom) {
      nsIContent *child = nsnull;
      nsIContent *element = data.mContent;
      PRInt32 index = -1;

      result = PR_FALSE;
      if (element) {
        do {
          child = element->GetChildAt(++index);
          if (child && child->IsNodeOfType(nsINode::eHTML) &&
              child->Tag() == nsGkAtoms::param &&
              child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                                 NS_LITERAL_STRING("pluginurl"), eIgnoreCase)) {
            result = PR_TRUE;
            break;
          }
        } while (child);
      }
    }
    else if (nsCSSPseudoClasses::root == pseudoClass->mAtom) {
      result = (data.mParentContent == nsnull &&
                data.mContent &&
                data.mContent ==
                  data.mContent->GetOwnerDoc()->GetRootContent());
    }
    else if (nsCSSPseudoClasses::mozBoundElement == pseudoClass->mAtom) {
      
      
      
      result = (data.mScopedRoot && data.mScopedRoot == data.mContent);
    }
    else if (nsCSSPseudoClasses::lang == pseudoClass->mAtom) {
      NS_ASSERTION(nsnull != pseudoClass->u.mString, "null lang parameter");
      result = PR_FALSE;
      if (pseudoClass->u.mString && *pseudoClass->u.mString) {
        
        
        
        
        const nsString* lang = data.GetLang();
        if (lang && !lang->IsEmpty()) { 
          result = nsStyleUtil::DashMatchCompare(*lang,
                                    nsDependentString(pseudoClass->u.mString), 
                                    nsCaseInsensitiveStringComparator());
        }
        else if (data.mContent) {
          nsIDocument* doc = data.mContent->GetDocument();
          if (doc) {
            
            
            
            
            nsAutoString language;
            doc->GetContentLanguage(language);

            nsDependentString langString(pseudoClass->u.mString);
            language.StripWhitespace();
            PRInt32 begin = 0;
            PRInt32 len = language.Length();
            while (begin < len) {
              PRInt32 end = language.FindChar(PRUnichar(','), begin);
              if (end == kNotFound) {
                end = len;
              }
              if (nsStyleUtil::DashMatchCompare(Substring(language, begin, end-begin),
                                   langString,
                                   nsCaseInsensitiveStringComparator())) {
                result = PR_TRUE;
                break;
              }
              begin = end + 1;
            }
          }
        }
      }
    } else if (nsCSSPseudoClasses::active == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_ACTIVE;
    }
    else if (nsCSSPseudoClasses::focus == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_FOCUS;
    }
    else if (nsCSSPseudoClasses::hover == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_HOVER;
    }
    else if (nsCSSPseudoClasses::mozDragOver == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_DRAGOVER;
    }
    else if (nsCSSPseudoClasses::target == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_URLTARGET;
    }
    else if (IsLinkPseudo(pseudoClass->mAtom)) {
      if (data.mIsLink) {
        if (nsCSSPseudoClasses::mozAnyLink == pseudoClass->mAtom) {
          result = PR_TRUE;
        }
        else {
          NS_ASSERTION(nsCSSPseudoClasses::link == pseudoClass->mAtom ||
                       nsCSSPseudoClasses::visited == pseudoClass->mAtom,
                       "somebody changed IsLinkPseudo");
          NS_ASSERTION(data.mLinkState == eLinkState_Unvisited ||
                       data.mLinkState == eLinkState_Visited,
                       "unexpected link state for mIsLink");
          if (aStateMask & NS_EVENT_STATE_VISITED) {
            result = PR_TRUE;
            if (aDependence)
              *aDependence = PR_TRUE;
          } else {
            result = ((eLinkState_Unvisited == data.mLinkState) ==
                      (nsCSSPseudoClasses::link == pseudoClass->mAtom));
          }
        }
      }
      else {
        result = PR_FALSE;  
      }
    }
    else if (nsCSSPseudoClasses::checked == pseudoClass->mAtom) {
      
      
      
      
      stateToCheck = NS_EVENT_STATE_CHECKED;
    }
    else if (nsCSSPseudoClasses::enabled == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_ENABLED;
    }
    else if (nsCSSPseudoClasses::disabled == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_DISABLED;
    }    
    else if (nsCSSPseudoClasses::mozBroken == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_BROKEN;
    }
    else if (nsCSSPseudoClasses::mozUserDisabled == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_USERDISABLED;
    }
    else if (nsCSSPseudoClasses::mozSuppressed == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_SUPPRESSED;
    }
    else if (nsCSSPseudoClasses::mozLoading == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_LOADING;
    }
    else if (nsCSSPseudoClasses::mozTypeUnsupported == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_TYPE_UNSUPPORTED;
    }
    else if (nsCSSPseudoClasses::defaultPseudo == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_DEFAULT;
    }
    else if (nsCSSPseudoClasses::required == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_REQUIRED;
    }
    else if (nsCSSPseudoClasses::optional == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_OPTIONAL;
    }
    else if (nsCSSPseudoClasses::valid == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_VALID;
    }
    else if (nsCSSPseudoClasses::invalid == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_INVALID;
    }
    else if (nsCSSPseudoClasses::inRange == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_INRANGE;
    }
    else if (nsCSSPseudoClasses::outOfRange == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_OUTOFRANGE;
    }
    else if (nsCSSPseudoClasses::mozReadOnly == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_MOZ_READONLY;
    }
    else if (nsCSSPseudoClasses::mozReadWrite == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_MOZ_READWRITE;
    }
    else if (nsCSSPseudoClasses::mozIsHTML == pseudoClass->mAtom) {
      result = data.mIsHTMLContent &&
        data.mContent->GetNameSpaceID() == kNameSpaceID_None;
    }
#ifdef MOZ_MATHML
    else if (nsCSSPseudoClasses::mozMathIncrementScriptLevel == pseudoClass->mAtom) {
      stateToCheck = NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL;
    }
#endif
    else {
      NS_ERROR("CSS parser parsed a pseudo-class that we do not handle");
      result = PR_FALSE;  
    }
    if (stateToCheck) {
      
      if ((stateToCheck & (NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE)) &&
          data.mCompatMode == eCompatibility_NavQuirks &&
          
          !aSelector->mTag && !aSelector->mIDList && !aSelector->mAttrList &&
          
          
          
          
          !isNegated &&
          
          data.mIsHTMLContent && !data.mIsLink &&
          !IsQuirkEventSensitive(data.mContentTag)) {
        
        
        result = PR_FALSE;
      } else {
        if (aStateMask & stateToCheck) {
          result = PR_TRUE;
          if (aDependence)
            *aDependence = PR_TRUE;
        } else {
          result = (0 != (data.mEventState & stateToCheck));
        }
      }
    }
  }

  if (result && aSelector->mAttrList) {
    
    if (!data.mHasAttributes && !aAttribute) {
      
      result = PR_FALSE;
    } else {
      NS_ASSERTION(data.mContent,
                   "Must have content if either data.mHasAttributes or "
                   "aAttribute is set!");
      result = PR_TRUE;
      nsAttrSelector* attr = aSelector->mAttrList;
      do {
        if (attr->mAttr == aAttribute) {
          
          
          result = PR_TRUE;
          if (aDependence)
            *aDependence = PR_TRUE;
        }
        else if (attr->mNameSpace == kNameSpaceID_Unknown) {
          
          
          
          
          
          
          
          PRUint32 attrCount = data.mContent->GetAttrCount();
          result = PR_FALSE;
          for (PRUint32 i = 0; i < attrCount; ++i) {
            const nsAttrName* attrName =
              data.mContent->GetAttrNameAt(i);
            NS_ASSERTION(attrName, "GetAttrCount lied or GetAttrNameAt failed");
            if (attrName->LocalName() != attr->mAttr) {
              continue;
            }
            if (attr->mFunction == NS_ATTR_FUNC_SET) {
              result = PR_TRUE;
            } else {
              nsAutoString value;
#ifdef DEBUG
              PRBool hasAttr =
#endif
                data.mContent->GetAttr(attrName->NamespaceID(),
                                       attrName->LocalName(), value);
              NS_ASSERTION(hasAttr, "GetAttrNameAt lied");
              result = AttrMatchesValue(attr, value);
            }

            
            
            
            
            
            if (result) {
              break;
            }
          }
        }
        else if (attr->mFunction == NS_ATTR_FUNC_EQUALS) {
          result =
            data.mContent->
              AttrValueIs(attr->mNameSpace, attr->mAttr, attr->mValue,
                          attr->mCaseSensitive ? eCaseMatters : eIgnoreCase);
        }
        else if (!data.mContent->HasAttr(attr->mNameSpace, attr->mAttr)) {
          result = PR_FALSE;
        }
        else if (attr->mFunction != NS_ATTR_FUNC_SET) {
          nsAutoString value;
#ifdef DEBUG
          PRBool hasAttr =
#endif
              data.mContent->GetAttr(attr->mNameSpace, attr->mAttr, value);
          NS_ASSERTION(hasAttr, "HasAttr lied");
          result = AttrMatchesValue(attr, value);
        }
        
        attr = attr->mNext;
      } while (attr && result);
    }
  }
  nsAtomList* IDList = aSelector->mIDList;
  if (result && IDList) {
    
    result = PR_FALSE;

    if (aAttribute && aAttribute == data.mContent->GetIDAttributeName()) {
      result = PR_TRUE;
      if (aDependence)
        *aDependence = PR_TRUE;
    }
    else if (nsnull != data.mContentID) {
      
      const PRBool isCaseSensitive =
        data.mCompatMode != eCompatibility_NavQuirks;

      result = PR_TRUE;
      if (isCaseSensitive) {
        do {
          if (IDList->mAtom != data.mContentID) {
            result = PR_FALSE;
            break;
          }
          IDList = IDList->mNext;
        } while (IDList);
      } else {
        const char* id1Str;
        data.mContentID->GetUTF8String(&id1Str);
        nsDependentCString id1(id1Str);
        do {
          const char* id2Str;
          IDList->mAtom->GetUTF8String(&id2Str);
          if (!id1.Equals(id2Str, nsCaseInsensitiveCStringComparator())) {
            result = PR_FALSE;
            break;
          }
          IDList = IDList->mNext;
        } while (IDList);
      }
    }
  }
    
  if (result && aSelector->mClassList) {
    
    if (aAttribute && aAttribute == data.mContent->GetClassAttributeName()) {
      result = PR_TRUE;
      if (aDependence)
        *aDependence = PR_TRUE;
    }
    else {
      
      const PRBool isCaseSensitive =
        data.mCompatMode != eCompatibility_NavQuirks;

      nsAtomList* classList = aSelector->mClassList;
      const nsAttrValue *elementClasses = data.mClasses;
      while (nsnull != classList) {
        if (!(elementClasses && elementClasses->Contains(classList->mAtom, isCaseSensitive ? eCaseMatters : eIgnoreCase))) {
          result = PR_FALSE;
          break;
        }
        classList = classList->mNext;
      }
    }
  }
  
  
  if (!isNegated) {
    for (nsCSSSelector *negation = aSelector->mNegations;
         result && negation; negation = negation->mNegations) {
      PRBool dependence = PR_FALSE;
      result = !SelectorMatches(data, negation, aStateMask,
                                aAttribute, aForStyling, &dependence);
      
      
      
      
      
      result = result || dependence;
    }
  }
  return result;
}

#undef STATE_CHECK





#define NS_IS_GREEDY_OPERATOR(ch) (ch == PRUnichar(0) || ch == PRUnichar('~'))

static PRBool SelectorMatchesTree(RuleProcessorData& aPrevData,
                                  nsCSSSelector* aSelector,
                                  PRBool aForStyling) 
{
  nsCSSSelector* selector = aSelector;
  RuleProcessorData* prevdata = &aPrevData;
  while (selector) { 
    
    
    

    
    
    RuleProcessorData* data;
    if (PRUnichar('+') == selector->mOperator ||
        PRUnichar('~') == selector->mOperator) {
      data = prevdata->mPreviousSiblingData;
      if (!data) {
        nsIContent* content = prevdata->mContent;
        nsIContent* parent = content->GetParent();
        if (parent) {
          parent->SetFlags(NODE_HAS_SLOW_SELECTOR_NOAPPEND);

          PRInt32 index = parent->IndexOf(content);
          while (0 <= --index) {
            content = parent->GetChildAt(index);
            if (content->IsNodeOfType(nsINode::eELEMENT)) {
              data = RuleProcessorData::Create(prevdata->mPresContext, content,
                                               prevdata->mRuleWalker,
                                               prevdata->mCompatMode);
              prevdata->mPreviousSiblingData = data;    
              break;
            }
          }
        }
      }
    }
    
    
    else {
      data = prevdata->mParentData;
      if (!data) {
        nsIContent *content = prevdata->mContent->GetParent();
        
        
        if (content && content->IsNodeOfType(nsINode::eELEMENT)) {
          data = RuleProcessorData::Create(prevdata->mPresContext, content,
                                           prevdata->mRuleWalker,
                                           prevdata->mCompatMode);
          prevdata->mParentData = data;    
        }
      }
    }
    if (! data) {
      return PR_FALSE;
    }
    if (SelectorMatches(*data, selector, 0, nsnull, aForStyling)) {
      
      
      
      
      
      if ((NS_IS_GREEDY_OPERATOR(selector->mOperator)) &&
          (selector->mNext) &&
          (selector->mNext->mOperator != selector->mOperator) &&
          !(selector->mOperator == '~' &&
            selector->mNext->mOperator == PRUnichar(0))) {

        
        

        
        
        
        
        if (SelectorMatchesTree(*data, selector, aForStyling)) {
          return PR_TRUE;
        }
      }
      selector = selector->mNext;
    }
    else {
      
      
      if (!NS_IS_GREEDY_OPERATOR(selector->mOperator)) {
        return PR_FALSE;  
      }
    }
    prevdata = data;
  }
  return PR_TRUE; 
}

static void ContentEnumFunc(nsICSSStyleRule* aRule, nsCSSSelector* aSelector,
                            void* aData)
{
  ElementRuleProcessorData* data = (ElementRuleProcessorData*)aData;

  if (SelectorMatches(*data, aSelector, 0, nsnull, PR_TRUE)) {
    nsCSSSelector *next = aSelector->mNext;
    if (!next || SelectorMatchesTree(*data, next, PR_TRUE)) {
      
      
      
#ifdef DEBUG
      nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
      NS_ASSERTION(static_cast<nsIStyleRule*>(aRule) == iRule.get(),
                   "Please fix QI so this performance optimization is valid");
#endif
      data->mRuleWalker->Forward(static_cast<nsIStyleRule*>(aRule));
      
    }
  }
}

NS_IMETHODIMP
nsCSSRuleProcessor::RulesMatching(ElementRuleProcessorData *aData)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    cascade->mRuleHash.EnumerateAllRules(aData->mNameSpaceID,
                                         aData->mContentTag,
                                         aData->mContentID,
                                         aData->mClasses,
                                         ContentEnumFunc,
                                         aData);
  }
  return NS_OK;
}

static void PseudoEnumFunc(nsICSSStyleRule* aRule, nsCSSSelector* aSelector,
                           void* aData)
{
  PseudoRuleProcessorData* data = (PseudoRuleProcessorData*)aData;

  NS_ASSERTION(aSelector->mTag == data->mPseudoTag, "RuleHash failure");
  PRBool matches = PR_TRUE;
  if (data->mComparator)
    data->mComparator->PseudoMatches(data->mPseudoTag, aSelector, &matches);

  if (matches) {
    nsCSSSelector *selector = aSelector->mNext;

    if (selector) { 
      if (PRUnichar('+') == selector->mOperator) {
        return; 
      }
      if (SelectorMatches(*data, selector, 0, nsnull, PR_TRUE)) {
        selector = selector->mNext;
      }
      else {
        if (PRUnichar('>') == selector->mOperator) {
          return; 
        }
      }
    }

    if (selector && 
        (! SelectorMatchesTree(*data, selector, PR_TRUE))) {
      return; 
    }

    
    
    
#ifdef DEBUG
    nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
    NS_ASSERTION(static_cast<nsIStyleRule*>(aRule) == iRule.get(),
                 "Please fix QI so this performance optimization is valid");
#endif
    data->mRuleWalker->Forward(static_cast<nsIStyleRule*>(aRule));
    
  }
}

NS_IMETHODIMP
nsCSSRuleProcessor::RulesMatching(PseudoRuleProcessorData* aData)
{
  NS_PRECONDITION(!aData->mContent ||
                  aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content (if present) must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    cascade->mRuleHash.EnumerateTagRules(aData->mPseudoTag,
                                         PseudoEnumFunc, aData);
  }
  return NS_OK;
}

inline PRBool
IsSiblingOperator(PRUnichar oper)
{
  return oper == PRUnichar('+') || oper == PRUnichar('~');
}

struct StateEnumData {
  StateEnumData(StateRuleProcessorData *aData)
    : data(aData), change(nsReStyleHint(0)) {}

  StateRuleProcessorData *data;
  nsReStyleHint change;
};

PR_STATIC_CALLBACK(PRBool) StateEnumFunc(void* aSelector, void* aData)
{
  StateEnumData *enumData = static_cast<StateEnumData*>(aData);
  StateRuleProcessorData *data = enumData->data;
  nsCSSSelector* selector = static_cast<nsCSSSelector*>(aSelector);

  nsReStyleHint possibleChange = IsSiblingOperator(selector->mOperator) ?
    eReStyle_LaterSiblings : eReStyle_Self;

  
  
  
  if ((possibleChange & ~(enumData->change)) &&
      SelectorMatches(*data, selector, data->mStateMask, nsnull, PR_TRUE) &&
      SelectorMatchesTree(*data, selector->mNext, PR_TRUE)) {
    enumData->change = nsReStyleHint(enumData->change | possibleChange);
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsCSSRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData,
                                           nsReStyleHint* aResult)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  
  
  StateEnumData data(aData);
  if (cascade)
    cascade->mStateSelectors.EnumerateForwards(StateEnumFunc, &data);
  *aResult = data.change;
  return NS_OK;
}

struct AttributeEnumData {
  AttributeEnumData(AttributeRuleProcessorData *aData)
    : data(aData), change(nsReStyleHint(0)) {}

  AttributeRuleProcessorData *data;
  nsReStyleHint change;
};


PR_STATIC_CALLBACK(PRBool) AttributeEnumFunc(void* aSelector, void* aData)
{
  AttributeEnumData *enumData = static_cast<AttributeEnumData*>(aData);
  AttributeRuleProcessorData *data = enumData->data;
  nsCSSSelector* selector = static_cast<nsCSSSelector*>(aSelector);

  nsReStyleHint possibleChange = IsSiblingOperator(selector->mOperator) ?
    eReStyle_LaterSiblings : eReStyle_Self;

  
  
  
  if ((possibleChange & ~(enumData->change)) &&
      SelectorMatches(*data, selector, data->mStateMask, data->mAttribute,
                      PR_TRUE) &&
      SelectorMatchesTree(*data, selector->mNext, PR_TRUE)) {
    enumData->change = nsReStyleHint(enumData->change | possibleChange);
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsCSSRuleProcessor::HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                               nsReStyleHint* aResult)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  AttributeEnumData data(aData);

  
  
  
  if (aData->mAttribute == nsGkAtoms::href &&
      aData->mIsHTMLContent &&
      (aData->mContentTag == nsGkAtoms::a ||
       aData->mContentTag == nsGkAtoms::area ||
       aData->mContentTag == nsGkAtoms::link)) {
    data.change = nsReStyleHint(data.change | eReStyle_Self);
  }
  
#ifdef MOZ_SVG
  
  if (aData->mAttribute == nsGkAtoms::href &&
      aData->mNameSpaceID == kNameSpaceID_SVG &&
      aData->mContentTag == nsGkAtoms::a) {
    data.change = nsReStyleHint(data.change | eReStyle_Self);
  }
#endif
  
  

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  

  if (cascade) {
    if (aData->mAttribute == aData->mContent->GetIDAttributeName()) {
      cascade->mIDSelectors.EnumerateForwards(AttributeEnumFunc, &data);
    }
    
    if (aData->mAttribute == aData->mContent->GetClassAttributeName()) {
      cascade->mClassSelectors.EnumerateForwards(AttributeEnumFunc, &data);
    }

    AttributeSelectorEntry *entry = static_cast<AttributeSelectorEntry*>
                                               (PL_DHashTableOperate(&cascade->mAttributeSelectors, aData->mAttribute,
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      entry->mSelectors->EnumerateForwards(AttributeEnumFunc, &data);
    }
  }

  *aResult = data.change;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSRuleProcessor::MediumFeaturesChanged(nsPresContext* aPresContext,
                                          PRBool* aRulesChanged)
{
  RuleCascadeData *old = mRuleCascades;
  RefreshRuleCascade(aPresContext);
  *aRulesChanged = (old != mRuleCascades);
  return NS_OK;
}

nsresult
nsCSSRuleProcessor::ClearRuleCascades()
{
  RuleCascadeData *data = mRuleCascades;
  mRuleCascades = nsnull;
  while (data) {
    RuleCascadeData *next = data->mNext;
    delete data;
    data = next;
  }
  return NS_OK;
}




inline
PRBool IsStateSelector(nsCSSSelector& aSelector)
{
  for (nsPseudoClassList* pseudoClass = aSelector.mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    if ((pseudoClass->mAtom == nsCSSPseudoClasses::active) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::checked) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozDragOver) || 
        (pseudoClass->mAtom == nsCSSPseudoClasses::focus) || 
        (pseudoClass->mAtom == nsCSSPseudoClasses::hover) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::target) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::link) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::visited) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::enabled) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::disabled) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozBroken) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozUserDisabled) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozSuppressed) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozLoading) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::required) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::optional) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::valid) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::invalid) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::inRange) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::outOfRange) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozReadOnly) ||
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozReadWrite) ||
#ifdef MOZ_MATHML
        (pseudoClass->mAtom == nsCSSPseudoClasses::mozMathIncrementScriptLevel) ||
#endif
        (pseudoClass->mAtom == nsCSSPseudoClasses::defaultPseudo)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static PRBool
AddRule(RuleValue* aRuleInfo, void* aCascade)
{
  RuleCascadeData *cascade = static_cast<RuleCascadeData*>(aCascade);

  
  cascade->mRuleHash.PrependRule(aRuleInfo);

  nsVoidArray* stateArray = &cascade->mStateSelectors;
  nsVoidArray* classArray = &cascade->mClassSelectors;
  nsVoidArray* idArray = &cascade->mIDSelectors;
  
  for (nsCSSSelector* selector = aRuleInfo->mSelector;
           selector; selector = selector->mNext) {
    
    
    
    
    
    
    
    
    for (nsCSSSelector* negation = selector; negation;
         negation = negation->mNegations) {
      
      if (IsStateSelector(*negation))
        stateArray->AppendElement(selector);

      
      if (negation->mIDList) {
        idArray->AppendElement(selector);
      }
      
      
      if (negation->mClassList) {
        classArray->AppendElement(selector);
      }

      
      for (nsAttrSelector *attr = negation->mAttrList; attr;
           attr = attr->mNext) {
        nsVoidArray *array = cascade->AttributeListFor(attr->mAttr);
        if (!array)
          return PR_FALSE;
        array->AppendElement(selector);
      }
    }
  }

  return PR_TRUE;
}

struct PerWeightData {
  PRInt32 mWeight;
  RuleValue* mRules; 
};

struct RuleByWeightEntry : public PLDHashEntryHdr {
  PerWeightData data; 
};

PR_STATIC_CALLBACK(PLDHashNumber)
HashIntKey(PLDHashTable *table, const void *key)
{
  return PLDHashNumber(NS_PTR_TO_INT32(key));
}

PR_STATIC_CALLBACK(PRBool)
MatchWeightEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                 const void *key)
{
  const RuleByWeightEntry *entry = (const RuleByWeightEntry *)hdr;
  return entry->data.mWeight == NS_PTR_TO_INT32(key);
}

static PLDHashTableOps gRulesByWeightOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIntKey,
    MatchWeightEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    NULL
};

struct CascadeEnumData {
  CascadeEnumData(nsPresContext* aPresContext,
                  nsMediaQueryResultCacheKey& aKey,
                  PLArenaPool& aArena)
    : mPresContext(aPresContext),
      mCacheKey(aKey),
      mArena(aArena)
  {
    if (!PL_DHashTableInit(&mRulesByWeight, &gRulesByWeightOps, nsnull,
                          sizeof(RuleByWeightEntry), 64))
      mRulesByWeight.ops = nsnull;
  }

  ~CascadeEnumData()
  {
    if (mRulesByWeight.ops)
      PL_DHashTableFinish(&mRulesByWeight);
  }

  nsPresContext* mPresContext;
  nsMediaQueryResultCacheKey& mCacheKey;
  
  
  PLDHashTable mRulesByWeight; 
  PLArenaPool& mArena;
};

static PRBool
InsertRuleByWeight(nsICSSRule* aRule, void* aData)
{
  CascadeEnumData* data = (CascadeEnumData*)aData;
  PRInt32 type = nsICSSRule::UNKNOWN_RULE;
  aRule->GetType(type);

  if (nsICSSRule::STYLE_RULE == type) {
    nsICSSStyleRule* styleRule = (nsICSSStyleRule*)aRule;

    for (nsCSSSelectorList *sel = styleRule->Selector();
         sel; sel = sel->mNext) {
      PRInt32 weight = sel->mWeight;
      RuleByWeightEntry *entry = static_cast<RuleByWeightEntry*>(
        PL_DHashTableOperate(&data->mRulesByWeight, NS_INT32_TO_PTR(weight),
                             PL_DHASH_ADD));
      if (!entry)
        return PR_FALSE;
      entry->data.mWeight = weight;
      RuleValue *info =
        new (data->mArena) RuleValue(styleRule, sel->mSelectors);
      
      info->mNext = entry->data.mRules;
      entry->data.mRules = info;
    }
  }
  else if (nsICSSRule::MEDIA_RULE == type ||
           nsICSSRule::DOCUMENT_RULE == type) {
    nsICSSGroupRule* groupRule = (nsICSSGroupRule*)aRule;
    if (groupRule->UseForPresentation(data->mPresContext, data->mCacheKey))
      if (!groupRule->EnumerateRulesForwards(InsertRuleByWeight, aData))
        return PR_FALSE;
  }
  return PR_TRUE;
}


static PRBool
CascadeSheetRulesInto(nsICSSStyleSheet* aSheet, void* aData)
{
  nsCSSStyleSheet*  sheet = static_cast<nsCSSStyleSheet*>(aSheet);
  CascadeEnumData* data = static_cast<CascadeEnumData*>(aData);
  PRBool bSheetApplicable = PR_TRUE;
  sheet->GetApplicable(bSheetApplicable);

  if (bSheetApplicable &&
      sheet->UseForPresentation(data->mPresContext, data->mCacheKey)) {
    nsCSSStyleSheet* child = sheet->mFirstChild;
    while (child) {
      CascadeSheetRulesInto(child, data);
      child = child->mNext;
    }

    if (sheet->mInner) {
      if (!sheet->mInner->mOrderedRules.EnumerateForwards(InsertRuleByWeight, data))
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PR_STATIC_CALLBACK(int) CompareWeightData(const void* aArg1, const void* aArg2,
                                         void* closure)
{
  const PerWeightData* arg1 = static_cast<const PerWeightData*>(aArg1);
  const PerWeightData* arg2 = static_cast<const PerWeightData*>(aArg2);
  return arg1->mWeight - arg2->mWeight; 
}


struct FillWeightArrayData {
  FillWeightArrayData(PerWeightData* aArrayData) :
    mIndex(0),
    mWeightArray(aArrayData)
  {
  }
  PRInt32 mIndex;
  PerWeightData* mWeightArray;
};


PR_STATIC_CALLBACK(PLDHashOperator)
FillWeightArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                PRUint32 number, void *arg)
{
  FillWeightArrayData* data = static_cast<FillWeightArrayData*>(arg);
  const RuleByWeightEntry *entry = (const RuleByWeightEntry *)hdr;

  data->mWeightArray[data->mIndex++] = entry->data;

  return PL_DHASH_NEXT;
}

RuleCascadeData*
nsCSSRuleProcessor::GetRuleCascade(nsPresContext* aPresContext)
{
  
  
  
  
  
  

  if (!mRuleCascades || aPresContext != mLastPresContext) {
    RefreshRuleCascade(aPresContext);
  }
  mLastPresContext = aPresContext;

  return mRuleCascades;
}

void
nsCSSRuleProcessor::RefreshRuleCascade(nsPresContext* aPresContext)
{
  
  
  
  
  

  for (RuleCascadeData **cascadep = &mRuleCascades, *cascade;
       (cascade = *cascadep); cascadep = &cascade->mNext) {
    if (cascade->mCacheKey.Matches(aPresContext)) {
      
      *cascadep = cascade->mNext;
      cascade->mNext = mRuleCascades;
      mRuleCascades = cascade;

      return;
    }
  }

  if (mSheets.Count() != 0) {
    nsAutoPtr<RuleCascadeData> newCascade(
      new RuleCascadeData(aPresContext->Medium(),
                          eCompatibility_NavQuirks == aPresContext->CompatibilityMode()));
    if (newCascade) {
      CascadeEnumData data(aPresContext, newCascade->mCacheKey,
                           newCascade->mRuleHash.Arena());
      if (!data.mRulesByWeight.ops)
        return; 
      if (!mSheets.EnumerateForwards(CascadeSheetRulesInto, &data))
        return; 

      
      PRUint32 weightCount = data.mRulesByWeight.entryCount;
      nsAutoArrayPtr<PerWeightData> weightArray(new PerWeightData[weightCount]);
      FillWeightArrayData fwData(weightArray);
      PL_DHashTableEnumerate(&data.mRulesByWeight, FillWeightArray, &fwData);
      NS_QuickSort(weightArray, weightCount, sizeof(PerWeightData),
                   CompareWeightData, nsnull);

      
      
      
      PRUint32 i = weightCount;
      while (i > 0) {
        --i;
        
        RuleValue *ruleValue = weightArray[i].mRules;
        do {
          
          RuleValue *next = ruleValue->mNext;
          if (!AddRule(ruleValue, newCascade))
            return; 
          ruleValue = next;
        } while (ruleValue);
      }

      
      newCascade->mNext = mRuleCascades;
      mRuleCascades = newCascade.forget();
    }
  }
  return;
}

 PRBool
nsCSSRuleProcessor::SelectorListMatches(RuleProcessorData& aData,
                                        nsCSSSelectorList* aSelectorList)
{
  while (aSelectorList) {
    nsCSSSelector* sel = aSelectorList->mSelectors;
    NS_ASSERTION(sel, "Should have *some* selectors");
    if (SelectorMatches(aData, sel, 0, nsnull, PR_FALSE)) {
      nsCSSSelector* next = sel->mNext;
      if (!next || SelectorMatchesTree(aData, next, PR_FALSE)) {
        return PR_TRUE;
      }
    }

    aSelectorList = aSelectorList->mNext;
  }

  return PR_FALSE;
}
