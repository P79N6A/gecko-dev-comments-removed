










#define PL_ARENA_CONST_ALIGN_MASK 7



#define NS_CASCADEENUMDATA_ARENA_BLOCK_SIZE (4096)
#include "plarena.h"

#include "nsCSSRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include <algorithm>
#include "nsIAtom.h"
#include "pldhash.h"
#include "nsICSSPseudoComparator.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/GroupRule.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsUnicharUtils.h"
#include "nsError.h"
#include "nsRuleWalker.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSPseudoElements.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsStyleUtil.h"
#include "nsQuickSort.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsAttrName.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "nsIMediaList.h"
#include "nsCSSRules.h"
#include "nsStyleSet.h"
#include "mozilla/dom/Element.h"
#include "nsNthIndexCache.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/Likely.h"

using namespace mozilla;
using namespace mozilla::dom;

#define VISITED_PSEUDO_PREF "layout.css.visited_links_enabled"

static bool gSupportVisitedPseudo = true;

static nsTArray< nsCOMPtr<nsIAtom> >* sSystemMetrics = 0;

#ifdef XP_WIN
uint8_t nsCSSRuleProcessor::sWinThemeId = LookAndFeel::eWindowsTheme_Generic;
#endif





struct RuleSelectorPair {
  RuleSelectorPair(css::StyleRule* aRule, nsCSSSelector* aSelector)
    : mRule(aRule), mSelector(aSelector) {}
  
  

  css::StyleRule*   mRule;
  nsCSSSelector*    mSelector; 
};

#define NS_IS_ANCESTOR_OPERATOR(ch) \
  ((ch) == char16_t(' ') || (ch) == char16_t('>'))






struct RuleValue : RuleSelectorPair {
  enum {
    eMaxAncestorHashes = 4
  };

  RuleValue(const RuleSelectorPair& aRuleSelectorPair, int32_t aIndex,
            bool aQuirksMode) :
    RuleSelectorPair(aRuleSelectorPair),
    mIndex(aIndex)
  {
    CollectAncestorHashes(aQuirksMode);
  }

  int32_t mIndex; 
  uint32_t mAncestorSelectorHashes[eMaxAncestorHashes];

private:
  void CollectAncestorHashes(bool aQuirksMode) {
    
    
    
    
    size_t hashIndex = 0;
    for (nsCSSSelector* sel = mSelector->mNext; sel; sel = sel->mNext) {
      if (!NS_IS_ANCESTOR_OPERATOR(sel->mOperator)) {
        
        
        
        
        continue;
      }

      
      
      
      
      if (!aQuirksMode) {
        nsAtomList* ids = sel->mIDList;
        while (ids) {
          mAncestorSelectorHashes[hashIndex++] = ids->mAtom->hash();
          if (hashIndex == eMaxAncestorHashes) {
            return;
          }
          ids = ids->mNext;
        }

        nsAtomList* classes = sel->mClassList;
        while (classes) {
          mAncestorSelectorHashes[hashIndex++] = classes->mAtom->hash();
          if (hashIndex == eMaxAncestorHashes) {
            return;
          }
          classes = classes->mNext;
        }
      }

      
      
      
      if (sel->mLowercaseTag && sel->mCasedTag == sel->mLowercaseTag) {
        mAncestorSelectorHashes[hashIndex++] = sel->mLowercaseTag->hash();
        if (hashIndex == eMaxAncestorHashes) {
          return;
        }
      }
    }

    while (hashIndex != eMaxAncestorHashes) {
      mAncestorSelectorHashes[hashIndex++] = 0;
    }
  }
};






struct RuleHashTableEntry : public PLDHashEntryHdr {
  
  
  
  nsAutoTArray<RuleValue, 1> mRules;
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
  nsContentUtils::ASCIIToLower(str);
  return HashString(str);
}

typedef nsIAtom*
(* RuleHashGetKey) (PLDHashTable *table, const PLDHashEntryHdr *entry);

struct RuleHashTableOps {
  const PLDHashTableOps ops;
  
  
  
  RuleHashGetKey getKey;
};

inline const RuleHashTableOps*
ToLocalOps(const PLDHashTableOps *aOps)
{
  return (const RuleHashTableOps*)
           (((const char*) aOps) - offsetof(RuleHashTableOps, ops));
}

static bool
RuleHash_CIMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  
  nsIAtom *entry_atom = ToLocalOps(table->ops)->getKey(table, hdr);

  
  if (match_atom == entry_atom)
    return true;

  
  
  

  return
    nsContentUtils::EqualsIgnoreASCIICase(nsDependentAtomString(entry_atom),
                                          nsDependentAtomString(match_atom));
}

static bool
RuleHash_CSMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  
  nsIAtom *entry_atom = ToLocalOps(table->ops)->getKey(table, hdr);

  return match_atom == entry_atom;
}

static bool
RuleHash_InitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                   const void *key)
{
  RuleHashTableEntry* entry = static_cast<RuleHashTableEntry*>(hdr);
  new (entry) RuleHashTableEntry();
  return true;
}

static void
RuleHash_ClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTableEntry* entry = static_cast<RuleHashTableEntry*>(hdr);
  entry->~RuleHashTableEntry();
}

static void
RuleHash_MoveEntry(PLDHashTable *table, const PLDHashEntryHdr *from,
                   PLDHashEntryHdr *to)
{
  NS_PRECONDITION(from != to, "This is not going to work!");
  RuleHashTableEntry *oldEntry =
    const_cast<RuleHashTableEntry*>(
      static_cast<const RuleHashTableEntry*>(from));
  RuleHashTableEntry *newEntry = new (to) RuleHashTableEntry();
  newEntry->mRules.SwapElements(oldEntry->mRules);
  oldEntry->~RuleHashTableEntry();
}

static bool
RuleHash_TagTable_MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>
                                              (key));
  nsIAtom *entry_atom = static_cast<const RuleHashTagTableEntry*>(hdr)->mTag;

  return match_atom == entry_atom;
}

static bool
RuleHash_TagTable_InitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                            const void *key)
{
  RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>(hdr);
  new (entry) RuleHashTagTableEntry();
  entry->mTag = const_cast<nsIAtom*>(static_cast<const nsIAtom*>(key));
  return true;
}

static void
RuleHash_TagTable_ClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>(hdr);
  entry->~RuleHashTagTableEntry();
}

static void
RuleHash_TagTable_MoveEntry(PLDHashTable *table, const PLDHashEntryHdr *from,
                            PLDHashEntryHdr *to)
{
  NS_PRECONDITION(from != to, "This is not going to work!");
  RuleHashTagTableEntry *oldEntry =
    const_cast<RuleHashTagTableEntry*>(
      static_cast<const RuleHashTagTableEntry*>(from));
  RuleHashTagTableEntry *newEntry = new (to) RuleHashTagTableEntry();
  newEntry->mTag.swap(oldEntry->mTag);
  newEntry->mRules.SwapElements(oldEntry->mRules);
  oldEntry->~RuleHashTagTableEntry();
}

static nsIAtom*
RuleHash_ClassTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  nsCSSSelector* selector = entry->mRules[0].mSelector;
  if (selector->IsPseudoElement()) {
    selector = selector->mNext;
  }
  return selector->mClassList->mAtom;
}

static nsIAtom*
RuleHash_IdTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  nsCSSSelector* selector = entry->mRules[0].mSelector;
  if (selector->IsPseudoElement()) {
    selector = selector->mNext;
  }
  return selector->mIDList->mAtom;
}

static PLDHashNumber
RuleHash_NameSpaceTable_HashKey(PLDHashTable *table, const void *key)
{
  return NS_PTR_TO_INT32(key);
}

static bool
RuleHash_NameSpaceTable_MatchEntry(PLDHashTable *table,
                                   const PLDHashEntryHdr *hdr,
                                   const void *key)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);

  nsCSSSelector* selector = entry->mRules[0].mSelector;
  if (selector->IsPseudoElement()) {
    selector = selector->mNext;
  }
  return NS_PTR_TO_INT32(key) == selector->mNameSpace;
}

static const PLDHashTableOps RuleHash_TagTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_TagTable_MatchEntry,
  RuleHash_TagTable_MoveEntry,
  RuleHash_TagTable_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_TagTable_InitEntry
};


static const RuleHashTableOps RuleHash_ClassTable_CSOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  RuleHash_MoveEntry,
  RuleHash_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_InitEntry
  },
  RuleHash_ClassTable_GetKey
};


static const RuleHashTableOps RuleHash_ClassTable_CIOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  RuleHash_MoveEntry,
  RuleHash_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_InitEntry
  },
  RuleHash_ClassTable_GetKey
};


static const RuleHashTableOps RuleHash_IdTable_CSOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  RuleHash_MoveEntry,
  RuleHash_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_InitEntry
  },
  RuleHash_IdTable_GetKey
};


static const RuleHashTableOps RuleHash_IdTable_CIOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  RuleHash_MoveEntry,
  RuleHash_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_InitEntry
  },
  RuleHash_IdTable_GetKey
};

static const PLDHashTableOps RuleHash_NameSpaceTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_NameSpaceTable_HashKey,
  RuleHash_NameSpaceTable_MatchEntry,
  RuleHash_MoveEntry,
  RuleHash_ClearEntry,
  PL_DHashFinalizeStub,
  RuleHash_InitEntry
};

#undef RULE_HASH_STATS
#undef PRINT_UNIVERSAL_RULES

#ifdef RULE_HASH_STATS
#define RULE_HASH_STAT_INCREMENT(var_) PR_BEGIN_MACRO ++(var_); PR_END_MACRO
#else
#define RULE_HASH_STAT_INCREMENT(var_) PR_BEGIN_MACRO PR_END_MACRO
#endif

struct NodeMatchContext;

class RuleHash {
public:
  explicit RuleHash(bool aQuirksMode);
  ~RuleHash();
  void AppendRule(const RuleSelectorPair &aRuleInfo);
  void EnumerateAllRules(Element* aElement, ElementDependentRuleProcessorData* aData,
                         NodeMatchContext& aNodeMatchContext);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

protected:
  typedef nsTArray<RuleValue> RuleValueList;
  void AppendRuleToTable(PLDHashTable* aTable, const void* aKey,
                         const RuleSelectorPair& aRuleInfo);
  void AppendUniversalRule(const RuleSelectorPair& aRuleInfo);

  int32_t     mRuleCount;
  
  
  PLDHashTable mIdTable;
  PLDHashTable mClassTable;
  PLDHashTable mTagTable;
  PLDHashTable mNameSpaceTable;
  RuleValueList mUniversalRules;

  struct EnumData {
    const RuleValue* mCurValue;
    const RuleValue* mEnd;
  };
  EnumData* mEnumList;
  int32_t   mEnumListSize;

  bool mQuirksMode;

  inline EnumData ToEnumData(const RuleValueList& arr) {
    EnumData data = { arr.Elements(), arr.Elements() + arr.Length() };
    return data;
  }

#ifdef RULE_HASH_STATS
  uint32_t    mUniversalSelectors;
  uint32_t    mNameSpaceSelectors;
  uint32_t    mTagSelectors;
  uint32_t    mClassSelectors;
  uint32_t    mIdSelectors;

  uint32_t    mElementsMatched;

  uint32_t    mElementUniversalCalls;
  uint32_t    mElementNameSpaceCalls;
  uint32_t    mElementTagCalls;
  uint32_t    mElementClassCalls;
  uint32_t    mElementIdCalls;
#endif 
};

RuleHash::RuleHash(bool aQuirksMode)
  : mRuleCount(0),
    mUniversalRules(0),
    mEnumList(nullptr), mEnumListSize(0),
    mQuirksMode(aQuirksMode)
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

  mTagTable.ops = nullptr;
  mIdTable.ops = nullptr;
  mClassTable.ops = nullptr;
  mNameSpaceTable.ops = nullptr;
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
    if (mUniversalRules.Length() > 0) {
      printf("  Universal rules:\n");
      for (uint32_t i = 0; i < mUniversalRules.Length(); ++i) {
        RuleValue* value = &(mUniversalRules[i]);
        nsAutoString selectorText;
        uint32_t lineNumber = value->mRule->GetLineNumber();
        nsRefPtr<CSSStyleSheet> cssSheet = value->mRule->GetStyleSheet();
        value->mSelector->ToString(selectorText, cssSheet);

        printf("    line %d, %s\n",
               lineNumber, NS_ConvertUTF16toUTF8(selectorText).get());
      }
    }
  }
#endif 
#endif 
  
  
  
  if (nullptr != mEnumList) {
    delete [] mEnumList;
  }
  
  if (mIdTable.ops) {
    PL_DHashTableFinish(&mIdTable);
  }
  if (mClassTable.ops) {
    PL_DHashTableFinish(&mClassTable);
  }
  if (mTagTable.ops) {
    PL_DHashTableFinish(&mTagTable);
  }
  if (mNameSpaceTable.ops) {
    PL_DHashTableFinish(&mNameSpaceTable);
  }
}

void RuleHash::AppendRuleToTable(PLDHashTable* aTable, const void* aKey,
                                 const RuleSelectorPair& aRuleInfo)
{
  
  RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                         (PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
  if (!entry)
    return;
  entry->mRules.AppendElement(RuleValue(aRuleInfo, mRuleCount++, mQuirksMode));
}

static void
AppendRuleToTagTable(PLDHashTable* aTable, nsIAtom* aKey,
                     const RuleValue& aRuleInfo)
{
  
  RuleHashTagTableEntry *entry = static_cast<RuleHashTagTableEntry*>
    (PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
  if (!entry)
    return;

  entry->mRules.AppendElement(aRuleInfo);
}

void RuleHash::AppendUniversalRule(const RuleSelectorPair& aRuleInfo)
{
  mUniversalRules.AppendElement(RuleValue(aRuleInfo, mRuleCount++, mQuirksMode));
}

void RuleHash::AppendRule(const RuleSelectorPair& aRuleInfo)
{
  nsCSSSelector *selector = aRuleInfo.mSelector;
  if (selector->IsPseudoElement()) {
    selector = selector->mNext;
  }
  if (nullptr != selector->mIDList) {
    if (!mIdTable.ops) {
      PL_DHashTableInit(&mIdTable,
                        mQuirksMode ? &RuleHash_IdTable_CIOps.ops
                                    : &RuleHash_IdTable_CSOps.ops,
                        nullptr, sizeof(RuleHashTableEntry));
    }
    AppendRuleToTable(&mIdTable, selector->mIDList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mIdSelectors);
  }
  else if (nullptr != selector->mClassList) {
    if (!mClassTable.ops) {
      PL_DHashTableInit(&mClassTable,
                        mQuirksMode ? &RuleHash_ClassTable_CIOps.ops
                                    : &RuleHash_ClassTable_CSOps.ops,
                        nullptr, sizeof(RuleHashTableEntry));
    }
    AppendRuleToTable(&mClassTable, selector->mClassList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mClassSelectors);
  }
  else if (selector->mLowercaseTag) {
    RuleValue ruleValue(aRuleInfo, mRuleCount++, mQuirksMode);
    if (!mTagTable.ops) {
      PL_DHashTableInit(&mTagTable, &RuleHash_TagTable_Ops, nullptr,
                        sizeof(RuleHashTagTableEntry));
    }
    AppendRuleToTagTable(&mTagTable, selector->mLowercaseTag, ruleValue);
    RULE_HASH_STAT_INCREMENT(mTagSelectors);
    if (selector->mCasedTag &&
        selector->mCasedTag != selector->mLowercaseTag) {
      AppendRuleToTagTable(&mTagTable, selector->mCasedTag, ruleValue);
      RULE_HASH_STAT_INCREMENT(mTagSelectors);
    }
  }
  else if (kNameSpaceID_Unknown != selector->mNameSpace) {
    if (!mNameSpaceTable.ops) {
      PL_DHashTableInit(&mNameSpaceTable, &RuleHash_NameSpaceTable_Ops, nullptr,
                        sizeof(RuleHashTableEntry));
    }
    AppendRuleToTable(&mNameSpaceTable,
                      NS_INT32_TO_PTR(selector->mNameSpace), aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mNameSpaceSelectors);
  }
  else {  
    AppendUniversalRule(aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mUniversalSelectors);
  }
}


#define MIN_ENUM_LIST_SIZE 8

#ifdef RULE_HASH_STATS
#define RULE_HASH_STAT_INCREMENT_LIST_COUNT(list_, var_) \
  (var_) += (list_).Length()
#else
#define RULE_HASH_STAT_INCREMENT_LIST_COUNT(list_, var_) \
  PR_BEGIN_MACRO PR_END_MACRO
#endif

static inline
void ContentEnumFunc(const RuleValue &value, nsCSSSelector* selector,
                     ElementDependentRuleProcessorData* data, NodeMatchContext& nodeContext,
                     AncestorFilter *ancestorFilter);

void RuleHash::EnumerateAllRules(Element* aElement, ElementDependentRuleProcessorData* aData,
                                 NodeMatchContext& aNodeContext)
{
  int32_t nameSpace = aElement->GetNameSpaceID();
  nsIAtom* tag = aElement->Tag();
  nsIAtom* id = aElement->GetID();
  const nsAttrValue* classList = aElement->GetClasses();

  NS_ABORT_IF_FALSE(tag, "How could we not have a tag?");

  int32_t classCount = classList ? classList->GetAtomCount() : 0;

  
  
  int32_t testCount = classCount + 4;

  if (mEnumListSize < testCount) {
    delete [] mEnumList;
    mEnumListSize = std::max(testCount, MIN_ENUM_LIST_SIZE);
    mEnumList = new EnumData[mEnumListSize];
  }

  int32_t valueCount = 0;
  RULE_HASH_STAT_INCREMENT(mElementsMatched);

  if (mUniversalRules.Length() != 0) { 
    mEnumList[valueCount++] = ToEnumData(mUniversalRules);
    RULE_HASH_STAT_INCREMENT_LIST_COUNT(mUniversalRules, mElementUniversalCalls);
  }
  
  if (kNameSpaceID_Unknown != nameSpace && mNameSpaceTable.ops) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mNameSpaceTable, NS_INT32_TO_PTR(nameSpace),
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementNameSpaceCalls);
    }
  }
  if (mTagTable.ops) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mTagTable, tag, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementTagCalls);
    }
  }
  if (id && mIdTable.ops) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mIdTable, id, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementIdCalls);
    }
  }
  if (mClassTable.ops) {
    for (int32_t index = 0; index < classCount; ++index) {
      RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                             (PL_DHashTableOperate(&mClassTable, classList->AtomAt(index),
                             PL_DHASH_LOOKUP));
      if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        mEnumList[valueCount++] = ToEnumData(entry->mRules);
        RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementClassCalls);
      }
    }
  }
  NS_ASSERTION(valueCount <= testCount, "values exceeded list size");

  if (valueCount > 0) {
    AncestorFilter *filter =
      aData->mTreeMatchContext.mAncestorFilter.HasFilter() ?
        &aData->mTreeMatchContext.mAncestorFilter : nullptr;
#ifdef DEBUG
    if (filter) {
      filter->AssertHasAllAncestors(aElement);
    }
#endif
    
    while (valueCount > 1) {
      int32_t valueIndex = 0;
      int32_t lowestRuleIndex = mEnumList[valueIndex].mCurValue->mIndex;
      for (int32_t index = 1; index < valueCount; ++index) {
        int32_t ruleIndex = mEnumList[index].mCurValue->mIndex;
        if (ruleIndex < lowestRuleIndex) {
          valueIndex = index;
          lowestRuleIndex = ruleIndex;
        }
      }
      const RuleValue *cur = mEnumList[valueIndex].mCurValue;
      ContentEnumFunc(*cur, cur->mSelector, aData, aNodeContext, filter);
      cur++;
      if (cur == mEnumList[valueIndex].mEnd) {
        mEnumList[valueIndex] = mEnumList[--valueCount];
      } else {
        mEnumList[valueIndex].mCurValue = cur;
      }
    }

    
    for (const RuleValue *value = mEnumList[0].mCurValue,
                         *end = mEnumList[0].mEnd;
         value != end; ++value) {
      ContentEnumFunc(*value, value->mSelector, aData, aNodeContext, filter);
    }
  }
}

static size_t
SizeOfRuleHashTableEntry(PLDHashEntryHdr* aHdr, MallocSizeOf aMallocSizeOf, void *)
{
  RuleHashTableEntry* entry = static_cast<RuleHashTableEntry*>(aHdr);
  return entry->mRules.SizeOfExcludingThis(aMallocSizeOf);
}

size_t
RuleHash::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;

  if (mIdTable.ops) {
    n += PL_DHashTableSizeOfExcludingThis(&mIdTable,
                                          SizeOfRuleHashTableEntry,
                                          aMallocSizeOf);
  }

  if (mClassTable.ops) {
    n += PL_DHashTableSizeOfExcludingThis(&mClassTable,
                                          SizeOfRuleHashTableEntry,
                                          aMallocSizeOf);
  }

  if (mTagTable.ops) {
    n += PL_DHashTableSizeOfExcludingThis(&mTagTable,
                                          SizeOfRuleHashTableEntry,
                                          aMallocSizeOf);
  }

  if (mNameSpaceTable.ops) {
    n += PL_DHashTableSizeOfExcludingThis(&mNameSpaceTable,
                                          SizeOfRuleHashTableEntry,
                                          aMallocSizeOf);
  }

  n += mUniversalRules.SizeOfExcludingThis(aMallocSizeOf);

  return n;
}

size_t
RuleHash::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}




struct AtomSelectorEntry : public PLDHashEntryHdr {
  nsIAtom *mAtom;
  
  
  nsAutoTArray<nsCSSSelector*, 2> mSelectors;
};

static void
AtomSelector_ClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  (static_cast<AtomSelectorEntry*>(hdr))->~AtomSelectorEntry();
}

static bool
AtomSelector_InitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                       const void *key)
{
  AtomSelectorEntry *entry = static_cast<AtomSelectorEntry*>(hdr);
  new (entry) AtomSelectorEntry();
  entry->mAtom = const_cast<nsIAtom*>(static_cast<const nsIAtom*>(key));
  return true;
}

static void
AtomSelector_MoveEntry(PLDHashTable *table, const PLDHashEntryHdr *from,
                       PLDHashEntryHdr *to)
{
  NS_PRECONDITION(from != to, "This is not going to work!");
  AtomSelectorEntry *oldEntry =
    const_cast<AtomSelectorEntry*>(static_cast<const AtomSelectorEntry*>(from));
  AtomSelectorEntry *newEntry = new (to) AtomSelectorEntry();
  newEntry->mAtom = oldEntry->mAtom;
  newEntry->mSelectors.SwapElements(oldEntry->mSelectors);
  oldEntry->~AtomSelectorEntry();
}

static nsIAtom*
AtomSelector_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const AtomSelectorEntry *entry = static_cast<const AtomSelectorEntry*>(hdr);
  return entry->mAtom;
}


static const PLDHashTableOps AtomSelector_CSOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PL_DHashMatchEntryStub,
  AtomSelector_MoveEntry,
  AtomSelector_ClearEntry,
  PL_DHashFinalizeStub,
  AtomSelector_InitEntry
};


static const RuleHashTableOps AtomSelector_CIOps = {
  {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  AtomSelector_MoveEntry,
  AtomSelector_ClearEntry,
  PL_DHashFinalizeStub,
  AtomSelector_InitEntry
  },
  AtomSelector_GetKey
};



struct RuleCascadeData {
  RuleCascadeData(nsIAtom *aMedium, bool aQuirksMode)
    : mRuleHash(aQuirksMode),
      mStateSelectors(),
      mSelectorDocumentStates(0),
      mKeyframesRuleTable(),
      mCounterStyleRuleTable(),
      mCacheKey(aMedium),
      mNext(nullptr),
      mQuirksMode(aQuirksMode)
  {
    
    
    PL_DHashTableInit(&mAttributeSelectors, &AtomSelector_CSOps, nullptr,
                      sizeof(AtomSelectorEntry));
    PL_DHashTableInit(&mAnonBoxRules, &RuleHash_TagTable_Ops, nullptr,
                      sizeof(RuleHashTagTableEntry));
    PL_DHashTableInit(&mIdSelectors,
                      aQuirksMode ? &AtomSelector_CIOps.ops :
                                    &AtomSelector_CSOps,
                      nullptr, sizeof(AtomSelectorEntry));
    PL_DHashTableInit(&mClassSelectors,
                      aQuirksMode ? &AtomSelector_CIOps.ops :
                                    &AtomSelector_CSOps,
                      nullptr, sizeof(AtomSelectorEntry));
    memset(mPseudoElementRuleHashes, 0, sizeof(mPseudoElementRuleHashes));
#ifdef MOZ_XUL
    PL_DHashTableInit(&mXULTreeRules, &RuleHash_TagTable_Ops, nullptr,
                      sizeof(RuleHashTagTableEntry));
#endif
  }

  ~RuleCascadeData()
  {
    PL_DHashTableFinish(&mAttributeSelectors);
    PL_DHashTableFinish(&mAnonBoxRules);
    PL_DHashTableFinish(&mIdSelectors);
    PL_DHashTableFinish(&mClassSelectors);
#ifdef MOZ_XUL
    PL_DHashTableFinish(&mXULTreeRules);
#endif
    for (uint32_t i = 0; i < ArrayLength(mPseudoElementRuleHashes); ++i) {
      delete mPseudoElementRuleHashes[i];
    }
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  RuleHash                 mRuleHash;
  RuleHash*
    mPseudoElementRuleHashes[nsCSSPseudoElements::ePseudo_PseudoElementCount];
  nsTArray<nsCSSRuleProcessor::StateSelector>  mStateSelectors;
  EventStates              mSelectorDocumentStates;
  PLDHashTable             mClassSelectors;
  PLDHashTable             mIdSelectors;
  nsTArray<nsCSSSelector*> mPossiblyNegatedClassSelectors;
  nsTArray<nsCSSSelector*> mPossiblyNegatedIDSelectors;
  PLDHashTable             mAttributeSelectors;
  PLDHashTable             mAnonBoxRules;
#ifdef MOZ_XUL
  PLDHashTable             mXULTreeRules;
#endif

  nsTArray<nsFontFaceRuleContainer> mFontFaceRules;
  nsTArray<nsCSSKeyframesRule*> mKeyframesRules;
  nsTArray<nsCSSFontFeatureValuesRule*> mFontFeatureValuesRules;
  nsTArray<nsCSSPageRule*> mPageRules;
  nsTArray<nsCSSCounterStyleRule*> mCounterStyleRules;

  nsDataHashtable<nsStringHashKey, nsCSSKeyframesRule*> mKeyframesRuleTable;
  nsDataHashtable<nsStringHashKey, nsCSSCounterStyleRule*> mCounterStyleRuleTable;

  
  
  nsTArray<nsCSSSelector*>* AttributeListFor(nsIAtom* aAttribute);

  nsMediaQueryResultCacheKey mCacheKey;
  RuleCascadeData*  mNext; 

  const bool mQuirksMode;
};

static size_t
SizeOfSelectorsEntry(PLDHashEntryHdr* aHdr, MallocSizeOf aMallocSizeOf, void *)
{
  AtomSelectorEntry* entry = static_cast<AtomSelectorEntry*>(aHdr);
  return entry->mSelectors.SizeOfExcludingThis(aMallocSizeOf);
}

static size_t
SizeOfKeyframesRuleEntryExcludingThis(nsStringHashKey::KeyType aKey,
                                      nsCSSKeyframesRule* const& aData,
                                      mozilla::MallocSizeOf aMallocSizeOf,
                                      void* aUserArg)
{
  
  
  
  
  
  

  return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

size_t
RuleCascadeData::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  n += mRuleHash.SizeOfExcludingThis(aMallocSizeOf);
  for (uint32_t i = 0; i < ArrayLength(mPseudoElementRuleHashes); ++i) {
    if (mPseudoElementRuleHashes[i])
      n += mPseudoElementRuleHashes[i]->SizeOfIncludingThis(aMallocSizeOf);
  }

  n += mStateSelectors.SizeOfExcludingThis(aMallocSizeOf);

  n += PL_DHashTableSizeOfExcludingThis(&mIdSelectors,
                                        SizeOfSelectorsEntry, aMallocSizeOf);
  n += PL_DHashTableSizeOfExcludingThis(&mClassSelectors,
                                        SizeOfSelectorsEntry, aMallocSizeOf);

  n += mPossiblyNegatedClassSelectors.SizeOfExcludingThis(aMallocSizeOf);
  n += mPossiblyNegatedIDSelectors.SizeOfExcludingThis(aMallocSizeOf);

  n += PL_DHashTableSizeOfExcludingThis(&mAttributeSelectors,
                                        SizeOfSelectorsEntry, aMallocSizeOf);
  n += PL_DHashTableSizeOfExcludingThis(&mAnonBoxRules,
                                        SizeOfRuleHashTableEntry, aMallocSizeOf);
#ifdef MOZ_XUL
  n += PL_DHashTableSizeOfExcludingThis(&mXULTreeRules,
                                        SizeOfRuleHashTableEntry, aMallocSizeOf);
#endif

  n += mFontFaceRules.SizeOfExcludingThis(aMallocSizeOf);
  n += mKeyframesRules.SizeOfExcludingThis(aMallocSizeOf);
  n += mFontFeatureValuesRules.SizeOfExcludingThis(aMallocSizeOf);
  n += mPageRules.SizeOfExcludingThis(aMallocSizeOf);
  n += mCounterStyleRules.SizeOfExcludingThis(aMallocSizeOf);
  n += mKeyframesRuleTable.SizeOfExcludingThis(SizeOfKeyframesRuleEntryExcludingThis,
                                               aMallocSizeOf,
                                               nullptr);

  return n;
}

nsTArray<nsCSSSelector*>*
RuleCascadeData::AttributeListFor(nsIAtom* aAttribute)
{
  AtomSelectorEntry *entry =
    static_cast<AtomSelectorEntry*>
               (PL_DHashTableOperate(&mAttributeSelectors, aAttribute,
                                     PL_DHASH_ADD));
  if (!entry)
    return nullptr;
  return &entry->mSelectors;
}





nsCSSRuleProcessor::nsCSSRuleProcessor(const sheet_array_type& aSheets,
                                       uint8_t aSheetType,
                                       Element* aScopeElement)
  : mSheets(aSheets)
  , mRuleCascades(nullptr)
  , mLastPresContext(nullptr)
  , mScopeElement(aScopeElement)
  , mSheetType(aSheetType)
{
  NS_ASSERTION(!!mScopeElement == (aSheetType == nsStyleSet::eScopedDocSheet),
               "aScopeElement must be specified iff aSheetType is "
               "eScopedDocSheet");
  for (sheet_array_type::size_type i = mSheets.Length(); i-- != 0; ) {
    mSheets[i]->AddRuleProcessor(this);
  }
}

nsCSSRuleProcessor::~nsCSSRuleProcessor()
{
  ClearSheets();
  ClearRuleCascades();
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSRuleProcessor)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRuleProcessor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSRuleProcessor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSRuleProcessor)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCSSRuleProcessor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCSSRuleProcessor)
  tmp->ClearSheets();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mScopeElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCSSRuleProcessor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mScopeElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

void
nsCSSRuleProcessor::ClearSheets()
{
  for (sheet_array_type::size_type i = mSheets.Length(); i-- != 0; ) {
    mSheets[i]->DropRuleProcessor(this);
  }
  mSheets.Clear();
}

 nsresult
nsCSSRuleProcessor::Startup()
{
  Preferences::AddBoolVarCache(&gSupportVisitedPseudo, VISITED_PSEUDO_PREF,
                               true);

  return NS_OK;
}

static bool
InitSystemMetrics()
{
  NS_ASSERTION(!sSystemMetrics, "already initialized");

  sSystemMetrics = new nsTArray< nsCOMPtr<nsIAtom> >;
  NS_ENSURE_TRUE(sSystemMetrics, false);

  




  int32_t metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ScrollArrowStyle);
  if (metricResult & LookAndFeel::eScrollArrow_StartBackward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_start_backward);
  }
  if (metricResult & LookAndFeel::eScrollArrow_StartForward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_start_forward);
  }
  if (metricResult & LookAndFeel::eScrollArrow_EndBackward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_end_backward);
  }
  if (metricResult & LookAndFeel::eScrollArrow_EndForward) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_end_forward);
  }

  metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ScrollSliderStyle);
  if (metricResult != LookAndFeel::eScrollThumbStyle_Normal) {
    sSystemMetrics->AppendElement(nsGkAtoms::scrollbar_thumb_proportional);
  }

  metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ImagesInMenus);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::images_in_menus);
  }

  metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ImagesInButtons);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::images_in_buttons);
  }

  metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::overlay_scrollbars);
  }

  metricResult =
    LookAndFeel::GetInt(LookAndFeel::eIntID_MenuBarDrag);
  if (metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::menubar_drag);
  }

  nsresult rv =
    LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsDefaultTheme, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_default_theme);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_MacGraphiteTheme, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::mac_graphite_theme);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_MacLionTheme, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::mac_lion_theme);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_MacYosemiteTheme, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::mac_yosemite_theme);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_DWMCompositor, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_compositor);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsGlass, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_glass);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_ColorPickerAvailable, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::color_picker_available);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsClassic, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_classic);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_TouchEnabled, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::touch_enabled);
  }
 
  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_SwipeAnimationEnabled,
                           &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::swipe_animation_enabled);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_PhysicalHomeButton,
                           &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::physical_home_button);
  }

#ifdef XP_WIN
  if (NS_SUCCEEDED(
        LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsThemeIdentifier,
                            &metricResult))) {
    nsCSSRuleProcessor::SetWindowsThemeIdentifier(static_cast<uint8_t>(metricResult));
    switch(metricResult) {
      case LookAndFeel::eWindowsTheme_Aero:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_aero);
        break;
      case LookAndFeel::eWindowsTheme_AeroLite:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_aero_lite);
        break;
      case LookAndFeel::eWindowsTheme_LunaBlue:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_luna_blue);
        break;
      case LookAndFeel::eWindowsTheme_LunaOlive:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_luna_olive);
        break;
      case LookAndFeel::eWindowsTheme_LunaSilver:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_luna_silver);
        break;
      case LookAndFeel::eWindowsTheme_Royale:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_royale);
        break;
      case LookAndFeel::eWindowsTheme_Zune:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_zune);
        break;
      case LookAndFeel::eWindowsTheme_Generic:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_generic);
        break;
    }
  }
#endif

  return true;
}

 void
nsCSSRuleProcessor::FreeSystemMetrics()
{
  delete sSystemMetrics;
  sSystemMetrics = nullptr;
}

 void
nsCSSRuleProcessor::Shutdown()
{
  FreeSystemMetrics();
}

 bool
nsCSSRuleProcessor::HasSystemMetric(nsIAtom* aMetric)
{
  if (!sSystemMetrics && !InitSystemMetrics()) {
    return false;
  }
  return sSystemMetrics->IndexOf(aMetric) != sSystemMetrics->NoIndex;
}

#ifdef XP_WIN
 uint8_t
nsCSSRuleProcessor::GetWindowsThemeIdentifier()
{
  if (!sSystemMetrics)
    InitSystemMetrics();
  return sWinThemeId;
}
#endif


EventStates
nsCSSRuleProcessor::GetContentState(Element* aElement, const TreeMatchContext& aTreeMatchContext)
{
  EventStates state = aElement->StyleState();

  
  
  
  
  if (state.HasState(NS_EVENT_STATE_VISITED) &&
      (!gSupportVisitedPseudo ||
       aElement->OwnerDoc()->IsBeingUsedAsImage() ||
       aTreeMatchContext.mUsingPrivateBrowsing)) {
    state &= ~NS_EVENT_STATE_VISITED;
    state |= NS_EVENT_STATE_UNVISITED;
  }
  return state;
}


bool
nsCSSRuleProcessor::IsLink(Element* aElement)
{
  EventStates state = aElement->StyleState();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED);
}


EventStates
nsCSSRuleProcessor::GetContentStateForVisitedHandling(
                     Element* aElement,
                     const TreeMatchContext& aTreeMatchContext,
                     nsRuleWalker::VisitedHandlingType aVisitedHandling,
                     bool aIsRelevantLink)
{
  EventStates contentState = GetContentState(aElement, aTreeMatchContext);
  if (contentState.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED)) {
    NS_ABORT_IF_FALSE(IsLink(aElement), "IsLink() should match state");
    contentState &= ~(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED);
    if (aIsRelevantLink) {
      switch (aVisitedHandling) {
        case nsRuleWalker::eRelevantLinkUnvisited:
          contentState |= NS_EVENT_STATE_UNVISITED;
          break;
        case nsRuleWalker::eRelevantLinkVisited:
          contentState |= NS_EVENT_STATE_VISITED;
          break;
        case nsRuleWalker::eLinksVisitedOrUnvisited:
          contentState |= NS_EVENT_STATE_UNVISITED | NS_EVENT_STATE_VISITED;
          break;
      }
    } else {
      contentState |= NS_EVENT_STATE_UNVISITED;
    }
  }
  return contentState;
}











struct NodeMatchContext {
  
  
  
  
  
  
  
  
  
  
  
  const EventStates mStateMask;

  
  
  
  
  
  
  
  
  
  const bool mIsRelevantLink;

  NodeMatchContext(EventStates aStateMask, bool aIsRelevantLink)
    : mStateMask(aStateMask)
    , mIsRelevantLink(aIsRelevantLink)
  {
  }
};

static bool ValueIncludes(const nsSubstring& aValueList,
                            const nsSubstring& aValue,
                            const nsStringComparator& aComparator)
{
  const char16_t *p = aValueList.BeginReading(),
              *p_end = aValueList.EndReading();

  while (p < p_end) {
    
    while (p != p_end && nsContentUtils::IsHTMLWhitespace(*p))
      ++p;

    const char16_t *val_start = p;

    
    while (p != p_end && !nsContentUtils::IsHTMLWhitespace(*p))
      ++p;

    const char16_t *val_end = p;

    if (val_start < val_end &&
        aValue.Equals(Substring(val_start, val_end), aComparator))
      return true;

    ++p; 
  }
  return false;
}





inline bool IsQuirkEventSensitive(nsIAtom *aContentTag)
{
  return bool ((nsGkAtoms::button == aContentTag) ||
                 (nsGkAtoms::img == aContentTag)    ||
                 (nsGkAtoms::input == aContentTag)  ||
                 (nsGkAtoms::label == aContentTag)  ||
                 (nsGkAtoms::select == aContentTag) ||
                 (nsGkAtoms::textarea == aContentTag));
}


static inline bool
IsSignificantChild(nsIContent* aChild, bool aTextIsSignificant,
                   bool aWhitespaceIsSignificant)
{
  return nsStyleUtil::IsSignificantChild(aChild, aTextIsSignificant,
                                         aWhitespaceIsSignificant);
}




static bool AttrMatchesValue(const nsAttrSelector* aAttrSelector,
                               const nsString& aValue, bool isHTML)
{
  NS_PRECONDITION(aAttrSelector, "Must have an attribute selector");

  
  
  
  if (aAttrSelector->mValue.IsEmpty() &&
      (aAttrSelector->mFunction == NS_ATTR_FUNC_INCLUDES ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_ENDSMATCH ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_BEGINSMATCH ||
       aAttrSelector->mFunction == NS_ATTR_FUNC_CONTAINSMATCH))
    return false;

  const nsDefaultStringComparator defaultComparator;
  const nsASCIICaseInsensitiveStringComparator ciComparator;
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
      return false;
  }
}

static inline bool
edgeChildMatches(Element* aElement, TreeMatchContext& aTreeMatchContext,
                 bool checkFirst, bool checkLast)
{
  nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling)
    parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

  return (!checkFirst ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, false, false, true) == 1) &&
         (!checkLast ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, false, true, true) == 1);
}

static inline bool
nthChildGenericMatches(Element* aElement,
                       TreeMatchContext& aTreeMatchContext,
                       nsPseudoClassList* pseudoClass,
                       bool isOfType, bool isFromEnd)
{
  nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling) {
    if (isFromEnd)
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
    else
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  }

  const int32_t index = aTreeMatchContext.mNthIndexCache.
    GetNthIndex(aElement, isOfType, isFromEnd, false);
  if (index <= 0) {
    
    return false;
  }

  const int32_t a = pseudoClass->u.mNumbers[0];
  const int32_t b = pseudoClass->u.mNumbers[1];
  
  
  if (a == 0) {
    return b == index;
  }

  
  
  
  const int32_t n = (index - b) / a;
  return n >= 0 && (a * n == index - b);
}

static inline bool
edgeOfTypeMatches(Element* aElement, TreeMatchContext& aTreeMatchContext,
                  bool checkFirst, bool checkLast)
{
  nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling) {
    if (checkLast)
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR);
    else
      parent->SetFlags(NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  }

  return (!checkFirst ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, true, false, true) == 1) &&
         (!checkLast ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, true, true, true) == 1);
}

static inline bool
checkGenericEmptyMatches(Element* aElement,
                         TreeMatchContext& aTreeMatchContext,
                         bool isWhitespaceSignificant)
{
  nsIContent *child = nullptr;
  int32_t index = -1;

  if (aTreeMatchContext.mForStyling)
    aElement->SetFlags(NODE_HAS_EMPTY_SELECTOR);

  do {
    child = aElement->GetChildAt(++index);
    
    
  } while (child && !IsSignificantChild(child, true, isWhitespaceSignificant));
  return (child == nullptr);
}


static const EventStates sPseudoClassStateDependences[] = {
#define CSS_PSEUDO_CLASS(_name, _value, _pref)  \
  EventStates(),
#define CSS_STATE_DEPENDENT_PSEUDO_CLASS(_name, _value, _pref, _states)  \
  _states,
#include "nsCSSPseudoClassList.h"
#undef CSS_STATE_DEPENDENT_PSEUDO_CLASS
#undef CSS_PSEUDO_CLASS
  
  
  EventStates(),
  EventStates()
};

static const EventStates sPseudoClassStates[] = {
#define CSS_PSEUDO_CLASS(_name, _value, _pref)  \
  EventStates(),
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _pref, _states) \
  _states,
#include "nsCSSPseudoClassList.h"
#undef CSS_STATE_PSEUDO_CLASS
#undef CSS_PSEUDO_CLASS
  
  
  EventStates(),
  EventStates()
};
static_assert(MOZ_ARRAY_LENGTH(sPseudoClassStates) ==
              nsCSSPseudoClasses::ePseudoClass_NotPseudoClass + 1,
              "ePseudoClass_NotPseudoClass is no longer at the end of"
              "sPseudoClassStates");

static bool
StateSelectorMatches(Element* aElement,
                     nsCSSSelector* aSelector,
                     NodeMatchContext& aNodeMatchContext,
                     TreeMatchContext& aTreeMatchContext,
                     bool* const aDependence,
                     EventStates aStatesToCheck)
{
  NS_PRECONDITION(!aStatesToCheck.IsEmpty(),
                  "should only need to call StateSelectorMatches if "
                  "aStatesToCheck is not empty");

  const bool isNegated = aDependence != nullptr;

  
  if (aStatesToCheck.HasAtLeastOneOfStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE) &&
      aTreeMatchContext.mCompatMode == eCompatibility_NavQuirks &&
      
      !aSelector->HasTagSelector() && !aSelector->mIDList &&
      !aSelector->mClassList && !aSelector->mAttrList &&
      
      
      
      
      !isNegated &&
      
      aElement->IsHTML() && !nsCSSRuleProcessor::IsLink(aElement) &&
      !IsQuirkEventSensitive(aElement->Tag())) {
    
    
    return false;
  }

  if (aTreeMatchContext.mForStyling &&
      aStatesToCheck.HasAtLeastOneOfStates(NS_EVENT_STATE_HOVER)) {
    
    aElement->SetHasRelevantHoverRules();
  }

  if (aNodeMatchContext.mStateMask.HasAtLeastOneOfStates(aStatesToCheck)) {
    if (aDependence) {
      *aDependence = true;
    }
  } else {
    EventStates contentState =
      nsCSSRuleProcessor::GetContentStateForVisitedHandling(
                                   aElement,
                                   aTreeMatchContext,
                                   aTreeMatchContext.VisitedHandling(),
                                   aNodeMatchContext.mIsRelevantLink);
    if (!contentState.HasAtLeastOneOfStates(aStatesToCheck)) {
      return false;
    }
  }

  return true;
}

static bool
StateSelectorMatches(Element* aElement,
                     nsCSSSelector* aSelector,
                     NodeMatchContext& aNodeMatchContext,
                     TreeMatchContext& aTreeMatchContext)
{
  for (nsPseudoClassList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    EventStates statesToCheck = sPseudoClassStates[pseudoClass->mType];
    if (!statesToCheck.IsEmpty() &&
        !StateSelectorMatches(aElement, aSelector, aNodeMatchContext,
                              aTreeMatchContext, nullptr, statesToCheck)) {
      return false;
    }
  }
  return true;
}






static bool SelectorMatches(Element* aElement,
                              nsCSSSelector* aSelector,
                              NodeMatchContext& aNodeMatchContext,
                              TreeMatchContext& aTreeMatchContext,
                              bool* const aDependence = nullptr)

{
  NS_PRECONDITION(!aSelector->IsPseudoElement(),
                  "Pseudo-element snuck into SelectorMatches?");
  NS_ABORT_IF_FALSE(aTreeMatchContext.mForStyling ||
                    !aNodeMatchContext.mIsRelevantLink,
                    "mIsRelevantLink should be set to false when mForStyling "
                    "is false since we don't know how to set it correctly in "
                    "Has(Attribute|State)DependentStyle");

  
  
  if ((kNameSpaceID_Unknown != aSelector->mNameSpace &&
       aElement->GetNameSpaceID() != aSelector->mNameSpace))
    return false;

  if (aSelector->mLowercaseTag) {
    nsIAtom* selectorTag =
      (aTreeMatchContext.mIsHTMLDocument && aElement->IsHTML()) ?
        aSelector->mLowercaseTag : aSelector->mCasedTag;
    if (selectorTag != aElement->Tag()) {
      return false;
    }
  }

  nsAtomList* IDList = aSelector->mIDList;
  if (IDList) {
    nsIAtom* id = aElement->GetID();
    if (id) {
      
      const bool isCaseSensitive =
        aTreeMatchContext.mCompatMode != eCompatibility_NavQuirks;

      if (isCaseSensitive) {
        do {
          if (IDList->mAtom != id) {
            return false;
          }
          IDList = IDList->mNext;
        } while (IDList);
      } else {
        
        
        
        nsDependentAtomString id1Str(id);
        do {
          if (!nsContentUtils::EqualsIgnoreASCIICase(id1Str,
                 nsDependentAtomString(IDList->mAtom))) {
            return false;
          }
          IDList = IDList->mNext;
        } while (IDList);
      }
    } else {
      
      return false;
    }
  }

  nsAtomList* classList = aSelector->mClassList;
  if (classList) {
    
    const nsAttrValue *elementClasses = aElement->GetClasses();
    if (!elementClasses) {
      
      return false;
    }

    
    const bool isCaseSensitive =
      aTreeMatchContext.mCompatMode != eCompatibility_NavQuirks;

    while (classList) {
      if (!elementClasses->Contains(classList->mAtom,
                                    isCaseSensitive ?
                                      eCaseMatters : eIgnoreCase)) {
        return false;
      }
      classList = classList->mNext;
    }
  }

  const bool isNegated = (aDependence != nullptr);
  
  
  
  
  
  NS_ASSERTION(aNodeMatchContext.mStateMask.IsEmpty() ||
               !aTreeMatchContext.mForStyling,
               "mForStyling must be false if we're just testing for "
               "state-dependence");

  
  for (nsPseudoClassList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    EventStates statesToCheck = sPseudoClassStates[pseudoClass->mType];
    if (statesToCheck.IsEmpty()) {
      
      
      switch (pseudoClass->mType) {
      case nsCSSPseudoClasses::ePseudoClass_empty:
        if (!checkGenericEmptyMatches(aElement, aTreeMatchContext, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozOnlyWhitespace:
        if (!checkGenericEmptyMatches(aElement, aTreeMatchContext, false)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozEmptyExceptChildrenWithLocalname:
        {
          NS_ASSERTION(pseudoClass->u.mString, "Must have string!");
          nsIContent *child = nullptr;
          int32_t index = -1;

          if (aTreeMatchContext.mForStyling)
            
            
            
            
            
            aElement->SetFlags(NODE_HAS_SLOW_SELECTOR);
          do {
            child = aElement->GetChildAt(++index);
          } while (child &&
                   (!IsSignificantChild(child, true, false) ||
                    (child->GetNameSpaceID() == aElement->GetNameSpaceID() &&
                     child->Tag()->Equals(nsDependentString(pseudoClass->u.mString)))));
          if (child != nullptr) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_lang:
        {
          NS_ASSERTION(nullptr != pseudoClass->u.mString, "null lang parameter");
          if (!pseudoClass->u.mString || !*pseudoClass->u.mString) {
            return false;
          }

          
          
          
          
          nsAutoString language;
          aElement->GetLang(language);
          if (!language.IsEmpty()) {
            if (!nsStyleUtil::DashMatchCompare(language,
                                               nsDependentString(pseudoClass->u.mString),
                                               nsASCIICaseInsensitiveStringComparator())) {
              return false;
            }
            
            break;
          }

          nsIDocument* doc = aTreeMatchContext.mDocument;
          if (doc) {
            
            
            
            
            doc->GetContentLanguage(language);

            nsDependentString langString(pseudoClass->u.mString);
            language.StripWhitespace();
            int32_t begin = 0;
            int32_t len = language.Length();
            while (begin < len) {
              int32_t end = language.FindChar(char16_t(','), begin);
              if (end == kNotFound) {
                end = len;
              }
              if (nsStyleUtil::DashMatchCompare(Substring(language, begin,
                                                          end-begin),
                                                langString,
                                                nsASCIICaseInsensitiveStringComparator())) {
                break;
              }
              begin = end + 1;
            }
            if (begin < len) {
              
              break;
            }
          }

          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozBoundElement:
        if (aTreeMatchContext.mScopedRoot != aElement) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_root:
        if (aElement != aElement->OwnerDoc()->GetRootElement()) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_any:
        {
          nsCSSSelectorList *l;
          for (l = pseudoClass->u.mSelectors; l; l = l->mNext) {
            nsCSSSelector *s = l->mSelectors;
            NS_ABORT_IF_FALSE(!s->mNext && !s->IsPseudoElement(),
                              "parser failed");
            if (SelectorMatches(aElement, s, aNodeMatchContext,
                                aTreeMatchContext)) {
              break;
            }
          }
          if (!l) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_firstChild:
        if (!edgeChildMatches(aElement, aTreeMatchContext, true, false)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_firstNode:
        {
          nsIContent *firstNode = nullptr;
          nsIContent *parent = aElement->GetParent();
          if (parent) {
            if (aTreeMatchContext.mForStyling)
              parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);

            int32_t index = -1;
            do {
              firstNode = parent->GetChildAt(++index);
              
            } while (firstNode &&
                     !IsSignificantChild(firstNode, true, false));
          }
          if (aElement != firstNode) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_lastChild:
        if (!edgeChildMatches(aElement, aTreeMatchContext, false, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_lastNode:
        {
          nsIContent *lastNode = nullptr;
          nsIContent *parent = aElement->GetParent();
          if (parent) {
            if (aTreeMatchContext.mForStyling)
              parent->SetFlags(NODE_HAS_EDGE_CHILD_SELECTOR);
            
            uint32_t index = parent->GetChildCount();
            do {
              lastNode = parent->GetChildAt(--index);
              
            } while (lastNode &&
                     !IsSignificantChild(lastNode, true, false));
          }
          if (aElement != lastNode) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_onlyChild:
        if (!edgeChildMatches(aElement, aTreeMatchContext, true, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_firstOfType:
        if (!edgeOfTypeMatches(aElement, aTreeMatchContext, true, false)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_lastOfType:
        if (!edgeOfTypeMatches(aElement, aTreeMatchContext, false, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_onlyOfType:
        if (!edgeOfTypeMatches(aElement, aTreeMatchContext, true, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_nthChild:
        if (!nthChildGenericMatches(aElement, aTreeMatchContext, pseudoClass,
                                    false, false)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_nthLastChild:
        if (!nthChildGenericMatches(aElement, aTreeMatchContext, pseudoClass,
                                    false, true)) {
          return false;
        }
      break;

      case nsCSSPseudoClasses::ePseudoClass_nthOfType:
        if (!nthChildGenericMatches(aElement, aTreeMatchContext, pseudoClass,
                                    true, false)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_nthLastOfType:
        if (!nthChildGenericMatches(aElement, aTreeMatchContext, pseudoClass,
                                    true, true)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozIsHTML:
        if (!aTreeMatchContext.mIsHTMLDocument || !aElement->IsHTML()) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozSystemMetric:
        {
          nsCOMPtr<nsIAtom> metric = do_GetAtom(pseudoClass->u.mString);
          if (!nsCSSRuleProcessor::HasSystemMetric(metric)) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozLocaleDir:
        {
          bool docIsRTL =
            aTreeMatchContext.mDocument->GetDocumentState().
              HasState(NS_DOCUMENT_STATE_RTL_LOCALE);

          nsDependentString dirString(pseudoClass->u.mString);
          NS_ASSERTION(dirString.EqualsLiteral("ltr") ||
                       dirString.EqualsLiteral("rtl"),
                       "invalid value for -moz-locale-dir");

          if (dirString.EqualsLiteral("rtl") != docIsRTL) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozLWTheme:
        {
          if (aTreeMatchContext.mDocument->GetDocumentLWTheme() <=
                nsIDocument::Doc_Theme_None) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozLWThemeBrightText:
        {
          if (aTreeMatchContext.mDocument->GetDocumentLWTheme() !=
                nsIDocument::Doc_Theme_Bright) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozLWThemeDarkText:
        {
          if (aTreeMatchContext.mDocument->GetDocumentLWTheme() !=
                nsIDocument::Doc_Theme_Dark) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozWindowInactive:
        if (!aTreeMatchContext.mDocument->GetDocumentState().
               HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE)) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_mozTableBorderNonzero:
        {
          if (!aElement->IsHTML(nsGkAtoms::table)) {
            return false;
          }
          const nsAttrValue *val = aElement->GetParsedAttr(nsGkAtoms::border);
          if (!val ||
              (val->Type() == nsAttrValue::eInteger &&
               val->GetIntegerValue() == 0)) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_dir:
        {
          if (aDependence) {
            EventStates states
              = sPseudoClassStateDependences[pseudoClass->mType];
            if (aNodeMatchContext.mStateMask.HasAtLeastOneOfStates(states)) {
              *aDependence = true;
              return false;
            }
          }

          
          
          
          
          
          
          
          
          
          EventStates state = aElement->StyleState();
          bool elementIsRTL = state.HasState(NS_EVENT_STATE_RTL);
          bool elementIsLTR = state.HasState(NS_EVENT_STATE_LTR);
          nsDependentString dirString(pseudoClass->u.mString);

          if ((dirString.EqualsLiteral("rtl") && !elementIsRTL) ||
              (dirString.EqualsLiteral("ltr") && !elementIsLTR)) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_scope:
        if (aTreeMatchContext.mForScopedStyle) {
          if (aTreeMatchContext.mCurrentStyleScope) {
            
            
            
            
            
            
            
            return false;
          }
        } else if (aTreeMatchContext.HasSpecifiedScope()) {
          if (!aTreeMatchContext.IsScopeElement(aElement)) {
            return false;
          }
        } else {
          if (aElement != aElement->OwnerDoc()->GetRootElement()) {
            return false;
          }
        }
        break;

      default:
        NS_ABORT_IF_FALSE(false, "How did that happen?");
      }
    } else {
      if (!StateSelectorMatches(aElement, aSelector, aNodeMatchContext,
                                aTreeMatchContext, aDependence,
                                statesToCheck)) {
        return false;
      }
    }
  }

  bool result = true;
  if (aSelector->mAttrList) {
    
    if (!aElement->HasAttrs()) {
      
      return false;
    } else {
      result = true;
      nsAttrSelector* attr = aSelector->mAttrList;
      nsIAtom* matchAttribute;

      do {
        bool isHTML =
          (aTreeMatchContext.mIsHTMLDocument && aElement->IsHTML());
        matchAttribute = isHTML ? attr->mLowercaseAttr : attr->mCasedAttr;
        if (attr->mNameSpace == kNameSpaceID_Unknown) {
          
          
          
          
          
          
          
          result = false;
          const nsAttrName* attrName;
          for (uint32_t i = 0; (attrName = aElement->GetAttrNameAt(i)); ++i) {
            if (attrName->LocalName() != matchAttribute) {
              continue;
            }
            if (attr->mFunction == NS_ATTR_FUNC_SET) {
              result = true;
            } else {
              nsAutoString value;
#ifdef DEBUG
              bool hasAttr =
#endif
                aElement->GetAttr(attrName->NamespaceID(),
                                  attrName->LocalName(), value);
              NS_ASSERTION(hasAttr, "GetAttrNameAt lied");
              result = AttrMatchesValue(attr, value, isHTML);
            }

            
            
            
            
            
            if (result) {
              break;
            }
          }
        }
        else if (attr->mFunction == NS_ATTR_FUNC_EQUALS) {
          result =
            aElement->
              AttrValueIs(attr->mNameSpace, matchAttribute, attr->mValue,
                          (!isHTML || attr->mCaseSensitive) ? eCaseMatters
                                                            : eIgnoreCase);
        }
        else if (!aElement->HasAttr(attr->mNameSpace, matchAttribute)) {
          result = false;
        }
        else if (attr->mFunction != NS_ATTR_FUNC_SET) {
          nsAutoString value;
#ifdef DEBUG
          bool hasAttr =
#endif
              aElement->GetAttr(attr->mNameSpace, matchAttribute, value);
          NS_ASSERTION(hasAttr, "HasAttr lied");
          result = AttrMatchesValue(attr, value, isHTML);
        }
        
        attr = attr->mNext;
      } while (attr && result);
    }
  }

  
  if (!isNegated) {
    for (nsCSSSelector *negation = aSelector->mNegations;
         result && negation; negation = negation->mNegations) {
      bool dependence = false;
      result = !SelectorMatches(aElement, negation, aNodeMatchContext,
                                aTreeMatchContext, &dependence);
      
      
      
      
      result = result || dependence;
    }
  }
  return result;
}

#undef STATE_CHECK





#define NS_IS_GREEDY_OPERATOR(ch) \
  ((ch) == char16_t(' ') || (ch) == char16_t('~'))

static bool SelectorMatchesTree(Element* aPrevElement,
                                  nsCSSSelector* aSelector,
                                  TreeMatchContext& aTreeMatchContext,
                                  bool aLookForRelevantLink)
{
  MOZ_ASSERT(!aSelector || !aSelector->IsPseudoElement());
  nsCSSSelector* selector = aSelector;
  Element* prevElement = aPrevElement;
  while (selector) { 
    NS_ASSERTION(!selector->mNext ||
                 selector->mNext->mOperator != char16_t(0),
                 "compound selector without combinator");

    
    
    if (aTreeMatchContext.mForScopedStyle &&
        !aTreeMatchContext.IsWithinStyleScopeForSelectorMatching()) {
      return false;
    }

    
    
    Element* element = nullptr;
    if (char16_t('+') == selector->mOperator ||
        char16_t('~') == selector->mOperator) {
      
      aLookForRelevantLink = false;
      nsIContent* parent = prevElement->GetParent();
      if (parent) {
        if (aTreeMatchContext.mForStyling)
          parent->SetFlags(NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);

        element = prevElement->GetPreviousElementSibling();
      }
    }
    
    
    else {
      nsIContent *content = prevElement->GetParent();
      
      
      if (content && content->IsElement()) {
        element = content->AsElement();
        if (aTreeMatchContext.mForScopedStyle) {
          
          
          
          
          aTreeMatchContext.PopStyleScopeForSelectorMatching(element);
        }

        
        
        
        
        if (selector->mOperator == '>' && element->IsActiveChildrenElement()) {
          Element* styleScope = aTreeMatchContext.mCurrentStyleScope;
          if (SelectorMatchesTree(element, selector, aTreeMatchContext,
                                  aLookForRelevantLink)) {
            
            
            return true;
          }
          
          
          
          
          aTreeMatchContext.mCurrentStyleScope = styleScope;
        }
      }
    }
    if (!element) {
      return false;
    }
    NodeMatchContext nodeContext(EventStates(),
                                 aLookForRelevantLink &&
                                   nsCSSRuleProcessor::IsLink(element));
    if (nodeContext.mIsRelevantLink) {
      
      
      
      
      
      
      
      aLookForRelevantLink = false;
      aTreeMatchContext.SetHaveRelevantLink();
    }
    if (SelectorMatches(element, selector, nodeContext, aTreeMatchContext)) {
      
      
      
      
      
      if (NS_IS_GREEDY_OPERATOR(selector->mOperator) &&
          selector->mNext &&
          selector->mNext->mOperator != selector->mOperator &&
          !(selector->mOperator == '~' &&
            NS_IS_ANCESTOR_OPERATOR(selector->mNext->mOperator))) {

        
        

        
        
        
        
        Element* styleScope = aTreeMatchContext.mCurrentStyleScope;
        if (SelectorMatchesTree(element, selector, aTreeMatchContext,
                                aLookForRelevantLink)) {
          return true;
        }
        
        
        
        
        aTreeMatchContext.mCurrentStyleScope = styleScope;
      }
      selector = selector->mNext;
    }
    else {
      
      
      if (!NS_IS_GREEDY_OPERATOR(selector->mOperator)) {
        return false;  
      }
    }
    prevElement = element;
  }
  return true; 
}

static inline
void ContentEnumFunc(const RuleValue& value, nsCSSSelector* aSelector,
                     ElementDependentRuleProcessorData* data, NodeMatchContext& nodeContext,
                     AncestorFilter *ancestorFilter)
{
  if (nodeContext.mIsRelevantLink) {
    data->mTreeMatchContext.SetHaveRelevantLink();
  }
  if (ancestorFilter &&
      !ancestorFilter->MightHaveMatchingAncestor<RuleValue::eMaxAncestorHashes>(
          value.mAncestorSelectorHashes)) {
    
    return;
  }
  if (!data->mTreeMatchContext.SetStyleScopeForSelectorMatching(data->mElement,
                                                                data->mScope)) {
    
    
    return;
  }
  nsCSSSelector* selector = aSelector;
  if (selector->IsPseudoElement()) {
    PseudoElementRuleProcessorData* pdata =
      static_cast<PseudoElementRuleProcessorData*>(data);
    if (!pdata->mPseudoElement && selector->mPseudoClassList) {
      
      
      
      
      
      
      
      
      
      
      return;
    }
    if (!StateSelectorMatches(pdata->mPseudoElement, aSelector, nodeContext,
                              data->mTreeMatchContext)) {
      return;
    }
    selector = selector->mNext;
  }
  if (SelectorMatches(data->mElement, selector, nodeContext,
                      data->mTreeMatchContext)) {
    nsCSSSelector *next = selector->mNext;
    if (!next || SelectorMatchesTree(data->mElement, next,
                                     data->mTreeMatchContext,
                                     !nodeContext.mIsRelevantLink)) {
      css::StyleRule *rule = value.mRule;
      rule->RuleMatched();
      data->mRuleWalker->Forward(rule);
      
    }
  }
}

 void
nsCSSRuleProcessor::RulesMatching(ElementRuleProcessorData *aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    NodeMatchContext nodeContext(EventStates(),
                                 nsCSSRuleProcessor::IsLink(aData->mElement));
    cascade->mRuleHash.EnumerateAllRules(aData->mElement, aData, nodeContext);
  }
}

 void
nsCSSRuleProcessor::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    RuleHash* ruleHash = cascade->mPseudoElementRuleHashes[aData->mPseudoType];
    if (ruleHash) {
      NodeMatchContext nodeContext(EventStates(),
                                   nsCSSRuleProcessor::IsLink(aData->mElement));
      ruleHash->EnumerateAllRules(aData->mElement, aData, nodeContext);
    }
  }
}

 void
nsCSSRuleProcessor::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade && cascade->mAnonBoxRules.EntryCount()) {
    RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>
      (PL_DHashTableOperate(&cascade->mAnonBoxRules, aData->mPseudoTag,
                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      nsTArray<RuleValue>& rules = entry->mRules;
      for (RuleValue *value = rules.Elements(), *end = value + rules.Length();
           value != end; ++value) {
        value->mRule->RuleMatched();
        aData->mRuleWalker->Forward(value->mRule);
      }
    }
  }
}

#ifdef MOZ_XUL
 void
nsCSSRuleProcessor::RulesMatching(XULTreeRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade && cascade->mXULTreeRules.EntryCount()) {
    RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>
      (PL_DHashTableOperate(&cascade->mXULTreeRules, aData->mPseudoTag,
                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      NodeMatchContext nodeContext(EventStates(),
                                   nsCSSRuleProcessor::IsLink(aData->mElement));
      nsTArray<RuleValue>& rules = entry->mRules;
      for (RuleValue *value = rules.Elements(), *end = value + rules.Length();
           value != end; ++value) {
        if (aData->mComparator->PseudoMatches(value->mSelector)) {
          ContentEnumFunc(*value, value->mSelector->mNext, aData, nodeContext,
                          nullptr);
        }
      }
    }
  }
}
#endif

static inline nsRestyleHint RestyleHintForOp(char16_t oper)
{
  if (oper == char16_t('+') || oper == char16_t('~')) {
    return eRestyle_LaterSiblings;
  }

  if (oper != char16_t(0)) {
    return eRestyle_Subtree;
  }

  return eRestyle_Self;
}

nsRestyleHint
nsCSSRuleProcessor::HasStateDependentStyle(ElementDependentRuleProcessorData* aData,
                                           Element* aStatefulElement,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           EventStates aStateMask)
{
  MOZ_ASSERT(!aData->mTreeMatchContext.mForScopedStyle,
             "mCurrentStyleScope will need to be saved and restored after the "
             "SelectorMatchesTree call");

  bool isPseudoElement =
    aPseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement;

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  
  
  nsRestyleHint hint = nsRestyleHint(0);
  if (cascade) {
    StateSelector *iter = cascade->mStateSelectors.Elements(),
                  *end = iter + cascade->mStateSelectors.Length();
    NodeMatchContext nodeContext(aStateMask, false);
    for(; iter != end; ++iter) {
      nsCSSSelector* selector = iter->mSelector;
      EventStates states = iter->mStates;

      if (selector->IsPseudoElement() != isPseudoElement) {
        continue;
      }

      nsCSSSelector* selectorForPseudo;
      if (isPseudoElement) {
        if (selector->PseudoType() != aPseudoType) {
          continue;
        }
        selectorForPseudo = selector;
        selector = selector->mNext;
      }

      nsRestyleHint possibleChange = RestyleHintForOp(selector->mOperator);

      
      
      
      
      
      if ((possibleChange & ~hint) &&
          states.HasAtLeastOneOfStates(aStateMask) &&
          
          
          
          
          
          
          
          
          (states != NS_EVENT_STATE_HOVER ||
           aStatefulElement->HasRelevantHoverRules() ||
           selector->mIDList || selector->mClassList ||
           
           
           (selector->mPseudoClassList &&
            (selector->mPseudoClassList->mNext ||
             selector->mPseudoClassList->mType !=
               nsCSSPseudoClasses::ePseudoClass_hover)) ||
           selector->mAttrList || selector->mNegations) &&
          (!isPseudoElement ||
           StateSelectorMatches(aStatefulElement, selectorForPseudo,
                                nodeContext, aData->mTreeMatchContext,
                                nullptr, aStateMask)) &&
          SelectorMatches(aData->mElement, selector, nodeContext,
                          aData->mTreeMatchContext) &&
          SelectorMatchesTree(aData->mElement, selector->mNext,
                              aData->mTreeMatchContext,
                              false))
      {
        hint = nsRestyleHint(hint | possibleChange);
      }
    }
  }
  return hint;
}

nsRestyleHint
nsCSSRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return HasStateDependentStyle(aData,
                                aData->mElement,
                                nsCSSPseudoElements::ePseudo_NotPseudoElement,
                                aData->mStateMask);
}

nsRestyleHint
nsCSSRuleProcessor::HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData)
{
  return HasStateDependentStyle(aData,
                                aData->mPseudoElement,
                                aData->mPseudoType,
                                aData->mStateMask);
}

bool
nsCSSRuleProcessor::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  return cascade && cascade->mSelectorDocumentStates.HasAtLeastOneOfStates(aData->mStateMask);
}

struct AttributeEnumData {
  explicit AttributeEnumData(AttributeRuleProcessorData *aData)
    : data(aData), change(nsRestyleHint(0)) {}

  AttributeRuleProcessorData *data;
  nsRestyleHint change;
};


static void
AttributeEnumFunc(nsCSSSelector* aSelector, AttributeEnumData* aData)
{
  AttributeRuleProcessorData *data = aData->data;

  if (!data->mTreeMatchContext.SetStyleScopeForSelectorMatching(data->mElement,
                                                                data->mScope)) {
    
    
    return;
  }

  nsRestyleHint possibleChange = RestyleHintForOp(aSelector->mOperator);

  
  
  
  NodeMatchContext nodeContext(EventStates(), false);
  if ((possibleChange & ~(aData->change)) &&
      SelectorMatches(data->mElement, aSelector, nodeContext,
                      data->mTreeMatchContext) &&
      SelectorMatchesTree(data->mElement, aSelector->mNext,
                          data->mTreeMatchContext, false)) {
    aData->change = nsRestyleHint(aData->change | possibleChange);
  }
}

static MOZ_ALWAYS_INLINE void
EnumerateSelectors(nsTArray<nsCSSSelector*>& aSelectors, AttributeEnumData* aData)
{
  nsCSSSelector **iter = aSelectors.Elements(),
                **end = iter + aSelectors.Length();
  for (; iter != end; ++iter) {
    AttributeEnumFunc(*iter, aData);
  }
}

nsRestyleHint
nsCSSRuleProcessor::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  
  

  AttributeEnumData data(aData);

  
  
  if (aData->mAttrHasChanged) {
    
    if ((aData->mAttribute == nsGkAtoms::lwtheme ||
         aData->mAttribute == nsGkAtoms::lwthemetextcolor) &&
        aData->mElement->GetNameSpaceID() == kNameSpaceID_XUL &&
        aData->mElement == aData->mElement->OwnerDoc()->GetRootElement())
      {
        data.change = nsRestyleHint(data.change | eRestyle_Subtree);
      }

    
    
    
    
    if (aData->mAttribute == nsGkAtoms::lang) {
      data.change = nsRestyleHint(data.change | eRestyle_Subtree);
    }
  }

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  if (cascade) {
    if (aData->mAttribute == nsGkAtoms::id) {
      nsIAtom* id = aData->mElement->GetID();
      if (id) {
        AtomSelectorEntry *entry =
          static_cast<AtomSelectorEntry*>
                     (PL_DHashTableOperate(&cascade->mIdSelectors,
                                           id, PL_DHASH_LOOKUP));
        if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
          EnumerateSelectors(entry->mSelectors, &data);
        }
      }

      EnumerateSelectors(cascade->mPossiblyNegatedIDSelectors, &data);
    }
    
    if (aData->mAttribute == nsGkAtoms::_class) {
      const nsAttrValue* elementClasses = aData->mElement->GetClasses();
      if (elementClasses) {
        int32_t atomCount = elementClasses->GetAtomCount();
        for (int32_t i = 0; i < atomCount; ++i) {
          nsIAtom* curClass = elementClasses->AtomAt(i);
          AtomSelectorEntry *entry =
            static_cast<AtomSelectorEntry*>
                       (PL_DHashTableOperate(&cascade->mClassSelectors,
                                             curClass, PL_DHASH_LOOKUP));
          if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
            EnumerateSelectors(entry->mSelectors, &data);
          }
        }
      }

      EnumerateSelectors(cascade->mPossiblyNegatedClassSelectors, &data);
    }

    AtomSelectorEntry *entry =
      static_cast<AtomSelectorEntry*>
                 (PL_DHashTableOperate(&cascade->mAttributeSelectors,
                                       aData->mAttribute, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      EnumerateSelectors(entry->mSelectors, &data);
    }
  }

  return data.change;
}

 bool
nsCSSRuleProcessor::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  RuleCascadeData *old = mRuleCascades;
  
  
  
  
  
  
  if (old) {
    RefreshRuleCascade(aPresContext);
  }
  return (old != mRuleCascades);
}

 size_t
nsCSSRuleProcessor::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  n += mSheets.SizeOfExcludingThis(aMallocSizeOf);
  for (RuleCascadeData* cascade = mRuleCascades; cascade;
       cascade = cascade->mNext) {
    n += cascade->SizeOfIncludingThis(aMallocSizeOf);
  }

  return n;
}

 size_t
nsCSSRuleProcessor::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}



bool
nsCSSRuleProcessor::AppendFontFaceRules(
                              nsPresContext *aPresContext,
                              nsTArray<nsFontFaceRuleContainer>& aArray)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    if (!aArray.AppendElements(cascade->mFontFaceRules))
      return false;
  }
  
  return true;
}

nsCSSKeyframesRule*
nsCSSRuleProcessor::KeyframesRuleForName(nsPresContext* aPresContext,
                                         const nsString& aName)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    return cascade->mKeyframesRuleTable.Get(aName);
  }

  return nullptr;
}

nsCSSCounterStyleRule*
nsCSSRuleProcessor::CounterStyleRuleForName(nsPresContext* aPresContext,
                                            const nsAString& aName)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    return cascade->mCounterStyleRuleTable.Get(aName);
  }

  return nullptr;
}



bool
nsCSSRuleProcessor::AppendPageRules(
                              nsPresContext* aPresContext,
                              nsTArray<nsCSSPageRule*>& aArray)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    if (!aArray.AppendElements(cascade->mPageRules)) {
      return false;
    }
  }
  
  return true;
}

bool
nsCSSRuleProcessor::AppendFontFeatureValuesRules(
                              nsPresContext *aPresContext,
                              nsTArray<nsCSSFontFeatureValuesRule*>& aArray)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    if (!aArray.AppendElements(cascade->mFontFeatureValuesRules))
      return false;
  }

  return true;
}

nsresult
nsCSSRuleProcessor::ClearRuleCascades()
{
  
  
  
  
  RuleCascadeData *data = mRuleCascades;
  mRuleCascades = nullptr;
  while (data) {
    RuleCascadeData *next = data->mNext;
    delete data;
    data = next;
  }
  return NS_OK;
}





inline
EventStates ComputeSelectorStateDependence(nsCSSSelector& aSelector)
{
  EventStates states;
  for (nsPseudoClassList* pseudoClass = aSelector.mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    
    
    if (pseudoClass->mType >= nsCSSPseudoClasses::ePseudoClass_Count) {
      continue;
    }
    states |= sPseudoClassStateDependences[pseudoClass->mType];
  }
  return states;
}

static bool
AddSelector(RuleCascadeData* aCascade,
            
            nsCSSSelector* aSelectorInTopLevel,
            
            nsCSSSelector* aSelectorPart)
{
  
  
  
  
  
  
  
  
  for (nsCSSSelector* negation = aSelectorPart; negation;
       negation = negation->mNegations) {
    
    for (nsPseudoClassList* pseudoClass = negation->mPseudoClassList;
         pseudoClass; pseudoClass = pseudoClass->mNext) {
      switch (pseudoClass->mType) {
        case nsCSSPseudoClasses::ePseudoClass_mozLocaleDir: {
          aCascade->mSelectorDocumentStates |= NS_DOCUMENT_STATE_RTL_LOCALE;
          break;
        }
        case nsCSSPseudoClasses::ePseudoClass_mozWindowInactive: {
          aCascade->mSelectorDocumentStates |= NS_DOCUMENT_STATE_WINDOW_INACTIVE;
          break;
        }
        case nsCSSPseudoClasses::ePseudoClass_mozTableBorderNonzero: {
          nsTArray<nsCSSSelector*> *array =
            aCascade->AttributeListFor(nsGkAtoms::border);
          if (!array) {
            return false;
          }
          array->AppendElement(aSelectorInTopLevel);
          break;
        }
        default: {
          break;
        }
      }
    }

    
    EventStates dependentStates = ComputeSelectorStateDependence(*negation);
    if (!dependentStates.IsEmpty()) {
      aCascade->mStateSelectors.AppendElement(
        nsCSSRuleProcessor::StateSelector(dependentStates,
                                          aSelectorInTopLevel));
    }

    
    if (negation == aSelectorInTopLevel) {
      for (nsAtomList* curID = negation->mIDList; curID;
           curID = curID->mNext) {
        AtomSelectorEntry *entry =
          static_cast<AtomSelectorEntry*>(PL_DHashTableOperate(&aCascade->mIdSelectors,
                                                               curID->mAtom,
                                                               PL_DHASH_ADD));
        if (entry) {
          entry->mSelectors.AppendElement(aSelectorInTopLevel);
        }
      }
    } else if (negation->mIDList) {
      aCascade->mPossiblyNegatedIDSelectors.AppendElement(aSelectorInTopLevel);
    }

    
    if (negation == aSelectorInTopLevel) {
      for (nsAtomList* curClass = negation->mClassList; curClass;
           curClass = curClass->mNext) {
        AtomSelectorEntry *entry =
          static_cast<AtomSelectorEntry*>(PL_DHashTableOperate(&aCascade->mClassSelectors,
                                                               curClass->mAtom,
                                                               PL_DHASH_ADD));
        if (entry) {
          entry->mSelectors.AppendElement(aSelectorInTopLevel);
        }
      }
    } else if (negation->mClassList) {
      aCascade->mPossiblyNegatedClassSelectors.AppendElement(aSelectorInTopLevel);
    }

    
    for (nsAttrSelector *attr = negation->mAttrList; attr;
         attr = attr->mNext) {
      nsTArray<nsCSSSelector*> *array =
        aCascade->AttributeListFor(attr->mCasedAttr);
      if (!array) {
        return false;
      }
      array->AppendElement(aSelectorInTopLevel);
      if (attr->mLowercaseAttr != attr->mCasedAttr) {
        array = aCascade->AttributeListFor(attr->mLowercaseAttr);
        if (!array) {
          return false;
        }
        array->AppendElement(aSelectorInTopLevel);
      }
    }

    
    for (nsPseudoClassList* pseudoClass = negation->mPseudoClassList;
         pseudoClass; pseudoClass = pseudoClass->mNext) {
      if (pseudoClass->mType == nsCSSPseudoClasses::ePseudoClass_any) {
        for (nsCSSSelectorList *l = pseudoClass->u.mSelectors; l; l = l->mNext) {
          nsCSSSelector *s = l->mSelectors;
          if (!AddSelector(aCascade, aSelectorInTopLevel, s)) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

static bool
AddRule(RuleSelectorPair* aRuleInfo, RuleCascadeData* aCascade)
{
  RuleCascadeData * const cascade = aCascade;

  
  nsCSSPseudoElements::Type pseudoType = aRuleInfo->mSelector->PseudoType();
  if (MOZ_LIKELY(pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement)) {
    cascade->mRuleHash.AppendRule(*aRuleInfo);
  } else if (pseudoType < nsCSSPseudoElements::ePseudo_PseudoElementCount) {
    RuleHash*& ruleHash = cascade->mPseudoElementRuleHashes[pseudoType];
    if (!ruleHash) {
      ruleHash = new RuleHash(cascade->mQuirksMode);
      if (!ruleHash) {
        
        return false;
      }
    }
    NS_ASSERTION(aRuleInfo->mSelector->mNext,
                 "Must have mNext; parser screwed up");
    NS_ASSERTION(aRuleInfo->mSelector->mNext->mOperator == ':',
                 "Unexpected mNext combinator");
    ruleHash->AppendRule(*aRuleInfo);
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

    
    
    AppendRuleToTagTable(&cascade->mAnonBoxRules,
                         aRuleInfo->mSelector->mLowercaseTag,
                         RuleValue(*aRuleInfo, 0, aCascade->mQuirksMode));
  } else {
#ifdef MOZ_XUL
    NS_ASSERTION(pseudoType == nsCSSPseudoElements::ePseudo_XULTree,
                 "Unexpected pseudo type");
    
    
    AppendRuleToTagTable(&cascade->mXULTreeRules,
                         aRuleInfo->mSelector->mLowercaseTag,
                         RuleValue(*aRuleInfo, 0, aCascade->mQuirksMode));
#else
    NS_NOTREACHED("Unexpected pseudo type");
#endif
  }

  for (nsCSSSelector* selector = aRuleInfo->mSelector;
           selector; selector = selector->mNext) {
    if (selector->IsPseudoElement()) {
      nsCSSPseudoElements::Type pseudo = selector->PseudoType();
      if (pseudo >= nsCSSPseudoElements::ePseudo_PseudoElementCount ||
          !nsCSSPseudoElements::PseudoElementSupportsUserActionState(pseudo)) {
        NS_ASSERTION(!selector->mNegations, "Shouldn't have negations");
        
        
        
        
        
        
        
        continue;
      }
    }
    if (!AddSelector(cascade, selector, selector)) {
      return false;
    }
  }

  return true;
}

struct PerWeightDataListItem : public RuleSelectorPair {
  PerWeightDataListItem(css::StyleRule* aRule, nsCSSSelector* aSelector)
    : RuleSelectorPair(aRule, aSelector)
    , mNext(nullptr)
  {}
  


  
  void *operator new(size_t aSize, PLArenaPool &aArena) CPP_THROW_NEW {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &aArena, aSize);
    return mem;
  }

  PerWeightDataListItem *mNext;
};

struct PerWeightData {
  PerWeightData()
    : mRuleSelectorPairs(nullptr)
    , mTail(&mRuleSelectorPairs)
  {}

  int32_t mWeight;
  PerWeightDataListItem *mRuleSelectorPairs;
  PerWeightDataListItem **mTail;
};

struct RuleByWeightEntry : public PLDHashEntryHdr {
  PerWeightData data; 
};

static PLDHashNumber
HashIntKey(PLDHashTable *table, const void *key)
{
  return PLDHashNumber(NS_PTR_TO_INT32(key));
}

static bool
MatchWeightEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                 const void *key)
{
  const RuleByWeightEntry *entry = (const RuleByWeightEntry *)hdr;
  return entry->data.mWeight == NS_PTR_TO_INT32(key);
}

static bool
InitWeightEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                const void *key)
{
  RuleByWeightEntry* entry = static_cast<RuleByWeightEntry*>(hdr);
  new (entry) RuleByWeightEntry();
  return true;
}

static const PLDHashTableOps gRulesByWeightOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIntKey,
    MatchWeightEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    InitWeightEntry
};

struct CascadeEnumData {
  CascadeEnumData(nsPresContext* aPresContext,
                  nsTArray<nsFontFaceRuleContainer>& aFontFaceRules,
                  nsTArray<nsCSSKeyframesRule*>& aKeyframesRules,
                  nsTArray<nsCSSFontFeatureValuesRule*>& aFontFeatureValuesRules,
                  nsTArray<nsCSSPageRule*>& aPageRules,
                  nsTArray<nsCSSCounterStyleRule*>& aCounterStyleRules,
                  nsMediaQueryResultCacheKey& aKey,
                  uint8_t aSheetType)
    : mPresContext(aPresContext),
      mFontFaceRules(aFontFaceRules),
      mKeyframesRules(aKeyframesRules),
      mFontFeatureValuesRules(aFontFeatureValuesRules),
      mPageRules(aPageRules),
      mCounterStyleRules(aCounterStyleRules),
      mCacheKey(aKey),
      mSheetType(aSheetType)
  {
    if (!PL_DHashTableInit(&mRulesByWeight, &gRulesByWeightOps, nullptr,
                           sizeof(RuleByWeightEntry), fallible_t(), 32))
      mRulesByWeight.ops = nullptr;

    
    PL_INIT_ARENA_POOL(&mArena, "CascadeEnumDataArena",
                       NS_CASCADEENUMDATA_ARENA_BLOCK_SIZE);
  }

  ~CascadeEnumData()
  {
    if (mRulesByWeight.ops)
      PL_DHashTableFinish(&mRulesByWeight);
    PL_FinishArenaPool(&mArena);
  }

  nsPresContext* mPresContext;
  nsTArray<nsFontFaceRuleContainer>& mFontFaceRules;
  nsTArray<nsCSSKeyframesRule*>& mKeyframesRules;
  nsTArray<nsCSSFontFeatureValuesRule*>& mFontFeatureValuesRules;
  nsTArray<nsCSSPageRule*>& mPageRules;
  nsTArray<nsCSSCounterStyleRule*>& mCounterStyleRules;
  nsMediaQueryResultCacheKey& mCacheKey;
  PLArenaPool mArena;
  
  
  PLDHashTable mRulesByWeight; 
  uint8_t mSheetType;
};














static bool
CascadeRuleEnumFunc(css::Rule* aRule, void* aData)
{
  CascadeEnumData* data = (CascadeEnumData*)aData;
  int32_t type = aRule->GetType();

  if (css::Rule::STYLE_RULE == type) {
    css::StyleRule* styleRule = static_cast<css::StyleRule*>(aRule);

    for (nsCSSSelectorList *sel = styleRule->Selector();
         sel; sel = sel->mNext) {
      int32_t weight = sel->mWeight;
      RuleByWeightEntry *entry = static_cast<RuleByWeightEntry*>(
        PL_DHashTableOperate(&data->mRulesByWeight, NS_INT32_TO_PTR(weight),
                             PL_DHASH_ADD));
      if (!entry)
        return false;
      entry->data.mWeight = weight;
      
      
      PerWeightDataListItem *newItem =
        new (data->mArena) PerWeightDataListItem(styleRule, sel->mSelectors);
      if (newItem) {
        *(entry->data.mTail) = newItem;
        entry->data.mTail = &newItem->mNext;
      }
    }
  }
  else if (css::Rule::MEDIA_RULE == type ||
           css::Rule::DOCUMENT_RULE == type ||
           css::Rule::SUPPORTS_RULE == type) {
    css::GroupRule* groupRule = static_cast<css::GroupRule*>(aRule);
    if (groupRule->UseForPresentation(data->mPresContext, data->mCacheKey))
      if (!groupRule->EnumerateRulesForwards(CascadeRuleEnumFunc, aData))
        return false;
  }
  else if (css::Rule::FONT_FACE_RULE == type) {
    nsCSSFontFaceRule *fontFaceRule = static_cast<nsCSSFontFaceRule*>(aRule);
    nsFontFaceRuleContainer *ptr = data->mFontFaceRules.AppendElement();
    if (!ptr)
      return false;
    ptr->mRule = fontFaceRule;
    ptr->mSheetType = data->mSheetType;
  }
  else if (css::Rule::KEYFRAMES_RULE == type) {
    nsCSSKeyframesRule *keyframesRule =
      static_cast<nsCSSKeyframesRule*>(aRule);
    if (!data->mKeyframesRules.AppendElement(keyframesRule)) {
      return false;
    }
  }
  else if (css::Rule::FONT_FEATURE_VALUES_RULE == type) {
    nsCSSFontFeatureValuesRule *fontFeatureValuesRule =
      static_cast<nsCSSFontFeatureValuesRule*>(aRule);
    if (!data->mFontFeatureValuesRules.AppendElement(fontFeatureValuesRule)) {
      return false;
    }
  }
  else if (css::Rule::PAGE_RULE == type) {
    nsCSSPageRule* pageRule = static_cast<nsCSSPageRule*>(aRule);
    if (!data->mPageRules.AppendElement(pageRule)) {
      return false;
    }
  }
  else if (css::Rule::COUNTER_STYLE_RULE == type) {
    nsCSSCounterStyleRule* counterStyleRule =
      static_cast<nsCSSCounterStyleRule*>(aRule);
    if (!data->mCounterStyleRules.AppendElement(counterStyleRule)) {
      return false;
    }
  }
  return true;
}

 bool
nsCSSRuleProcessor::CascadeSheet(CSSStyleSheet* aSheet, CascadeEnumData* aData)
{
  if (aSheet->IsApplicable() &&
      aSheet->UseForPresentation(aData->mPresContext, aData->mCacheKey) &&
      aSheet->mInner) {
    CSSStyleSheet* child = aSheet->mInner->mFirstChild;
    while (child) {
      CascadeSheet(child, aData);
      child = child->mNext;
    }

    if (!aSheet->mInner->mOrderedRules.EnumerateForwards(CascadeRuleEnumFunc,
                                                         aData))
      return false;
  }
  return true;
}

static int CompareWeightData(const void* aArg1, const void* aArg2,
                             void* closure)
{
  const PerWeightData* arg1 = static_cast<const PerWeightData*>(aArg1);
  const PerWeightData* arg2 = static_cast<const PerWeightData*>(aArg2);
  return arg1->mWeight - arg2->mWeight; 
}


struct FillWeightArrayData {
  explicit FillWeightArrayData(PerWeightData* aArrayData) :
    mIndex(0),
    mWeightArray(aArrayData)
  {
  }
  int32_t mIndex;
  PerWeightData* mWeightArray;
};


static PLDHashOperator
FillWeightArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                uint32_t number, void *arg)
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

  if (mSheets.Length() != 0) {
    nsAutoPtr<RuleCascadeData> newCascade(
      new RuleCascadeData(aPresContext->Medium(),
                          eCompatibility_NavQuirks == aPresContext->CompatibilityMode()));
    if (newCascade) {
      CascadeEnumData data(aPresContext, newCascade->mFontFaceRules,
                           newCascade->mKeyframesRules,
                           newCascade->mFontFeatureValuesRules,
                           newCascade->mPageRules,
                           newCascade->mCounterStyleRules,
                           newCascade->mCacheKey,
                           mSheetType);
      if (!data.mRulesByWeight.ops)
        return; 

      for (uint32_t i = 0; i < mSheets.Length(); ++i) {
        if (!CascadeSheet(mSheets.ElementAt(i), &data))
          return; 
      }

      
      uint32_t weightCount = data.mRulesByWeight.EntryCount();
      nsAutoArrayPtr<PerWeightData> weightArray(new PerWeightData[weightCount]);
      FillWeightArrayData fwData(weightArray);
      PL_DHashTableEnumerate(&data.mRulesByWeight, FillWeightArray, &fwData);
      NS_QuickSort(weightArray, weightCount, sizeof(PerWeightData),
                   CompareWeightData, nullptr);

      
      
      for (uint32_t i = 0; i < weightCount; ++i) {
        
        
        for (PerWeightDataListItem *cur = weightArray[i].mRuleSelectorPairs;
             cur;
             cur = cur->mNext) {
          if (!AddRule(cur, newCascade))
            return; 
        }
      }

      
      for (nsTArray<nsCSSKeyframesRule*>::size_type i = 0,
             iEnd = newCascade->mKeyframesRules.Length(); i < iEnd; ++i) {
        nsCSSKeyframesRule* rule = newCascade->mKeyframesRules[i];
        newCascade->mKeyframesRuleTable.Put(rule->GetName(), rule);
      }

      
      for (nsTArray<nsCSSCounterStyleRule*>::size_type i = 0,
           iEnd = newCascade->mCounterStyleRules.Length(); i < iEnd; ++i) {
        nsCSSCounterStyleRule* rule = newCascade->mCounterStyleRules[i];
        newCascade->mCounterStyleRuleTable.Put(rule->GetName(), rule);
      }

      
      newCascade->mNext = mRuleCascades;
      mRuleCascades = newCascade.forget();
    }
  }
  return;
}

 bool
nsCSSRuleProcessor::SelectorListMatches(Element* aElement,
                                        TreeMatchContext& aTreeMatchContext,
                                        nsCSSSelectorList* aSelectorList)
{
  MOZ_ASSERT(!aTreeMatchContext.mForScopedStyle,
             "mCurrentStyleScope will need to be saved and restored after the "
             "SelectorMatchesTree call");

  while (aSelectorList) {
    nsCSSSelector* sel = aSelectorList->mSelectors;
    NS_ASSERTION(sel, "Should have *some* selectors");
    NS_ASSERTION(!sel->IsPseudoElement(), "Shouldn't have been called");
    NodeMatchContext nodeContext(EventStates(), false);
    if (SelectorMatches(aElement, sel, nodeContext, aTreeMatchContext)) {
      nsCSSSelector* next = sel->mNext;
      if (!next ||
          SelectorMatchesTree(aElement, next, aTreeMatchContext, false)) {
        return true;
      }
    }

    aSelectorList = aSelectorList->mNext;
  }

  return false;
}


void
TreeMatchContext::InitAncestors(Element *aElement)
{
  MOZ_ASSERT(!mAncestorFilter.mFilter);
  MOZ_ASSERT(mAncestorFilter.mHashes.IsEmpty());
  MOZ_ASSERT(mStyleScopes.IsEmpty());

  mAncestorFilter.mFilter = new AncestorFilter::Filter();

  if (MOZ_LIKELY(aElement)) {
    MOZ_ASSERT(aElement->GetCurrentDoc() ||
               aElement->HasFlag(NODE_IS_IN_SHADOW_TREE),
               "aElement must be in the document or in shadow tree "
               "for the assumption that GetParentNode() is non-null "
               "on all element ancestors of aElement to be true");
    
    nsAutoTArray<Element*, 50> ancestors;
    Element* cur = aElement;
    do {
      ancestors.AppendElement(cur);
      cur = cur->GetParentElementCrossingShadowRoot();
    } while (cur);

    
    for (uint32_t i = ancestors.Length(); i-- != 0; ) {
      mAncestorFilter.PushAncestor(ancestors[i]);
      PushStyleScope(ancestors[i]);
    }
  }
}

void
TreeMatchContext::InitStyleScopes(Element* aElement)
{
  MOZ_ASSERT(mStyleScopes.IsEmpty());

  if (MOZ_LIKELY(aElement)) {
    
    nsAutoTArray<Element*, 50> ancestors;
    Element* cur = aElement;
    do {
      ancestors.AppendElement(cur);
      cur = cur->GetParentElementCrossingShadowRoot();
    } while (cur);

    
    for (uint32_t i = ancestors.Length(); i-- != 0; ) {
      PushStyleScope(ancestors[i]);
    }
  }
}

void
AncestorFilter::PushAncestor(Element *aElement)
{
  MOZ_ASSERT(mFilter);

  uint32_t oldLength = mHashes.Length();

  mPopTargets.AppendElement(oldLength);
#ifdef DEBUG
  mElements.AppendElement(aElement);
#endif
  mHashes.AppendElement(aElement->Tag()->hash());
  nsIAtom *id = aElement->GetID();
  if (id) {
    mHashes.AppendElement(id->hash());
  }
  const nsAttrValue *classes = aElement->GetClasses();
  if (classes) {
    uint32_t classCount = classes->GetAtomCount();
    for (uint32_t i = 0; i < classCount; ++i) {
      mHashes.AppendElement(classes->AtomAt(i)->hash());
    }
  }

  uint32_t newLength = mHashes.Length();
  for (uint32_t i = oldLength; i < newLength; ++i) {
    mFilter->add(mHashes[i]);
  }
}

void
AncestorFilter::PopAncestor()
{
  MOZ_ASSERT(!mPopTargets.IsEmpty());
  MOZ_ASSERT(mPopTargets.Length() == mElements.Length());

  uint32_t popTargetLength = mPopTargets.Length();
  uint32_t newLength = mPopTargets[popTargetLength-1];

  mPopTargets.TruncateLength(popTargetLength-1);
#ifdef DEBUG
  mElements.TruncateLength(popTargetLength-1);
#endif

  uint32_t oldLength = mHashes.Length();
  for (uint32_t i = newLength; i < oldLength; ++i) {
    mFilter->remove(mHashes[i]);
  }
  mHashes.TruncateLength(newLength);
}

#ifdef DEBUG
void
AncestorFilter::AssertHasAllAncestors(Element *aElement) const
{
  Element* cur = aElement->GetParentElementCrossingShadowRoot();
  while (cur) {
    MOZ_ASSERT(mElements.Contains(cur));
    cur = cur->GetParentElementCrossingShadowRoot();
  }
}

void
TreeMatchContext::AssertHasAllStyleScopes(Element* aElement) const
{
  Element* cur = aElement->GetParentElementCrossingShadowRoot();
  while (cur) {
    if (cur->IsScopedStyleRoot()) {
      MOZ_ASSERT(mStyleScopes.Contains(cur));
    }
    cur = cur->GetParentElementCrossingShadowRoot();
  }
}
#endif
