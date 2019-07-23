














































#include "nsCSSRuleProcessor.h"
#include "nsRuleProcessorData.h"

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
#include "nsDOMError.h"
#include "nsRuleWalker.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSPseudoElements.h"
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
#include "nsCSSRules.h"
#include "nsIPrincipal.h"
#include "nsStyleSet.h"
#include "prlog.h"

#define VISITED_PSEUDO_PREF "layout.css.visited_links_enabled"

static PRBool gSupportVisitedPseudo = PR_TRUE;

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

struct RuleHashTagTableEntry : public RuleHashTableEntry {
  nsCOMPtr<nsIAtom> mTag;
};

static PLDHashNumber
RuleHash_CIHashKey(PLDHashTable *table, const void *key)
{
  nsIAtom *atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>(key));

  nsAutoString str;
  atom->ToString(str);
  ToUpperCase(str);
  return HashString(str);
}

typedef nsIAtom*
(* RuleHashGetKey) (PLDHashTable *table, const PLDHashEntryHdr *entry);

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

static PRBool
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

static PRBool
RuleHash_CSMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  
  nsIAtom *entry_atom = ToLocalOps(table->ops)->getKey(table, hdr);

  return match_atom == entry_atom;
}

static PRBool
RuleHash_TagTable_MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  nsIAtom *entry_atom = static_cast<const RuleHashTagTableEntry*>(hdr)->mTag;

  return match_atom == entry_atom;
}

static void RuleHash_TagTable_ClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  (static_cast<RuleHashTagTableEntry*>(entry))->~RuleHashTagTableEntry();
}

static nsIAtom*
RuleHash_ClassTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules->mSelector->mClassList->mAtom;
}

static nsIAtom*
RuleHash_IdTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules->mSelector->mIDList->mAtom;
}

static PLDHashNumber
RuleHash_NameSpaceTable_HashKey(PLDHashTable *table, const void *key)
{
  return NS_PTR_TO_INT32(key);
}

static PRBool
RuleHash_NameSpaceTable_MatchEntry(PLDHashTable *table,
                                   const PLDHashEntryHdr *hdr,
                                   const void *key)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);

  return NS_PTR_TO_INT32(key) ==
         entry->mRules->mSelector->mNameSpace;
}

static const PLDHashTableOps RuleHash_TagTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_TagTable_MatchEntry,
  PL_DHashMoveEntryStub,
  RuleHash_TagTable_ClearEntry,
  PL_DHashFinalizeStub,
  NULL
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
                         RuleEnumFunc aFunc, RuleProcessorData* aData);
  PLArenaPool& Arena() { return mArena; }

protected:
  void PrependRuleToTable(PLDHashTable* aTable, const void* aKey,
                          RuleValue* aRuleInfo);
  void PrependRuleToTagTable(nsIAtom* aKey, RuleValue* aRuleInfo);
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

  PRUint32    mElementUniversalCalls;
  PRUint32    mElementNameSpaceCalls;
  PRUint32    mElementTagCalls;
  PRUint32    mElementClassCalls;
  PRUint32    mElementIdCalls;
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
    mElementUniversalCalls(0),
    mElementNameSpaceCalls(0),
    mElementTagCalls(0),
    mElementClassCalls(0),
    mElementIdCalls(0)
#endif
{
  MOZ_COUNT_CTOR(RuleHash);
  
  PL_INIT_ARENA_POOL(&mArena, "RuleHashArena", NS_RULEHASH_ARENA_BLOCK_SIZE);

  PL_DHashTableInit(&mTagTable, &RuleHash_TagTable_Ops, nsnull,
                    sizeof(RuleHashTagTableEntry), 64);
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
  MOZ_COUNT_DTOR(RuleHash);
#ifdef RULE_HASH_STATS
  printf(
"RuleHash(%p):\n"
"  Selectors: Universal (%u) NameSpace(%u) Tag(%u) Class(%u) Id(%u)\n"
"  Content Nodes: Elements(%u)\n"
"  Element Calls: Universal(%u) NameSpace(%u) Tag(%u) Class(%u) Id(%u)\n"
         static_cast<void*>(this),
         mUniversalSelectors, mNameSpaceSelectors, mTagSelectors,
           mClassSelectors, mIdSelectors,
         mElementsMatched,
         mElementUniversalCalls, mElementNameSpaceCalls, mElementTagCalls,
           mElementClassCalls, mElementIdCalls);
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

static void
DoPrependRuleToTagTable(PLDHashTable* aTable, nsIAtom* aKey,
                        RuleValue* aRuleInfo, PRInt32 aBackwardsIndex)
{
  
  RuleHashTagTableEntry *entry = static_cast<RuleHashTagTableEntry*>
    (PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
  if (!entry)
    return;

  entry->mTag = aKey;

  
  
  entry->mRules = aRuleInfo->Add(aBackwardsIndex, entry->mRules);
}

void RuleHash::PrependRuleToTagTable(nsIAtom* aKey, RuleValue* aRuleInfo)
{
  DoPrependRuleToTagTable(&mTagTable, aKey, aRuleInfo, mRuleCount++);
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
  else if (selector->mLowercaseTag) {
    PrependRuleToTagTable(selector->mLowercaseTag, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mTagSelectors);
    if (selector->mCasedTag && 
        selector->mCasedTag != selector->mLowercaseTag) {
      PrependRuleToTagTable(selector->mCasedTag, 
                            new (mArena) RuleValue(aRuleInfo->mRule, 
                                                   aRuleInfo->mSelector));
      RULE_HASH_STAT_INCREMENT(mTagSelectors);
    }
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
                                 RuleEnumFunc aFunc, RuleProcessorData* aData)
{
  PRInt32 classCount = aClassList ? aClassList->GetAtomCount() : 0;

  
  
  PRInt32 testCount = classCount + 4;

  if (mEnumListSize < testCount) {
    delete [] mEnumList;
    mEnumListSize = NS_MAX(testCount, MIN_ENUM_LIST_SIZE);
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
  
  if (kNameSpaceID_Unknown != aNameSpace && mNameSpaceTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mNameSpaceTable, NS_INT32_TO_PTR(aNameSpace),
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementNameSpaceCalls);
    }
  }
  if (aTag && mTagTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mTagTable, aTag, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementTagCalls);
    }
  }
  if (aID && mIdTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mIdTable, aID, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementIdCalls);
    }
  }
  if (mClassTable.entryCount) {
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




struct AttributeSelectorEntry : public PLDHashEntryHdr {
  nsIAtom *mAttribute;
  nsTArray<nsCSSSelector*> *mSelectors;
};

static void
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
      mNext(nsnull),
      mQuirksMode(aQuirksMode)
  {
    PL_DHashTableInit(&mAttributeSelectors, &AttributeSelectorOps, nsnull,
                      sizeof(AttributeSelectorEntry), 16);
    PL_DHashTableInit(&mAnonBoxRules, &RuleHash_TagTable_Ops, nsnull,
                      sizeof(RuleHashTagTableEntry), 16);
    memset(mPseudoElementRuleHashes, 0, sizeof(mPseudoElementRuleHashes));
#ifdef MOZ_XUL
    PL_DHashTableInit(&mXULTreeRules, &RuleHash_TagTable_Ops, nsnull,
                      sizeof(RuleHashTagTableEntry), 16);
#endif
  }

  ~RuleCascadeData()
  {
    PL_DHashTableFinish(&mAttributeSelectors);
    PL_DHashTableFinish(&mAnonBoxRules);
    PL_DHashTableFinish(&mXULTreeRules);
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mPseudoElementRuleHashes); ++i) {
      delete mPseudoElementRuleHashes[i];
    }
  }
  RuleHash                 mRuleHash;
  RuleHash*
    mPseudoElementRuleHashes[nsCSSPseudoElements::ePseudo_PseudoElementCount];
  nsTArray<nsCSSSelector*> mStateSelectors;
  nsTArray<nsCSSSelector*> mClassSelectors;
  nsTArray<nsCSSSelector*> mIDSelectors;
  PLDHashTable             mAttributeSelectors;
  PLDHashTable             mAnonBoxRules;
#ifdef MOZ_XUL
  PLDHashTable             mXULTreeRules;
#endif

  nsTArray<nsFontFaceRuleContainer> mFontFaceRules;

  
  
  nsTArray<nsCSSSelector*>* AttributeListFor(nsIAtom* aAttribute);

  nsMediaQueryResultCacheKey mCacheKey;
  RuleCascadeData*  mNext; 

  const PRBool mQuirksMode;
};

nsTArray<nsCSSSelector*>*
RuleCascadeData::AttributeListFor(nsIAtom* aAttribute)
{
  AttributeSelectorEntry *entry = static_cast<AttributeSelectorEntry*>
                                             (PL_DHashTableOperate(&mAttributeSelectors, aAttribute, PL_DHASH_ADD));
  if (!entry)
    return nsnull;
  if (!entry->mSelectors) {
    if (!(entry->mSelectors = new nsTArray<nsCSSSelector*>)) {
      PL_DHashTableRawRemove(&mAttributeSelectors, entry);
      return nsnull;
    }
    entry->mAttribute = aAttribute;
  }
  return entry->mSelectors;
}





nsCSSRuleProcessor::nsCSSRuleProcessor(const nsCOMArray<nsICSSStyleSheet>& aSheets,
                                       PRUint8 aSheetType)
  : mSheets(aSheets)
  , mRuleCascades(nsnull)
  , mLastPresContext(nsnull)
  , mSheetType(aSheetType)
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

 void
nsCSSRuleProcessor::Startup()
{
  nsContentUtils::AddBoolPrefVarCache(VISITED_PSEUDO_PREF,
                                      &gSupportVisitedPseudo);
  
  gSupportVisitedPseudo =
    nsContentUtils::GetBoolPref(VISITED_PSEUDO_PREF, PR_TRUE);
}

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
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_start_backward);
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowStartForward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_start_forward);
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowEndBackward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_end_backward);
  }
  if (metricResult & nsILookAndFeel::eMetric_ScrollArrowEndForward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_end_forward);
  }

  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ScrollSliderStyle, metricResult);
  if (metricResult != nsILookAndFeel::eMetric_ScrollThumbStyleNormal) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_thumb_proportional);
  }

  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ImagesInMenus, metricResult);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::images_in_menus);
  }

  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ImagesInButtons, metricResult);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::images_in_buttons);
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_WindowsDefaultTheme, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_default_theme);
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_MacGraphiteTheme, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::mac_graphite_theme);
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_DWMCompositor, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_compositor);
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_WindowsClassic, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_classic);
  }

  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TouchEnabled, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::touch_enabled);
  }
 
  rv = lookAndFeel->GetMetric(nsILookAndFeel::eMetric_MaemoClassic, metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::maemo_classic);
  }

  return PR_TRUE;
}

 void
nsCSSRuleProcessor::FreeSystemMetrics()
{
  delete sSystemMetrics;
  sSystemMetrics = nsnull;
}

 PRBool
nsCSSRuleProcessor::HasSystemMetric(nsIAtom* aMetric)
{
  if (!sSystemMetrics && !InitSystemMetrics()) {
    return PR_FALSE;
  }
  return sSystemMetrics->IndexOf(aMetric) != sSystemMetrics->NoIndex;
}

RuleProcessorData::RuleProcessorData(nsPresContext* aPresContext,
                                     nsIContent* aContent, 
                                     nsRuleWalker* aRuleWalker,
                                     nsCompatibility* aCompat )
  : mPresContext(aPresContext),
    mContent(aContent),
    mRuleWalker(aRuleWalker),
    mScopedRoot(nsnull),
    mPreviousSiblingData(nsnull),
    mParentData(nsnull),
    mLanguage(nsnull),
    mGotContentState(PR_FALSE),
    mGotLinkInfo(PR_FALSE)
{
  MOZ_COUNT_CTOR(RuleProcessorData);

  NS_ASSERTION(aContent && aContent->IsNodeOfType(nsINode::eELEMENT),
               "non-element leaked into SelectorMatches");

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

  NS_ASSERTION(aContent->GetOwnerDoc(), "Document-less node here?");
    
  
  mContentTag = aContent->Tag();
  mParentContent = aContent->GetParent();

  
  mHasAttributes = aContent->GetAttrCount() > 0;
  if (mHasAttributes) {
    
    mContentID = aContent->GetID();
    mClasses = aContent->GetClasses();
  } else {
    mContentID = nsnull;
    mClasses = nsnull;
  }

  
  mNameSpaceID = aContent->GetNameSpaceID();

  
  mIsHTMLContent = (mNameSpaceID == kNameSpaceID_XHTML);
  mIsHTML = mIsHTMLContent && aContent->IsInHTMLDocument();

  
  

  
  
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
    mLanguage = new nsString();
    if (!mLanguage)
      return nsnull;
    for (nsIContent* content = mContent; content;
         content = content->GetParent()) {
      if (content->GetAttrCount() > 0) {
        
        
        PRBool hasAttr = content->GetAttr(kNameSpaceID_XML, nsGkAtoms::lang,
                                          *mLanguage);
        if (!hasAttr && content->IsHTML()) {
          hasAttr = content->GetAttr(kNameSpaceID_None, nsGkAtoms::lang,
                                     *mLanguage);
        }
        NS_ASSERTION(hasAttr || mLanguage->IsEmpty(),
                     "GetAttr that returns false should not make string non-empty");
        if (hasAttr) {
          break;
        }
      }
    }
  }
  return mLanguage;
}

PRUint32
RuleProcessorData::ContentState()
{
  if (!mGotContentState) {
    mGotContentState = PR_TRUE;
    mContentState = 0;
    if (mPresContext) {
      mPresContext->EventStateManager()->GetContentState(mContent,
                                                         mContentState);
    } else {
      mContentState = mContent->IntrinsicState();
    }
  }
  return mContentState;
}

PRBool
RuleProcessorData::IsLink()
{
  if (!mGotLinkInfo) {
    mGotLinkInfo = PR_TRUE;
    mLinkState = eLinkState_Unknown;
    mIsLink = PR_FALSE;
    
    
    
    nsILinkHandler* linkHandler =
      mPresContext ? mPresContext->GetLinkHandler() : nsnull;
    if (mIsHTMLContent && mHasAttributes) {
      
      if (nsStyleUtil::IsHTMLLink(mContent, linkHandler, &mLinkState)) {
        mIsLink = PR_TRUE;
      }
    }

    
    
    
    if(!mIsLink &&
       mHasAttributes && 
       !(mIsHTMLContent || mContent->IsXUL()) && 
       nsStyleUtil::IsLink(mContent, linkHandler, &mLinkState)) {
      mIsLink = PR_TRUE;
    }

    if (mLinkState == eLinkState_Visited && !gSupportVisitedPseudo) {
      mLinkState = eLinkState_Unvisited;
    }
  }
  return mIsLink;
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

  PRUint32 childCount;
  nsIContent * const * curChildPtr = parent->GetChildArray(&childCount);

#ifdef DEBUG
  nsMutationGuard debugMutationGuard;
#endif  
  
  PRInt32 increment;
  nsIContent * const * stopPtr;
  if (aIsFromEnd) {
    stopPtr = curChildPtr - 1;
    curChildPtr = stopPtr + childCount;
    increment = -1;
  } else {
    increment = 1;
    stopPtr = curChildPtr + childCount;
  }

  for ( ; ; curChildPtr += increment) {
    if (curChildPtr == stopPtr) {
      
      result = 0; 
      break;
    }
    nsIContent* child = *curChildPtr;
    if (child == mContent)
      break;
    if (child->IsNodeOfType(nsINode::eELEMENT) &&
        (!aIsOfType ||
         (child->Tag() == mContentTag &&
          child->GetNameSpaceID() == mNameSpaceID))) {
      if (aCheckEdgeOnly) {
        
        
        result = -1;
        break;
      }
      ++result;
    }
  }

#ifdef DEBUG
  NS_ASSERTION(!debugMutationGuard.Mutated(0), "Unexpected mutations happened");
#endif  

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
                               const nsString& aValue, PRBool isHTML)
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
  const nsStringComparator& comparator =
      (aAttrSelector->mCaseSensitive || !isHTML)
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

static PRBool NS_FASTCALL
firstNodeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::firstNode,
                  "Unexpected atom");
  nsIContent *firstNode = nsnull;
  nsIContent *parent = data.mParentContent;
  if (parent) {
    if (setNodeFlags)
      parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

    PRInt32 index = -1;
    do {
      firstNode = parent->GetChildAt(++index);
      
    } while (firstNode &&
             !IsSignificantChild(firstNode, PR_TRUE, PR_FALSE));
  }
  return (data.mContent == firstNode);
}

static PRBool NS_FASTCALL
lastNodeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::lastNode,
                  "Unexpected atom");
  nsIContent *lastNode = nsnull;
  nsIContent *parent = data.mParentContent;
  if (parent) {
    if (setNodeFlags)
      parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

    PRUint32 index = parent->GetChildCount();
    do {
      lastNode = parent->GetChildAt(--index);
      
    } while (lastNode &&
             !IsSignificantChild(lastNode, PR_TRUE, PR_FALSE));
  }
  return (data.mContent == lastNode);
}

static inline PRBool
edgeChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 PRBool checkFirst, PRBool checkLast)
{
  nsIContent *parent = data.mParentContent;
  if (!parent) {
    return PR_FALSE;
  }

  if (setNodeFlags)
    parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

  return (!checkFirst ||
          data.GetNthIndex(PR_FALSE, PR_FALSE, PR_TRUE) == 1) &&
         (!checkLast ||
          data.GetNthIndex(PR_FALSE, PR_TRUE, PR_TRUE) == 1);
}

static PRBool NS_FASTCALL
firstChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::firstChild,
                  "Unexpected atom");
  return edgeChildMatches(data, setNodeFlags, PR_TRUE, PR_FALSE);
}

static PRBool NS_FASTCALL
lastChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::lastChild,
                  "Unexpected atom");
  return edgeChildMatches(data, setNodeFlags, PR_FALSE, PR_TRUE);
}

static PRBool NS_FASTCALL
onlyChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::onlyChild,
                  "Unexpected atom");
  return edgeChildMatches(data, setNodeFlags, PR_TRUE, PR_TRUE);
}

static inline PRBool
nthChildGenericMatches(RuleProcessorData& data, PRBool setNodeFlags,
                       nsPseudoClassList* pseudoClass,
                       PRBool isOfType, PRBool isFromEnd)
{
  nsIContent *parent = data.mParentContent;
  if (!parent) {
    return PR_FALSE;
  }

  if (setNodeFlags) {
    if (isFromEnd)
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
    else
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR_NOAPPEND);
  }

  const PRInt32 index = data.GetNthIndex(isOfType, isFromEnd, PR_FALSE);
  if (index <= 0) {
    
    return PR_FALSE;
  }

  const PRInt32 a = pseudoClass->u.mNumbers[0];
  const PRInt32 b = pseudoClass->u.mNumbers[1];
  
  
  if (a == 0) {
    return b == index;
  }

  
  
  
  const PRInt32 n = (index - b) / a;
  return n >= 0 && (a * n == index - b);
}

static PRBool NS_FASTCALL
nthChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::nthChild,
                  "Unexpected atom");
  return nthChildGenericMatches(data, setNodeFlags, pseudoClass,
                                PR_FALSE, PR_FALSE);
}

static PRBool NS_FASTCALL
nthLastChildMatches(RuleProcessorData& data, PRBool setNodeFlags,
                    nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::nthLastChild,
                  "Unexpected atom");
  return nthChildGenericMatches(data, setNodeFlags, pseudoClass,
                                PR_FALSE, PR_TRUE);
}

static PRBool NS_FASTCALL
nthOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::nthOfType,
                  "Unexpected atom");
  return nthChildGenericMatches(data, setNodeFlags, pseudoClass,
                                PR_TRUE, PR_FALSE);
}

static PRBool NS_FASTCALL
nthLastOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                     nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::nthLastOfType,
                  "Unexpected atom");
  return nthChildGenericMatches(data, setNodeFlags, pseudoClass,
                                PR_TRUE, PR_TRUE);
}

static inline PRBool
edgeOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  PRBool checkFirst, PRBool checkLast)
{
  nsIContent *parent = data.mParentContent;
  if (!parent) {
    return PR_FALSE;
  }

  if (setNodeFlags) {
    if (checkLast)
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
    else
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR_NOAPPEND);
  }

  return (!checkFirst ||
          data.GetNthIndex(PR_TRUE, PR_FALSE, PR_TRUE) == 1) &&
         (!checkLast ||
          data.GetNthIndex(PR_TRUE, PR_TRUE, PR_TRUE) == 1);
}

static PRBool NS_FASTCALL
firstOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                   nsPseudoClassList* pseudoClass)
{ 
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::firstOfType,
                  "Unexpected atom");
  return edgeOfTypeMatches(data, setNodeFlags, PR_TRUE, PR_FALSE);
}

static PRBool NS_FASTCALL
lastOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::lastOfType,
                  "Unexpected atom");
  return edgeOfTypeMatches(data, setNodeFlags, PR_FALSE, PR_TRUE);
}

static PRBool NS_FASTCALL
onlyOfTypeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::onlyOfType,
                  "Unexpected atom");
  return edgeOfTypeMatches(data, setNodeFlags, PR_TRUE, PR_TRUE);
}

static inline PRBool
checkGenericEmptyMatches(RuleProcessorData& data, PRBool setNodeFlags,
                         PRBool isWhitespaceSignificant)
{
  nsIContent *child = nsnull;
  nsIContent *element = data.mContent;
  PRInt32 index = -1;

  if (setNodeFlags)
    element->SetFlags(NODE_HAS_EMPTY_SELECTOR);

  do {
    child = element->GetChildAt(++index);
    
    
  } while (child && !IsSignificantChild(child, PR_TRUE, isWhitespaceSignificant));
  return (child == nsnull);
}

static PRBool NS_FASTCALL
emptyMatches(RuleProcessorData& data, PRBool setNodeFlags,
             nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::empty,
                  "Unexpected atom");
  return checkGenericEmptyMatches(data, setNodeFlags, PR_TRUE);
}

static PRBool NS_FASTCALL
mozOnlyWhitespaceMatches(RuleProcessorData& data, PRBool setNodeFlags,
                         nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozOnlyWhitespace,
                  "Unexpected atom");
  return checkGenericEmptyMatches(data, setNodeFlags, PR_FALSE);
}

static PRBool NS_FASTCALL
mozEmptyExceptChildrenWithLocalnameMatches(RuleProcessorData& data,
                                           PRBool setNodeFlags,
                                           nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom ==
                    nsCSSPseudoClasses::mozEmptyExceptChildrenWithLocalname,
                  "Unexpected atom");
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
  return (child == nsnull);
}

static PRBool NS_FASTCALL
mozSystemMetricMatches(RuleProcessorData& data, PRBool setNodeFlags,
                       nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozSystemMetric,
                  "Unexpected atom");
  NS_ASSERTION(pseudoClass->u.mString, "Must have string!");
  nsCOMPtr<nsIAtom> metric = do_GetAtom(pseudoClass->u.mString);
  return nsCSSRuleProcessor::HasSystemMetric(metric);
}

static PRBool NS_FASTCALL
mozHasHandlerRefMatches(RuleProcessorData& data, PRBool setNodeFlags,
                        nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozHasHandlerRef,
                  "Unexpected atom");
  nsIContent *child = nsnull;
  nsIContent *element = data.mContent;
  PRInt32 index = -1;

  do {
    child = element->GetChildAt(++index);
    if (child && child->IsHTML() &&
        child->Tag() == nsGkAtoms::param &&
        child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                           NS_LITERAL_STRING("pluginurl"), eIgnoreCase)) {
      return PR_TRUE;
    }
  } while (child);
  return PR_FALSE;
}

static PRBool NS_FASTCALL
rootMatches(RuleProcessorData& data, PRBool setNodeFlags,
            nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::root,
                  "Unexpected atom");
  return (data.mParentContent == nsnull &&
          data.mContent == data.mContent->GetOwnerDoc()->GetRootContent());
}

static PRBool NS_FASTCALL
mozBoundElementMatches(RuleProcessorData& data, PRBool setNodeFlags,
                       nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozBoundElement,
                  "Unexpected atom");
  
  
  
  return (data.mScopedRoot == data.mContent);
}

static PRBool NS_FASTCALL
langMatches(RuleProcessorData& data, PRBool setNodeFlags,
            nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::lang,
                  "Unexpected atom");
  NS_ASSERTION(nsnull != pseudoClass->u.mString, "null lang parameter");
  if (!pseudoClass->u.mString || !*pseudoClass->u.mString) {
    return PR_FALSE;
  }

  
  
  
  
  const nsString* lang = data.GetLang();
  if (lang && !lang->IsEmpty()) { 
    return
      nsStyleUtil::DashMatchCompare(*lang,
                                    nsDependentString(pseudoClass->u.mString), 
                                    nsCaseInsensitiveStringComparator());
  }

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
        return PR_TRUE;
      }
      begin = end + 1;
    }
  }

  return PR_FALSE;
}

static PRBool NS_FASTCALL
mozAnyLinkMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozAnyLink,
                  "Unexpected atom");
  return data.IsLink();
}

static PRBool NS_FASTCALL
linkMatches(RuleProcessorData& data, PRBool setNodeFlags,
            nsPseudoClassList* pseudoClass)
{
  NS_NOTREACHED("Shouldn't be called");
  return PR_FALSE;
}

static PRBool NS_FASTCALL
visitedMatches(RuleProcessorData& data, PRBool setNodeFlags,
               nsPseudoClassList* pseudoClass)
{
  NS_NOTREACHED("Shouldn't be called");
  return PR_FALSE;
}

static PRBool NS_FASTCALL
mozIsHTMLMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozIsHTML,
                  "Unexpected atom");
  return data.mIsHTML;
}

static PRBool NS_FASTCALL
mozLocaleDirMatches(RuleProcessorData& data, PRBool setNodeFlags,
                    nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozLocaleDir,
                  "Unexpected atom");
  nsIDocument* doc = data.mContent->GetDocument();

  if (!doc) {
    return PR_FALSE;
  }

  PRBool docIsRTL = doc && doc->IsDocumentRightToLeft();

  nsDependentString dirString(pseudoClass->u.mString);
  NS_ASSERTION(dirString.EqualsLiteral("ltr") || dirString.EqualsLiteral("rtl"),
               "invalid value for -moz-locale-dir");

  return dirString.EqualsLiteral("rtl") == docIsRTL;
}

static PRBool NS_FASTCALL
mozLWThemeMatches(RuleProcessorData& data, PRBool setNodeFlags,
                  nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom == nsCSSPseudoClasses::mozLWTheme,
                  "Unexpected atom");
  nsIDocument* doc = data.mContent->GetOwnerDoc();
  return doc && doc->GetDocumentLWTheme() > nsIDocument::Doc_Theme_None;
}

static PRBool NS_FASTCALL
mozLWThemeBrightTextMatches(RuleProcessorData& data, PRBool setNodeFlags,
                            nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom ==
                    nsCSSPseudoClasses::mozLWThemeBrightText,
                  "Unexpected atom");
  nsIDocument* doc = data.mContent->GetOwnerDoc();
  return doc && doc->GetDocumentLWTheme() == nsIDocument::Doc_Theme_Bright;
}

static PRBool NS_FASTCALL
mozLWThemeDarkTextMatches(RuleProcessorData& data, PRBool setNodeFlags,
                          nsPseudoClassList* pseudoClass)
{
  NS_PRECONDITION(pseudoClass->mAtom ==
                    nsCSSPseudoClasses::mozLWThemeDarkText,
                  "Unexpected atom");
  nsIDocument* doc = data.mContent->GetOwnerDoc();
  return doc && doc->GetDocumentLWTheme() == nsIDocument::Doc_Theme_Dark;
}

static PRBool NS_FASTCALL
notPseudoMatches(RuleProcessorData& data, PRBool setNodeFlags,
                 nsPseudoClassList* pseudoClass)
{
  NS_NOTREACHED("Why did this get called?");
  return PR_FALSE;
}

typedef PRBool
  (NS_FASTCALL * PseudoClassMatcher)(RuleProcessorData&, PRBool setNodeFlags,
                                     nsPseudoClassList* pseudoClass);




struct PseudoClassInfo {
  PseudoClassMatcher mFunc;
  PRInt32 mBit;
};

static const PseudoClassInfo sPseudoClassInfo[] = {
#define CSS_PSEUDO_CLASS(_name, _value)         \
  { &_name##Matches, 0 },
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _bit) \
  { nsnull, _bit },
#include "nsCSSPseudoClassList.h"
#undef CSS_STATE_PSEUDO_CLASS
#undef CSS_PSEUDO_CLASS
  
  
  { nsnull, 0 },
  { nsnull, 0 }
};
PR_STATIC_ASSERT(NS_ARRAY_LENGTH(sPseudoClassInfo) >
                   nsCSSPseudoClasses::ePseudoClass_NotPseudoClass);















static PRBool SelectorMatches(RuleProcessorData &data,
                              nsCSSSelector* aSelector,
                              PRInt32 aStateMask, 
                              PRBool aForStyling,
                              PRBool* const aDependence = nsnull) 

{
  NS_PRECONDITION(!aSelector->IsPseudoElement(),
                  "Pseudo-element snuck into SelectorMatches?");
  
  
  if ((kNameSpaceID_Unknown != aSelector->mNameSpace &&
       data.mNameSpaceID != aSelector->mNameSpace))
    return PR_FALSE;

  if (aSelector->mLowercaseTag && 
      (data.mIsHTML ? aSelector->mLowercaseTag : aSelector->mCasedTag) !=
        data.mContentTag) {
    return PR_FALSE;
  }

  nsAtomList* IDList = aSelector->mIDList;
  if (IDList) {
    if (data.mContentID) {
      
      const PRBool isCaseSensitive =
        data.mCompatMode != eCompatibility_NavQuirks;

      if (isCaseSensitive) {
        do {
          if (IDList->mAtom != data.mContentID) {
            return PR_FALSE;
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
            return PR_FALSE;
          }
          IDList = IDList->mNext;
        } while (IDList);
      }
    } else {
      
      return PR_FALSE;
    }
  }

  nsAtomList* classList = aSelector->mClassList;
  if (classList) {
    
    const nsAttrValue *elementClasses = data.mClasses;
    if (!elementClasses) {
      
      return PR_FALSE;
    }

    
    const PRBool isCaseSensitive =
      data.mCompatMode != eCompatibility_NavQuirks;

    while (classList) {
      if (!elementClasses->Contains(classList->mAtom,
                                    isCaseSensitive ?
                                      eCaseMatters : eIgnoreCase)) {
        return PR_FALSE;
      }
      classList = classList->mNext;
    }
  }

  const PRBool isNegated = (aDependence != nsnull);
  
  
  
  
  
  NS_ASSERTION(aStateMask == 0 || !aForStyling,
               "aForStyling must be false if we're just testing for "
               "state-dependence");
  const PRBool setNodeFlags = aForStyling;

  
  for (nsPseudoClassList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    
    
    if (pseudoClass->mAtom == nsCSSPseudoClasses::link ||
        pseudoClass->mAtom == nsCSSPseudoClasses::visited) {
      if (!data.IsLink()) {
        return PR_FALSE;
      }

      if (aStateMask & NS_EVENT_STATE_VISITED) {
        if (aDependence)
          *aDependence = PR_TRUE;
      } else if ((eLinkState_Visited == data.LinkState()) ==
                 (nsCSSPseudoClasses::link == pseudoClass->mAtom)) {
        
        return PR_FALSE;
      }
      continue;
    }
    const PseudoClassInfo& info = sPseudoClassInfo[pseudoClass->mType];
    if (info.mFunc) {
      if (!(*info.mFunc)(data, setNodeFlags, pseudoClass)) {
        return PR_FALSE;
      }
    } else {
      PRInt32 stateToCheck = info.mBit;
      NS_ABORT_IF_FALSE(stateToCheck != 0, "How did that happen?");
      if ((stateToCheck & (NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE)) &&
          data.mCompatMode == eCompatibility_NavQuirks &&
          
          !aSelector->HasTagSelector() && !aSelector->mIDList && 
          !aSelector->mAttrList &&
          
          
          
          
          !isNegated &&
          
          data.mIsHTMLContent && !data.IsLink() &&
          !IsQuirkEventSensitive(data.mContentTag)) {
        
        
        return PR_FALSE;
      } else {
        if (aStateMask & stateToCheck) {
          if (aDependence)
            *aDependence = PR_TRUE;
        } else {
          if (!(data.ContentState() & stateToCheck)) {
            return PR_FALSE;
          }
        }
      }
    }
  }

  PRBool result = PR_TRUE;
  if (aSelector->mAttrList) {
    
    if (!data.mHasAttributes) {
      
      return PR_FALSE;
    } else {
      result = PR_TRUE;
      nsAttrSelector* attr = aSelector->mAttrList;
      nsIAtom* matchAttribute;

      do {
        matchAttribute = data.mIsHTML ? attr->mLowercaseAttr : attr->mCasedAttr;
        if (attr->mNameSpace == kNameSpaceID_Unknown) {
          
          
          
          
          
          
          
          PRUint32 attrCount = data.mContent->GetAttrCount();
          result = PR_FALSE;
          for (PRUint32 i = 0; i < attrCount; ++i) {
            const nsAttrName* attrName =
              data.mContent->GetAttrNameAt(i);
            NS_ASSERTION(attrName, "GetAttrCount lied or GetAttrNameAt failed");
            if (attrName->LocalName() != matchAttribute) {
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
              result = AttrMatchesValue(attr, value, data.mIsHTML);
            }

            
            
            
            
            
            if (result) {
              break;
            }
          }
        }
        else if (attr->mFunction == NS_ATTR_FUNC_EQUALS) {
          result =
            data.mContent->
              AttrValueIs(attr->mNameSpace, matchAttribute, attr->mValue,
                          (!data.mIsHTML || attr->mCaseSensitive) ? eCaseMatters
                                                                  : eIgnoreCase);
        }
        else if (!data.mContent->HasAttr(attr->mNameSpace, matchAttribute)) {
          result = PR_FALSE;
        }
        else if (attr->mFunction != NS_ATTR_FUNC_SET) {
          nsAutoString value;
#ifdef DEBUG
          PRBool hasAttr =
#endif
              data.mContent->GetAttr(attr->mNameSpace, matchAttribute, value);
          NS_ASSERTION(hasAttr, "HasAttr lied");
          result = AttrMatchesValue(attr, value, data.mIsHTML);
        }
        
        attr = attr->mNext;
      } while (attr && result);
    }
  }

  
  if (!isNegated) {
    for (nsCSSSelector *negation = aSelector->mNegations;
         result && negation; negation = negation->mNegations) {
      PRBool dependence = PR_FALSE;
      result = !SelectorMatches(data, negation, aStateMask,
                                aForStyling, &dependence);
      
      
      
      
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
        nsIContent* parent = prevdata->mParentContent;
        if (parent) {
          if (aForStyling)
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
        nsIContent *content = prevdata->mParentContent;
        
        
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
    if (SelectorMatches(*data, selector, 0, aForStyling)) {
      
      
      
      
      
      if (NS_IS_GREEDY_OPERATOR(selector->mOperator) &&
          selector->mNext &&
          selector->mNext->mOperator != selector->mOperator &&
          !(selector->mOperator == '~' &&
            (selector->mNext->mOperator == PRUnichar(0) ||
             selector->mNext->mOperator == PRUnichar('>')))) {

        
        

        
        
        
        
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
  RuleProcessorData* data = (RuleProcessorData*)aData;

  if (SelectorMatches(*data, aSelector, 0, PR_TRUE)) {
    nsCSSSelector *next = aSelector->mNext;
    if (!next || SelectorMatchesTree(*data, next, PR_TRUE)) {
      
      
      
#ifdef DEBUG
      nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
      NS_ASSERTION(static_cast<nsIStyleRule*>(aRule) == iRule.get(),
                   "Please fix QI so this performance optimization is valid");
#endif
      aRule->RuleMatched();
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

NS_IMETHODIMP
nsCSSRuleProcessor::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    RuleHash* ruleHash = cascade->mPseudoElementRuleHashes[aData->mPseudoType];
    if (ruleHash) {
      ruleHash->EnumerateAllRules(aData->mNameSpaceID,
                                  aData->mContentTag,
                                  aData->mContentID,
                                  aData->mClasses,
                                  ContentEnumFunc,
                                  aData);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSRuleProcessor::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade && cascade->mAnonBoxRules.entryCount) {
    RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>
      (PL_DHashTableOperate(&cascade->mAnonBoxRules, aData->mPseudoTag,
                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      for (RuleValue *value = entry->mRules; value; value = value->mNext) {
        
        
        
#ifdef DEBUG
        nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(value->mRule);
        NS_ASSERTION(static_cast<nsIStyleRule*>(value->mRule) == iRule.get(),
                     "Please fix QI so this performance optimization is valid");
#endif
        value->mRule->RuleMatched();
        aData->mRuleWalker->Forward(static_cast<nsIStyleRule*>(value->mRule));
      }
    }
  }
  return NS_OK;
}

#ifdef MOZ_XUL
NS_IMETHODIMP
nsCSSRuleProcessor::RulesMatching(XULTreeRuleProcessorData* aData)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade && cascade->mXULTreeRules.entryCount) {
    RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>
      (PL_DHashTableOperate(&cascade->mXULTreeRules, aData->mPseudoTag,
                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      for (RuleValue *value = entry->mRules; value; value = value->mNext) {
        PRBool matches = PR_TRUE;
        aData->mComparator->PseudoMatches(aData->mPseudoTag, value->mSelector,
                                          &matches);
        if (matches) {
          ContentEnumFunc(value->mRule, value->mSelector->mNext,
                          static_cast<RuleProcessorData*>(aData));
        }
      }
    }
  }
  return NS_OK;
}
#endif

inline PRBool
IsSiblingOperator(PRUnichar oper)
{
  return oper == PRUnichar('+') || oper == PRUnichar('~');
}

nsReStyleHint
nsCSSRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  
  
  nsReStyleHint hint = nsReStyleHint(0);
  if (cascade) {
    nsCSSSelector **iter = cascade->mStateSelectors.Elements(),
                  **end = iter + cascade->mStateSelectors.Length();
    for(; iter != end; ++iter) {
      nsCSSSelector* selector = *iter;

      nsReStyleHint possibleChange = IsSiblingOperator(selector->mOperator) ?
        eReStyle_LaterSiblings : eReStyle_Self;

      
      
      
      if ((possibleChange & ~hint) &&
          SelectorMatches(*aData, selector, aData->mStateMask, PR_FALSE) &&
          SelectorMatchesTree(*aData, selector->mNext, PR_FALSE)) {
        hint = nsReStyleHint(hint | possibleChange);
      }
    }
  }
  return hint;
}

struct AttributeEnumData {
  AttributeEnumData(AttributeRuleProcessorData *aData)
    : data(aData), change(nsReStyleHint(0)) {}

  AttributeRuleProcessorData *data;
  nsReStyleHint change;
};


static void
AttributeEnumFunc(nsCSSSelector* aSelector, AttributeEnumData* aData)
{
  AttributeRuleProcessorData *data = aData->data;

  nsReStyleHint possibleChange = IsSiblingOperator(aSelector->mOperator) ?
    eReStyle_LaterSiblings : eReStyle_Self;

  
  
  
  if ((possibleChange & ~(aData->change)) &&
      SelectorMatches(*data, aSelector, 0, PR_FALSE) &&
      SelectorMatchesTree(*data, aSelector->mNext, PR_FALSE)) {
    aData->change = nsReStyleHint(aData->change | possibleChange);
  }
}

nsReStyleHint
nsCSSRuleProcessor::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  NS_PRECONDITION(aData->mContent->IsNodeOfType(nsINode::eELEMENT),
                  "content must be element");

  
  

  AttributeEnumData data(aData);

  
  
  if (aData->mAttrHasChanged) {
    
    
    
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
    
    

    
    if ((aData->mAttribute == nsGkAtoms::localedir ||
         aData->mAttribute == nsGkAtoms::lwtheme ||
         aData->mAttribute == nsGkAtoms::lwthemetextcolor) &&
        aData->mNameSpaceID == kNameSpaceID_XUL &&
        aData->mContent == aData->mContent->GetOwnerDoc()->GetRootContent())
      {
        data.change = nsReStyleHint(data.change | eReStyle_Self);
      }
  }

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  if (cascade) {
    if (aData->mAttribute == aData->mContent->GetIDAttributeName()) {
      nsCSSSelector **iter = cascade->mIDSelectors.Elements(),
                    **end = iter + cascade->mIDSelectors.Length();
      for(; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
    }
    
    if (aData->mAttribute == aData->mContent->GetClassAttributeName()) {
      nsCSSSelector **iter = cascade->mClassSelectors.Elements(),
                    **end = iter + cascade->mClassSelectors.Length();
      for(; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
    }

    AttributeSelectorEntry *entry = static_cast<AttributeSelectorEntry*>
                                               (PL_DHashTableOperate(&cascade->mAttributeSelectors, aData->mAttribute,
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      nsCSSSelector **iter = entry->mSelectors->Elements(),
                    **end = iter + entry->mSelectors->Length();
      for(; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
    }
  }

  return data.change;
}

NS_IMETHODIMP
nsCSSRuleProcessor::MediumFeaturesChanged(nsPresContext* aPresContext,
                                          PRBool* aRulesChanged)
{
  RuleCascadeData *old = mRuleCascades;
  
  
  
  
  
  
  if (old) {
    RefreshRuleCascade(aPresContext);
  }
  *aRulesChanged = (old != mRuleCascades);
  return NS_OK;
}



PRBool
nsCSSRuleProcessor::AppendFontFaceRules(
                              nsPresContext *aPresContext,
                              nsTArray<nsFontFaceRuleContainer>& aArray)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    if (!aArray.AppendElements(cascade->mFontFaceRules))
      return PR_FALSE;
  }
  
  return PR_TRUE;
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
    
    
    if (pseudoClass->mType >= nsCSSPseudoClasses::ePseudoClass_Count) {
      continue;
    }
    
    
    if (sPseudoClassInfo[pseudoClass->mType].mBit ||
        pseudoClass->mAtom == nsCSSPseudoClasses::link ||
        pseudoClass->mAtom == nsCSSPseudoClasses::visited) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static PRBool
AddRule(RuleValue* aRuleInfo, RuleCascadeData* aCascade)
{
  RuleCascadeData * const cascade = aCascade;

  
  nsCSSPseudoElements::Type pseudoType = aRuleInfo->mSelector->PseudoType();
  if (NS_LIKELY(pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement)) {
    cascade->mRuleHash.PrependRule(aRuleInfo);
  } else if (pseudoType < nsCSSPseudoElements::ePseudo_PseudoElementCount) {
    RuleHash*& ruleHash = cascade->mPseudoElementRuleHashes[pseudoType];
    if (!ruleHash) {
      ruleHash = new RuleHash(cascade->mQuirksMode);
      if (!ruleHash) {
        
        return PR_FALSE;
      }
    }
    NS_ASSERTION(aRuleInfo->mSelector->mNext,
                 "Must have mNext; parser screwed up");
    NS_ASSERTION(aRuleInfo->mSelector->mNext->mOperator == '>',
                 "Unexpected mNext combinator");
    aRuleInfo->mSelector = aRuleInfo->mSelector->mNext;
    ruleHash->PrependRule(aRuleInfo);
  } else if (pseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
    NS_ASSERTION(!aRuleInfo->mSelector->mCasedTag &&
                 !aRuleInfo->mSelector->mIDList &&
                 !aRuleInfo->mSelector->mClassList &&
                 !aRuleInfo->mSelector->mPseudoClassList &&
                 !aRuleInfo->mSelector->mAttrList &&
                 !aRuleInfo->mSelector->mNegations &&
                 !aRuleInfo->mSelector->mNext &&
                 aRuleInfo->mSelector->mNameSpace == kNameSpaceID_Unknown,
                 "Parser messed up with anon box selector");

    
    
    DoPrependRuleToTagTable(&cascade->mAnonBoxRules,
                            aRuleInfo->mSelector->mLowercaseTag,
                            aRuleInfo, 0);
  } else {
#ifdef MOZ_XUL
    NS_ASSERTION(pseudoType == nsCSSPseudoElements::ePseudo_XULTree,
                 "Unexpected pseudo type");
    
    
    DoPrependRuleToTagTable(&cascade->mXULTreeRules,
                            aRuleInfo->mSelector->mLowercaseTag,
                            aRuleInfo, 0);
#else
    NS_NOTREACHED("Unexpected pseudo type");
#endif
  }

  nsTArray<nsCSSSelector*>* stateArray = &cascade->mStateSelectors;
  nsTArray<nsCSSSelector*>* classArray = &cascade->mClassSelectors;
  nsTArray<nsCSSSelector*>* idArray = &cascade->mIDSelectors;
  
  for (nsCSSSelector* selector = aRuleInfo->mSelector;
           selector; selector = selector->mNext) {
    if (selector->IsPseudoElement()) {
      NS_ASSERTION(!selector->mNegations, "Shouldn't have negations");
      
      
      
      continue;
    }
    
    
    
    
    
    
    
    
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
        nsTArray<nsCSSSelector*> *array =
          cascade->AttributeListFor(attr->mCasedAttr);
        if (!array)
          return PR_FALSE;
        array->AppendElement(selector);
        if (attr->mLowercaseAttr != attr->mCasedAttr) {
          nsTArray<nsCSSSelector*> *array =
            cascade->AttributeListFor(attr->mLowercaseAttr);
          if (!array)
            return PR_FALSE;
          array->AppendElement(selector);
        }          
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

static PLDHashNumber
HashIntKey(PLDHashTable *table, const void *key)
{
  return PLDHashNumber(NS_PTR_TO_INT32(key));
}

static PRBool
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
                  nsTArray<nsFontFaceRuleContainer>& aFontFaceRules,
                  nsMediaQueryResultCacheKey& aKey,
                  PLArenaPool& aArena,
                  PRUint8 aSheetType)
    : mPresContext(aPresContext),
      mFontFaceRules(aFontFaceRules),
      mCacheKey(aKey),
      mArena(aArena),
      mSheetType(aSheetType)
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
  nsTArray<nsFontFaceRuleContainer>& mFontFaceRules;
  nsMediaQueryResultCacheKey& mCacheKey;
  PLArenaPool& mArena;
  
  
  PLDHashTable mRulesByWeight; 
  PRUint8 mSheetType;
};









static PRBool
CascadeRuleEnumFunc(nsICSSRule* aRule, void* aData)
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
      if (!groupRule->EnumerateRulesForwards(CascadeRuleEnumFunc, aData))
        return PR_FALSE;
  }
  else if (nsICSSRule::FONT_FACE_RULE == type) {
    nsCSSFontFaceRule *fontFaceRule = static_cast<nsCSSFontFaceRule*>(aRule);
    nsFontFaceRuleContainer *ptr = data->mFontFaceRules.AppendElement();
    if (!ptr)
      return PR_FALSE;
    ptr->mRule = fontFaceRule;
    ptr->mSheetType = data->mSheetType;
  }

  return PR_TRUE;
}

 PRBool
nsCSSRuleProcessor::CascadeSheetEnumFunc(nsICSSStyleSheet* aSheet, void* aData)
{
  nsCSSStyleSheet*  sheet = static_cast<nsCSSStyleSheet*>(aSheet);
  CascadeEnumData* data = static_cast<CascadeEnumData*>(aData);
  PRBool bSheetApplicable = PR_TRUE;
  sheet->GetApplicable(bSheetApplicable);

  if (bSheetApplicable &&
      sheet->UseForPresentation(data->mPresContext, data->mCacheKey) &&
      sheet->mInner) {
    nsCSSStyleSheet* child = sheet->mInner->mFirstChild;
    while (child) {
      CascadeSheetEnumFunc(child, data);
      child = child->mNext;
    }

    if (!sheet->mInner->mOrderedRules.EnumerateForwards(CascadeRuleEnumFunc,
                                                        data))
      return PR_FALSE;
  }
  return PR_TRUE;
}

static int CompareWeightData(const void* aArg1, const void* aArg2,
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


static PLDHashOperator
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
      CascadeEnumData data(aPresContext, newCascade->mFontFaceRules,
                           newCascade->mCacheKey,
                           newCascade->mRuleHash.Arena(),
                           mSheetType);
      if (!data.mRulesByWeight.ops)
        return; 
      if (!mSheets.EnumerateForwards(CascadeSheetEnumFunc, &data))
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
    NS_ASSERTION(!sel->IsPseudoElement(), "Shouldn't have been called");
    if (SelectorMatches(aData, sel, 0, PR_FALSE)) {
      nsCSSSelector* next = sel->mNext;
      if (!next || SelectorMatchesTree(aData, next, PR_FALSE)) {
        return PR_TRUE;
      }
    }

    aSelectorList = aSelectorList->mNext;
  }

  return PR_FALSE;
}
