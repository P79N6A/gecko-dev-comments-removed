














































#include "mozilla/Util.h"

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
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/GroupRule.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsEventStateManager.h"
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
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "nsIMediaList.h"
#include "nsCSSRules.h"
#include "nsIPrincipal.h"
#include "nsStyleSet.h"
#include "prlog.h"
#include "nsIObserverService.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"
#include "mozilla/Services.h"
#include "mozilla/dom/Element.h"
#include "nsGenericElement.h"
#include "nsNthIndexCache.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"

using namespace mozilla;
using namespace mozilla::dom;

#define VISITED_PSEUDO_PREF "layout.css.visited_links_enabled"

static bool gSupportVisitedPseudo = true;

static nsTArray< nsCOMPtr<nsIAtom> >* sSystemMetrics = 0;

#ifdef XP_WIN
PRUint8 nsCSSRuleProcessor::sWinThemeId = LookAndFeel::eWindowsTheme_Generic;
#endif





struct RuleSelectorPair {
  RuleSelectorPair(css::StyleRule* aRule, nsCSSSelector* aSelector)
    : mRule(aRule), mSelector(aSelector) {}

  css::StyleRule*   mRule;
  nsCSSSelector*    mSelector; 
};






struct RuleValue : RuleSelectorPair {
  RuleValue(const RuleSelectorPair& aRuleSelectorPair, PRInt32 aIndex) :
    RuleSelectorPair(aRuleSelectorPair),
    mIndex(aIndex)
  {}
  PRInt32 mIndex; 
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
  PLDHashTableOps ops;
  
  
  
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
  return entry->mRules[0].mSelector->mClassList->mAtom;
}

static nsIAtom*
RuleHash_IdTable_GetKey(PLDHashTable *table, const PLDHashEntryHdr *hdr)
{
  const RuleHashTableEntry *entry =
    static_cast<const RuleHashTableEntry*>(hdr);
  return entry->mRules[0].mSelector->mIDList->mAtom;
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

  return NS_PTR_TO_INT32(key) ==
         entry->mRules[0].mSelector->mNameSpace;
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
  RuleHash(bool aQuirksMode);
  ~RuleHash();
  void AppendRule(const RuleSelectorPair &aRuleInfo);
  void EnumerateAllRules(const Element* aElement, RuleProcessorData* aData,
                         NodeMatchContext& aNodeMatchContext);
  PLArenaPool& Arena() { return mArena; }

  PRInt64 SizeOf() const;

protected:
  typedef nsTArray<RuleValue> RuleValueList;
  void AppendRuleToTable(PLDHashTable* aTable, const void* aKey,
                         const RuleSelectorPair& aRuleInfo);
  void AppendUniversalRule(const RuleSelectorPair& aRuleInfo);

  PRInt32     mRuleCount;
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
  PRInt32   mEnumListSize;

  inline EnumData ToEnumData(const RuleValueList& arr) {
    EnumData data = { arr.Elements(), arr.Elements() + arr.Length() };
    return data;
  }

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

RuleHash::RuleHash(bool aQuirksMode)
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
    if (mUniversalRules.Length() > 0) {
      printf("  Universal rules:\n");
      for (PRUint32 i = 0; i < mUniversalRules.Length(); ++i) {
        RuleValue* value = &(mUniversalRules[i]);
        nsAutoString selectorText;
        PRUint32 lineNumber = value->mRule->GetLineNumber();
        nsCOMPtr<nsIStyleSheet> sheet;
        value->mRule->GetStyleSheet(*getter_AddRefs(sheet));
        nsRefPtr<nsCSSStyleSheet> cssSheet = do_QueryObject(sheet);
        value->mSelector->ToString(selectorText, cssSheet);

        printf("    line %d, %s\n",
               lineNumber, NS_ConvertUTF16toUTF8(selectorText).get());
      }
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

void RuleHash::AppendRuleToTable(PLDHashTable* aTable, const void* aKey,
                                 const RuleSelectorPair& aRuleInfo)
{
  
  RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                         (PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
  if (!entry)
    return;
  entry->mRules.AppendElement(RuleValue(aRuleInfo, mRuleCount++));
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
  mUniversalRules.AppendElement(RuleValue(aRuleInfo, mRuleCount++));
}

void RuleHash::AppendRule(const RuleSelectorPair& aRuleInfo)
{
  nsCSSSelector *selector = aRuleInfo.mSelector;
  if (nsnull != selector->mIDList) {
    AppendRuleToTable(&mIdTable, selector->mIDList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mIdSelectors);
  }
  else if (nsnull != selector->mClassList) {
    AppendRuleToTable(&mClassTable, selector->mClassList->mAtom, aRuleInfo);
    RULE_HASH_STAT_INCREMENT(mClassSelectors);
  }
  else if (selector->mLowercaseTag) {
    RuleValue ruleValue(aRuleInfo, mRuleCount++);
    AppendRuleToTagTable(&mTagTable, selector->mLowercaseTag, ruleValue);
    RULE_HASH_STAT_INCREMENT(mTagSelectors);
    if (selector->mCasedTag && 
        selector->mCasedTag != selector->mLowercaseTag) {
      AppendRuleToTagTable(&mTagTable, selector->mCasedTag, ruleValue);
      RULE_HASH_STAT_INCREMENT(mTagSelectors);
    }
  }
  else if (kNameSpaceID_Unknown != selector->mNameSpace) {
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
void ContentEnumFunc(css::StyleRule* aRule, const nsCSSSelector* aSelector,
                     RuleProcessorData* data, NodeMatchContext& nodeContext);

void RuleHash::EnumerateAllRules(const Element* aElement, RuleProcessorData* aData,
                                 NodeMatchContext& aNodeContext)
{
  PRInt32 nameSpace = aElement->GetNameSpaceID();
  nsIAtom* tag = aElement->Tag();
  nsIAtom* id = aElement->GetID();
  const nsAttrValue* classList = aElement->GetClasses();

  NS_ABORT_IF_FALSE(tag, "How could we not have a tag?");

  PRInt32 classCount = classList ? classList->GetAtomCount() : 0;

  
  
  PRInt32 testCount = classCount + 4;

  if (mEnumListSize < testCount) {
    delete [] mEnumList;
    mEnumListSize = NS_MAX(testCount, MIN_ENUM_LIST_SIZE);
    mEnumList = new EnumData[mEnumListSize];
  }

  PRInt32 valueCount = 0;
  RULE_HASH_STAT_INCREMENT(mElementsMatched);

  if (mUniversalRules.Length() != 0) { 
    mEnumList[valueCount++] = ToEnumData(mUniversalRules);
    RULE_HASH_STAT_INCREMENT_LIST_COUNT(mUniversalRules, mElementUniversalCalls);
  }
  
  if (kNameSpaceID_Unknown != nameSpace && mNameSpaceTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mNameSpaceTable, NS_INT32_TO_PTR(nameSpace),
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementNameSpaceCalls);
    }
  }
  if (mTagTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mTagTable, tag, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementTagCalls);
    }
  }
  if (id && mIdTable.entryCount) {
    RuleHashTableEntry *entry = static_cast<RuleHashTableEntry*>
                                           (PL_DHashTableOperate(&mIdTable, id, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      mEnumList[valueCount++] = ToEnumData(entry->mRules);
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(entry->mRules, mElementIdCalls);
    }
  }
  if (mClassTable.entryCount) {
    for (PRInt32 index = 0; index < classCount; ++index) {
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
    
    while (valueCount > 1) {
      PRInt32 valueIndex = 0;
      PRInt32 lowestRuleIndex = mEnumList[valueIndex].mCurValue->mIndex;
      for (PRInt32 index = 1; index < valueCount; ++index) {
        PRInt32 ruleIndex = mEnumList[index].mCurValue->mIndex;
        if (ruleIndex < lowestRuleIndex) {
          valueIndex = index;
          lowestRuleIndex = ruleIndex;
        }
      }
      const RuleValue *cur = mEnumList[valueIndex].mCurValue;
      ContentEnumFunc(cur->mRule, cur->mSelector, aData, aNodeContext);
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
      ContentEnumFunc(value->mRule, value->mSelector, aData, aNodeContext);
    }
  }
}

static PLDHashOperator
RuleHashTableSizeOfEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                              PRUint32 i, void* aClosure)
{
  PRInt64* n = static_cast<PRInt64*>(aClosure);
  RuleHashTableEntry* entry = static_cast<RuleHashTableEntry*>(aHdr);

  *n += entry->mRules.SizeOf();
  

  return PL_DHASH_NEXT;
}

PRInt64
RuleHash::SizeOf() const
{
  PRInt64 n = sizeof(*this);

  n += PL_DHASH_TABLE_SIZE(&mIdTable) * sizeof(RuleHashTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mIdTable),
                         RuleHashTableSizeOfEnumerator, &n);

  n += PL_DHASH_TABLE_SIZE(&mClassTable) * sizeof(RuleHashTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mClassTable),
                         RuleHashTableSizeOfEnumerator, &n);

  n += PL_DHASH_TABLE_SIZE(&mTagTable) * sizeof(RuleHashTagTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mTagTable),
                         RuleHashTableSizeOfEnumerator, &n);

  n += PL_DHASH_TABLE_SIZE(&mNameSpaceTable) * sizeof(RuleHashTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mNameSpaceTable),
                         RuleHashTableSizeOfEnumerator, &n);

  n += mUniversalRules.SizeOf();

  const PLArena* current = &mArena.first;
  while (current) {
    n += current->limit - current->base;
    current = current->next;
  }

  return n;
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
      mCacheKey(aMedium),
      mNext(nsnull),
      mQuirksMode(aQuirksMode)
  {
    
    
    PL_DHashTableInit(&mAttributeSelectors, &AtomSelector_CSOps, nsnull,
                      sizeof(AtomSelectorEntry), 16);
    PL_DHashTableInit(&mAnonBoxRules, &RuleHash_TagTable_Ops, nsnull,
                      sizeof(RuleHashTagTableEntry), 16);
    PL_DHashTableInit(&mIdSelectors,
                      aQuirksMode ? &AtomSelector_CIOps.ops :
                                    &AtomSelector_CSOps,
                      nsnull, sizeof(AtomSelectorEntry), 16);
    PL_DHashTableInit(&mClassSelectors,
                      aQuirksMode ? &AtomSelector_CIOps.ops :
                                    &AtomSelector_CSOps,
                      nsnull, sizeof(AtomSelectorEntry), 16);
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
    PL_DHashTableFinish(&mIdSelectors);
    PL_DHashTableFinish(&mClassSelectors);
#ifdef MOZ_XUL
    PL_DHashTableFinish(&mXULTreeRules);
#endif
    for (PRUint32 i = 0; i < ArrayLength(mPseudoElementRuleHashes); ++i) {
      delete mPseudoElementRuleHashes[i];
    }
  }

  PRInt64 SizeOf() const;

  RuleHash                 mRuleHash;
  RuleHash*
    mPseudoElementRuleHashes[nsCSSPseudoElements::ePseudo_PseudoElementCount];
  nsTArray<nsCSSSelector*> mStateSelectors;
  nsEventStates            mSelectorDocumentStates;
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

  
  
  nsTArray<nsCSSSelector*>* AttributeListFor(nsIAtom* aAttribute);

  nsMediaQueryResultCacheKey mCacheKey;
  RuleCascadeData*  mNext; 

  const bool mQuirksMode;
};

static PLDHashOperator
SelectorsSizeOfEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                          PRUint32 i, void* aClosure)
{
  PRInt64* n = (PRInt64*) aClosure;
  AtomSelectorEntry* entry = (AtomSelectorEntry*)aHdr;

  *n += entry->mSelectors.SizeOf();

  return PL_DHASH_NEXT;
}

PRInt64
RuleCascadeData::SizeOf() const
{
  PRInt64 n = sizeof(*this);

  n += mRuleHash.SizeOf();
  for (PRUint32 i = 0; i < ArrayLength(mPseudoElementRuleHashes); ++i) {
    if (mPseudoElementRuleHashes[i])
      n += mPseudoElementRuleHashes[i]->SizeOf();
  }

  n += mStateSelectors.SizeOf();

  n += PL_DHASH_TABLE_SIZE(&mIdSelectors) * sizeof(AtomSelectorEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mIdSelectors),
                         SelectorsSizeOfEnumerator, &n);
  n += PL_DHASH_TABLE_SIZE(&mClassSelectors) * sizeof(AtomSelectorEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mClassSelectors),
                         SelectorsSizeOfEnumerator, &n);

  n += mPossiblyNegatedClassSelectors.SizeOf();
  n += mPossiblyNegatedIDSelectors.SizeOf();

  n += PL_DHASH_TABLE_SIZE(&mAttributeSelectors) * sizeof(AtomSelectorEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mAttributeSelectors),
                         SelectorsSizeOfEnumerator, &n);

  n += PL_DHASH_TABLE_SIZE(&mAnonBoxRules) * sizeof(RuleHashTagTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mAnonBoxRules),
                         RuleHashTableSizeOfEnumerator, &n);

#ifdef MOZ_XUL
  n += PL_DHASH_TABLE_SIZE(&mXULTreeRules) * sizeof(RuleHashTagTableEntry);
  PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&mAnonBoxRules),
                         RuleHashTableSizeOfEnumerator, &n);
#endif

  n += mFontFaceRules.SizeOf();
  n += mKeyframesRules.SizeOf();

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
    return nsnull;
  return &entry->mSelectors;
}

class nsPrivateBrowsingObserver : nsIObserver,
                                  nsSupportsWeakReference
{
public:
  nsPrivateBrowsingObserver();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  void Init();
  bool InPrivateBrowsing() const { return mInPrivateBrowsing; }

private:
  bool mInPrivateBrowsing;
};

NS_IMPL_ISUPPORTS2(nsPrivateBrowsingObserver, nsIObserver, nsISupportsWeakReference)

nsPrivateBrowsingObserver::nsPrivateBrowsingObserver()
  : mInPrivateBrowsing(false)
{
}

void
nsPrivateBrowsingObserver::Init()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(this, "profile-after-change", true);
    observerService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, true);
  }
}

nsresult
nsPrivateBrowsingObserver::Observe(nsISupports *aSubject,
                                   const char *aTopic,
                                   const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    if (!nsCRT::strcmp(aData, NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).get())) {
      mInPrivateBrowsing = true;
    } else {
      mInPrivateBrowsing = false;
    }
  }
  else if (!strcmp(aTopic, "profile-after-change")) {
    nsCOMPtr<nsIPrivateBrowsingService> pbService =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbService)
      pbService->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
  }
  return NS_OK;
}

static nsPrivateBrowsingObserver *gPrivateBrowsingObserver = nsnull;





nsCSSRuleProcessor::nsCSSRuleProcessor(const sheet_array_type& aSheets,
                                       PRUint8 aSheetType)
  : mSheets(aSheets)
  , mRuleCascades(nsnull)
  , mLastPresContext(nsnull)
  , mSheetType(aSheetType)
{
  for (sheet_array_type::size_type i = mSheets.Length(); i-- != 0; ) {
    mSheets[i]->AddRuleProcessor(this);
  }
}

nsCSSRuleProcessor::~nsCSSRuleProcessor()
{
  for (sheet_array_type::size_type i = mSheets.Length(); i-- != 0; ) {
    mSheets[i]->DropRuleProcessor(this);
  }
  mSheets.Clear();
  ClearRuleCascades();
}

NS_IMPL_ISUPPORTS1(nsCSSRuleProcessor, nsIStyleRuleProcessor)

 nsresult
nsCSSRuleProcessor::Startup()
{
  Preferences::AddBoolVarCache(&gSupportVisitedPseudo, VISITED_PSEUDO_PREF,
                               true);

  gPrivateBrowsingObserver = new nsPrivateBrowsingObserver();
  NS_ENSURE_TRUE(gPrivateBrowsingObserver, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(gPrivateBrowsingObserver);
  gPrivateBrowsingObserver->Init();

  return NS_OK;
}

static bool
InitSystemMetrics()
{
  NS_ASSERTION(!sSystemMetrics, "already initialized");

  sSystemMetrics = new nsTArray< nsCOMPtr<nsIAtom> >;
  NS_ENSURE_TRUE(sSystemMetrics, false);

  




  PRInt32 metricResult =
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

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_DWMCompositor, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_compositor);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsClassic, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::windows_classic);
  }

  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_TouchEnabled, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::touch_enabled);
  }
 
  rv = LookAndFeel::GetInt(LookAndFeel::eIntID_MaemoClassic, &metricResult);
  if (NS_SUCCEEDED(rv) && metricResult) {
    sSystemMetrics->AppendElement(nsGkAtoms::maemo_classic);
  }

#ifdef XP_WIN
  if (NS_SUCCEEDED(
        LookAndFeel::GetInt(LookAndFeel::eIntID_WindowsThemeIdentifier,
                            &metricResult))) {
    nsCSSRuleProcessor::SetWindowsThemeIdentifier(static_cast<PRUint8>(metricResult));
    switch(metricResult) {
      case LookAndFeel::eWindowsTheme_Aero:
        sSystemMetrics->AppendElement(nsGkAtoms::windows_theme_aero);
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
  sSystemMetrics = nsnull;
}

 void
nsCSSRuleProcessor::Shutdown()
{
  FreeSystemMetrics();
  
  NS_IF_RELEASE(gPrivateBrowsingObserver);
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
 PRUint8
nsCSSRuleProcessor::GetWindowsThemeIdentifier()
{
  if (!sSystemMetrics)
    InitSystemMetrics();
  return sWinThemeId;
}
#endif


nsEventStates
nsCSSRuleProcessor::GetContentState(const Element* aElement)
{
  nsEventStates state = aElement->State();

  
  
  
  
  if (state.HasState(NS_EVENT_STATE_VISITED) &&
      (!gSupportVisitedPseudo ||
       aElement->OwnerDoc()->IsBeingUsedAsImage() ||
       gPrivateBrowsingObserver->InPrivateBrowsing())) {
    state &= ~NS_EVENT_STATE_VISITED;
    state |= NS_EVENT_STATE_UNVISITED;
  }
  return state;
}


bool
nsCSSRuleProcessor::IsLink(const Element* aElement)
{
  nsEventStates state = aElement->State();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED);
}


nsEventStates
nsCSSRuleProcessor::GetContentStateForVisitedHandling(
                     const Element* aElement,
                     nsRuleWalker::VisitedHandlingType aVisitedHandling,
                     bool aIsRelevantLink)
{
  nsEventStates contentState = GetContentState(aElement);
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
  
  
  
  
  
  
  
  
  
  
  
  const nsEventStates mStateMask;

  
  
  
  
  
  
  
  
  
  const bool mIsRelevantLink;

  NodeMatchContext(nsEventStates aStateMask, bool aIsRelevantLink)
    : mStateMask(aStateMask)
    , mIsRelevantLink(aIsRelevantLink)
  {
  }
};

static bool ValueIncludes(const nsSubstring& aValueList,
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
IsSignificantChild(const nsIContent* aChild, bool aTextIsSignificant,
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
edgeChildMatches(const Element* aElement, TreeMatchContext& aTreeMatchContext,
                 bool checkFirst, bool checkLast)
{
  const nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling)
    aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_EDGE_CHILD_SELECTOR);

  return (!checkFirst ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, false, false, true) == 1) &&
         (!checkLast ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, false, true, true) == 1);
}

static inline bool
nthChildGenericMatches(const Element* aElement,
                       TreeMatchContext& aTreeMatchContext,
                       const nsPseudoClassList* pseudoClass,
                       bool isOfType, bool isFromEnd)
{
  const nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling) {
    if (isFromEnd)
      aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_SLOW_SELECTOR);
    else
      aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  }

  const PRInt32 index = aTreeMatchContext.mNthIndexCache.
    GetNthIndex(aElement, isOfType, isFromEnd, false);
  if (index <= 0) {
    
    return false;
  }

  const PRInt32 a = pseudoClass->u.mNumbers[0];
  const PRInt32 b = pseudoClass->u.mNumbers[1];
  
  
  if (a == 0) {
    return b == index;
  }

  
  
  
  const PRInt32 n = (index - b) / a;
  return n >= 0 && (a * n == index - b);
}

static inline bool
edgeOfTypeMatches(const Element* aElement, TreeMatchContext& aTreeMatchContext,
                  bool checkFirst, bool checkLast)
{
  const nsIContent *parent = aElement->GetParent();
  if (!parent) {
    return false;
  }

  if (aTreeMatchContext.mForStyling) {
    if (checkLast)
      aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_SLOW_SELECTOR);
    else
      aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  }

  return (!checkFirst ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, true, false, true) == 1) &&
         (!checkLast ||
          aTreeMatchContext.mNthIndexCache.
            GetNthIndex(aElement, true, true, true) == 1);
}

static inline bool
checkGenericEmptyMatches(const Element* aElement,
                         TreeMatchContext& aTreeMatchContext,
                         bool isWhitespaceSignificant)
{
  const nsIContent *child = nsnull;
  PRInt32 index = -1;

  if (aTreeMatchContext.mForStyling)
    aTreeMatchContext.RecordFlagsToAdd(aElement, NODE_HAS_EMPTY_SELECTOR);

  do {
    child = aElement->GetChildAt(++index);
    
    
  } while (child && !IsSignificantChild(child, true, isWhitespaceSignificant));
  return (child == nsnull);
}


static const nsEventStates sPseudoClassStates[] = {
#define CSS_PSEUDO_CLASS(_name, _value)         \
  nsEventStates(),
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _states) \
  _states,
#include "nsCSSPseudoClassList.h"
#undef CSS_STATE_PSEUDO_CLASS
#undef CSS_PSEUDO_CLASS
  
  
  nsEventStates(),
  nsEventStates()
};
PR_STATIC_ASSERT(NS_ARRAY_LENGTH(sPseudoClassStates) ==
                   nsCSSPseudoClasses::ePseudoClass_NotPseudoClass + 1);






static bool SelectorMatches(const Element* aElement,
                              const nsCSSSelector* aSelector,
                              NodeMatchContext& aNodeMatchContext,
                              TreeMatchContext& aTreeMatchContext,
                              bool* const aDependence = nsnull)

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

  const nsAtomList* IDList = aSelector->mIDList;
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

  const nsAtomList* classList = aSelector->mClassList;
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

  const bool isNegated = (aDependence != nsnull);
  
  
  
  
  
  NS_ASSERTION(aNodeMatchContext.mStateMask.IsEmpty() ||
               !aTreeMatchContext.mForStyling,
               "mForStyling must be false if we're just testing for "
               "state-dependence");

  
  for (const nsPseudoClassList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    nsEventStates statesToCheck = sPseudoClassStates[pseudoClass->mType];
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
          const nsIContent *child = nsnull;
          PRInt32 index = -1;

          if (aTreeMatchContext.mForStyling)
            
            
            
            
            
            aTreeMatchContext.RecordFlagsToAdd(aElement, NODE_HAS_SLOW_SELECTOR);
          do {
            child = aElement->GetChildAt(++index);
          } while (child &&
                   (!IsSignificantChild(child, true, false) ||
                    (child->GetNameSpaceID() == aElement->GetNameSpaceID() &&
                     child->Tag()->Equals(nsDependentString(pseudoClass->u.mString)))));
          if (child != nsnull) {
            return false;
          }
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_lang:
        {
          NS_ASSERTION(nsnull != pseudoClass->u.mString, "null lang parameter");
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

          const nsIDocument* doc = aTreeMatchContext.mDocument;
          if (doc) {
            
            
            
            
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
        if (aElement->GetParent() ||
            aElement != aElement->OwnerDoc()->GetRootElement()) {
          return false;
        }
        break;

      case nsCSSPseudoClasses::ePseudoClass_any:
        {
          const nsCSSSelectorList *l;
          for (l = pseudoClass->u.mSelectors; l; l = l->mNext) {
            const nsCSSSelector *s = l->mSelectors;
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
          const nsIContent *firstNode = nsnull;
          const nsIContent *parent = aElement->GetParent();
          if (parent) {
            if (aTreeMatchContext.mForStyling)
              aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_EDGE_CHILD_SELECTOR);

            PRInt32 index = -1;
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
          const nsIContent *lastNode = nsnull;
          const nsIContent *parent = aElement->GetParent();
          if (parent) {
            if (aTreeMatchContext.mForStyling)
              aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_EDGE_CHILD_SELECTOR);
            
            PRUint32 index = parent->GetChildCount();
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

      case nsCSSPseudoClasses::ePseudoClass_mozHasHandlerRef:
        {
          const nsIContent *child = nsnull;
          PRInt32 index = -1;

          do {
            child = aElement->GetChildAt(++index);
            if (child && child->IsHTML() &&
                child->Tag() == nsGkAtoms::param &&
                child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                                   NS_LITERAL_STRING("pluginurl"),
                                   eIgnoreCase)) {
              break;
            }
          } while (child);
          if (!child) {
            return false;
          }
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
          const nsGenericElement *ge = static_cast<const nsGenericElement*>(aElement);
          const nsAttrValue *val = ge->GetParsedAttr(nsGkAtoms::border);
          if (!val ||
              (val->Type() == nsAttrValue::eInteger &&
               val->GetIntegerValue() == 0)) {
            return false;
          }
        }
        break;

      default:
        NS_ABORT_IF_FALSE(false, "How did that happen?");
      }
    } else {
      
      if (statesToCheck.HasAtLeastOneOfStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE) &&
          aTreeMatchContext.mCompatMode == eCompatibility_NavQuirks &&
          
          !aSelector->HasTagSelector() && !aSelector->mIDList && 
          !aSelector->mClassList && !aSelector->mAttrList &&
          
          
          
          
          !isNegated &&
          
          aElement->IsHTML() && !nsCSSRuleProcessor::IsLink(aElement) &&
          !IsQuirkEventSensitive(aElement->Tag())) {
        
        
        return false;
      } else {
        if (aNodeMatchContext.mStateMask.HasAtLeastOneOfStates(statesToCheck)) {
          if (aDependence)
            *aDependence = true;
        } else {
          nsEventStates contentState =
            nsCSSRuleProcessor::GetContentStateForVisitedHandling(
                                         aElement,
                                         aTreeMatchContext.VisitedHandling(),
                                         aNodeMatchContext.mIsRelevantLink);
          if (!contentState.HasAtLeastOneOfStates(statesToCheck)) {
            return false;
          }
        }
      }
    }
  }

  bool result = true;
  if (aSelector->mAttrList) {
    
    PRUint32 attrCount = aElement->GetAttrCount();
    if (attrCount == 0) {
      
      return false;
    } else {
      result = true;
      const nsAttrSelector* attr = aSelector->mAttrList;
      nsIAtom* matchAttribute;

      do {
        bool isHTML =
          (aTreeMatchContext.mIsHTMLDocument && aElement->IsHTML());
        matchAttribute = isHTML ? attr->mLowercaseAttr : attr->mCasedAttr;
        if (attr->mNameSpace == kNameSpaceID_Unknown) {
          
          
          
          
          
          
          
          result = false;
          for (PRUint32 i = 0; i < attrCount; ++i) {
            const nsAttrName* attrName = aElement->GetAttrNameAt(i);
            NS_ASSERTION(attrName, "GetAttrCount lied or GetAttrNameAt failed");
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
    for (const nsCSSSelector *negation = aSelector->mNegations;
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
  ((ch) == PRUnichar(' ') || (ch) == PRUnichar('~'))

static bool SelectorMatchesTree(const Element* aPrevElement,
                                  const nsCSSSelector* aSelector,
                                  TreeMatchContext& aTreeMatchContext,
                                  bool aLookForRelevantLink)
{
  const nsCSSSelector* selector = aSelector;
  const Element* prevElement = aPrevElement;
  while (selector) { 
    NS_ASSERTION(!selector->mNext ||
                 selector->mNext->mOperator != PRUnichar(0),
                 "compound selector without combinator");

    
    
    const Element* element = nsnull;
    if (PRUnichar('+') == selector->mOperator ||
        PRUnichar('~') == selector->mOperator) {
      
      aLookForRelevantLink = false;
      const nsIContent* parent = prevElement->GetParent();
      if (parent) {
        if (aTreeMatchContext.mForStyling)
          aTreeMatchContext.RecordFlagsToAdd(parent, NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);

        for (const nsIContent* curr = prevElement->GetPreviousSibling();
             curr;
             curr = curr->GetPreviousSibling()) {
          if (curr->IsElement()) {
            element = curr->AsElement();
            break;
          }
        }
      }
    }
    
    
    else {
      const nsIContent *content = prevElement->GetParent();
      
      
      if (content && content->IsElement()) {
        element = content->AsElement();
      }
    }
    if (!element) {
      return false;
    }
    NodeMatchContext nodeContext(nsEventStates(),
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
            (selector->mNext->mOperator == PRUnichar(' ') ||
             selector->mNext->mOperator == PRUnichar('>')))) {

        
        

        
        
        
        
        if (SelectorMatchesTree(element, selector, aTreeMatchContext,
                                aLookForRelevantLink)) {
          return true;
        }
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
void ContentEnumFunc(css::StyleRule* aRule, const nsCSSSelector* aSelector,
                     RuleProcessorData* data, NodeMatchContext& nodeContext)
{
  if (nodeContext.mIsRelevantLink) {
    data->mTreeMatchContext.SetHaveRelevantLink();
  }
  if (SelectorMatches(data->mElement, aSelector, nodeContext,
                      data->mTreeMatchContext)) {
    const nsCSSSelector *next = aSelector->mNext;
    if (!next || SelectorMatchesTree(data->mElement, next,
                                     data->mTreeMatchContext,
                                     !nodeContext.mIsRelevantLink)) {
      aRule->RuleMatched();
      data->mRuleWalker->Forward(aRule);
      
    }
  }
}

 void
nsCSSRuleProcessor::RulesMatching(ElementRuleProcessorData *aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade) {
    NodeMatchContext nodeContext(nsEventStates(),
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
      NodeMatchContext nodeContext(nsEventStates(),
                                   nsCSSRuleProcessor::IsLink(aData->mElement));
      ruleHash->EnumerateAllRules(aData->mElement, aData, nodeContext);
    }
  }
}

 void
nsCSSRuleProcessor::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  if (cascade && cascade->mAnonBoxRules.entryCount) {
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

  if (cascade && cascade->mXULTreeRules.entryCount) {
    RuleHashTagTableEntry* entry = static_cast<RuleHashTagTableEntry*>
      (PL_DHashTableOperate(&cascade->mXULTreeRules, aData->mPseudoTag,
                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      NodeMatchContext nodeContext(nsEventStates(),
                                   nsCSSRuleProcessor::IsLink(aData->mElement));
      nsTArray<RuleValue>& rules = entry->mRules;
      for (RuleValue *value = rules.Elements(), *end = value + rules.Length();
           value != end; ++value) {
        if (aData->mComparator->PseudoMatches(value->mSelector)) {
          ContentEnumFunc(value->mRule, value->mSelector->mNext, aData,
                          nodeContext);
        }
      }
    }
  }
}
#endif

static inline nsRestyleHint RestyleHintForOp(PRUnichar oper)
{
  if (oper == PRUnichar('+') || oper == PRUnichar('~')) {
    return eRestyle_LaterSiblings;
  }

  if (oper != PRUnichar(0)) {
    return eRestyle_Subtree;
  }

  return eRestyle_Self;
}

nsRestyleHint
nsCSSRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  
  
  
  
  
  
  nsRestyleHint hint = nsRestyleHint(0);
  if (cascade) {
    nsCSSSelector **iter = cascade->mStateSelectors.Elements(),
                  **end = iter + cascade->mStateSelectors.Length();
    NodeMatchContext nodeContext(aData->mStateMask, false);
    for(; iter != end; ++iter) {
      nsCSSSelector* selector = *iter;

      nsRestyleHint possibleChange = RestyleHintForOp(selector->mOperator);

      
      
      
      if ((possibleChange & ~hint) &&
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

bool
nsCSSRuleProcessor::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  return cascade && cascade->mSelectorDocumentStates.HasAtLeastOneOfStates(aData->mStateMask);
}

struct AttributeEnumData {
  AttributeEnumData(AttributeRuleProcessorData *aData)
    : data(aData), change(nsRestyleHint(0)) {}

  AttributeRuleProcessorData *data;
  nsRestyleHint change;
};


static void
AttributeEnumFunc(nsCSSSelector* aSelector, AttributeEnumData* aData)
{
  AttributeRuleProcessorData *data = aData->data;

  nsRestyleHint possibleChange = RestyleHintForOp(aSelector->mOperator);

  
  
  
  NodeMatchContext nodeContext(nsEventStates(), false);
  if ((possibleChange & ~(aData->change)) &&
      SelectorMatches(data->mElement, aSelector, nodeContext,
                      data->mTreeMatchContext) &&
      SelectorMatchesTree(data->mElement, aSelector->mNext,
                          data->mTreeMatchContext, false)) {
    aData->change = nsRestyleHint(aData->change | possibleChange);
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
    if (aData->mAttribute == aData->mElement->GetIDAttributeName()) {
      nsIAtom* id = aData->mElement->GetID();
      if (id) {
        AtomSelectorEntry *entry =
          static_cast<AtomSelectorEntry*>
                     (PL_DHashTableOperate(&cascade->mIdSelectors,
                                           id, PL_DHASH_LOOKUP));
        if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
          nsCSSSelector **iter = entry->mSelectors.Elements(),
                        **end = iter + entry->mSelectors.Length();
          for(; iter != end; ++iter) {
            AttributeEnumFunc(*iter, &data);
          }
        }
      }

      nsCSSSelector **iter = cascade->mPossiblyNegatedIDSelectors.Elements(),
                    **end = iter + cascade->mPossiblyNegatedIDSelectors.Length();
      for(; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
    }
    
    if (aData->mAttribute == aData->mElement->GetClassAttributeName()) {
      const nsAttrValue* elementClasses = aData->mElement->GetClasses();
      if (elementClasses) {
        PRInt32 atomCount = elementClasses->GetAtomCount();
        for (PRInt32 i = 0; i < atomCount; ++i) {
          nsIAtom* curClass = elementClasses->AtomAt(i);
          AtomSelectorEntry *entry =
            static_cast<AtomSelectorEntry*>
                       (PL_DHashTableOperate(&cascade->mClassSelectors,
                                             curClass, PL_DHASH_LOOKUP));
          if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
            nsCSSSelector **iter = entry->mSelectors.Elements(),
                          **end = iter + entry->mSelectors.Length();
            for(; iter != end; ++iter) {
              AttributeEnumFunc(*iter, &data);
            }
          }
        }
      }

      nsCSSSelector **iter = cascade->mPossiblyNegatedClassSelectors.Elements(),
                    **end = iter +
                              cascade->mPossiblyNegatedClassSelectors.Length();
      for (; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
    }

    AtomSelectorEntry *entry =
      static_cast<AtomSelectorEntry*>
                 (PL_DHashTableOperate(&cascade->mAttributeSelectors,
                                       aData->mAttribute, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      nsCSSSelector **iter = entry->mSelectors.Elements(),
                    **end = iter + entry->mSelectors.Length();
      for(; iter != end; ++iter) {
        AttributeEnumFunc(*iter, &data);
      }
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

 PRInt64
nsCSSRuleProcessor::SizeOf() const
{
  PRInt64 n = sizeof(*this);

  n += mSheets.SizeOf();
  for (RuleCascadeData* cascade = mRuleCascades; cascade;
       cascade = cascade->mNext) {
    n += cascade->SizeOf();
  }

  return n;
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



bool
nsCSSRuleProcessor::AppendKeyframesRules(
                              nsPresContext *aPresContext,
                              nsTArray<nsCSSKeyframesRule*>& aArray)
{
  RuleCascadeData* cascade = GetRuleCascade(aPresContext);

  if (cascade) {
    if (!aArray.AppendElements(cascade->mKeyframesRules))
      return false;
  }
  
  return true;
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
bool IsStateSelector(nsCSSSelector& aSelector)
{
  for (nsPseudoClassList* pseudoClass = aSelector.mPseudoClassList;
       pseudoClass; pseudoClass = pseudoClass->mNext) {
    
    
    if (pseudoClass->mType >= nsCSSPseudoClasses::ePseudoClass_Count) {
      continue;
    }
    if (!sPseudoClassStates[pseudoClass->mType].IsEmpty()) {
      return true;
    }
  }
  return false;
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

    
    if (IsStateSelector(*negation))
      aCascade->mStateSelectors.AppendElement(aSelectorInTopLevel);

    
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
  if (NS_LIKELY(pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement)) {
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
    NS_ASSERTION(aRuleInfo->mSelector->mNext->mOperator == '>',
                 "Unexpected mNext combinator");
    aRuleInfo->mSelector = aRuleInfo->mSelector->mNext;
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
                         RuleValue(*aRuleInfo, 0));
  } else {
#ifdef MOZ_XUL
    NS_ASSERTION(pseudoType == nsCSSPseudoElements::ePseudo_XULTree,
                 "Unexpected pseudo type");
    
    
    AppendRuleToTagTable(&cascade->mXULTreeRules,
                         aRuleInfo->mSelector->mLowercaseTag,
                         RuleValue(*aRuleInfo, 0));
#else
    NS_NOTREACHED("Unexpected pseudo type");
#endif
  }

  for (nsCSSSelector* selector = aRuleInfo->mSelector;
           selector; selector = selector->mNext) {
    if (selector->IsPseudoElement()) {
      NS_ASSERTION(!selector->mNegations, "Shouldn't have negations");
      
      
      
      continue;
    }
    if (!AddSelector(cascade, selector, selector)) {
      return false;
    }
  }

  return true;
}

struct PerWeightData {
  PRInt32 mWeight;
  nsTArray<RuleSelectorPair> mRules; 
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

static void
ClearWeightEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleByWeightEntry* entry = static_cast<RuleByWeightEntry*>(hdr);
  entry->~RuleByWeightEntry();
}

static PLDHashTableOps gRulesByWeightOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIntKey,
    MatchWeightEntry,
    PL_DHashMoveEntryStub,
    ClearWeightEntry,
    PL_DHashFinalizeStub,
    InitWeightEntry
};

struct CascadeEnumData {
  CascadeEnumData(nsPresContext* aPresContext,
                  nsTArray<nsFontFaceRuleContainer>& aFontFaceRules,
                  nsTArray<nsCSSKeyframesRule*>& aKeyframesRules,
                  nsMediaQueryResultCacheKey& aKey,
                  PLArenaPool& aArena,
                  PRUint8 aSheetType)
    : mPresContext(aPresContext),
      mFontFaceRules(aFontFaceRules),
      mKeyframesRules(aKeyframesRules),
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
  nsTArray<nsCSSKeyframesRule*>& mKeyframesRules;
  nsMediaQueryResultCacheKey& mCacheKey;
  PLArenaPool& mArena;
  
  
  PLDHashTable mRulesByWeight; 
  PRUint8 mSheetType;
};










static bool
CascadeRuleEnumFunc(css::Rule* aRule, void* aData)
{
  CascadeEnumData* data = (CascadeEnumData*)aData;
  PRInt32 type = aRule->GetType();

  if (css::Rule::STYLE_RULE == type) {
    css::StyleRule* styleRule = static_cast<css::StyleRule*>(aRule);

    for (nsCSSSelectorList *sel = styleRule->Selector();
         sel; sel = sel->mNext) {
      PRInt32 weight = sel->mWeight;
      RuleByWeightEntry *entry = static_cast<RuleByWeightEntry*>(
        PL_DHashTableOperate(&data->mRulesByWeight, NS_INT32_TO_PTR(weight),
                             PL_DHASH_ADD));
      if (!entry)
        return false;
      entry->data.mWeight = weight;
      
      entry->data.mRules.AppendElement(RuleSelectorPair(styleRule,
                                                        sel->mSelectors));
    }
  }
  else if (css::Rule::MEDIA_RULE == type ||
           css::Rule::DOCUMENT_RULE == type) {
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

  return true;
}

 bool
nsCSSRuleProcessor::CascadeSheet(nsCSSStyleSheet* aSheet, CascadeEnumData* aData)
{
  if (aSheet->IsApplicable() &&
      aSheet->UseForPresentation(aData->mPresContext, aData->mCacheKey) &&
      aSheet->mInner) {
    nsCSSStyleSheet* child = aSheet->mInner->mFirstChild;
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

  if (mSheets.Length() != 0) {
    nsAutoPtr<RuleCascadeData> newCascade(
      new RuleCascadeData(aPresContext->Medium(),
                          eCompatibility_NavQuirks == aPresContext->CompatibilityMode()));
    if (newCascade) {
      CascadeEnumData data(aPresContext, newCascade->mFontFaceRules,
                           newCascade->mKeyframesRules,
                           newCascade->mCacheKey,
                           newCascade->mRuleHash.Arena(),
                           mSheetType);
      if (!data.mRulesByWeight.ops)
        return; 

      for (PRUint32 i = 0; i < mSheets.Length(); ++i) {
        if (!CascadeSheet(mSheets.ElementAt(i), &data))
          return; 
      }

      
      PRUint32 weightCount = data.mRulesByWeight.entryCount;
      nsAutoArrayPtr<PerWeightData> weightArray(new PerWeightData[weightCount]);
      FillWeightArrayData fwData(weightArray);
      PL_DHashTableEnumerate(&data.mRulesByWeight, FillWeightArray, &fwData);
      NS_QuickSort(weightArray, weightCount, sizeof(PerWeightData),
                   CompareWeightData, nsnull);

      
      
      for (PRUint32 i = 0; i < weightCount; ++i) {
        
        
        nsTArray<RuleSelectorPair>& arr = weightArray[i].mRules;
        for (RuleSelectorPair *cur = arr.Elements(),
                              *end = cur + arr.Length();
             cur != end; ++cur) {
          if (!AddRule(cur, newCascade))
            return; 
        }
      }

      
      newCascade->mNext = mRuleCascades;
      mRuleCascades = newCascade.forget();
    }
  }
  return;
}

 bool
nsCSSRuleProcessor::SelectorListMatches(const Element* aElement,
                                        TreeMatchContext& aTreeMatchContext,
                                        const nsCSSSelectorList* aSelectorList)
{
  while (aSelectorList) {
    const nsCSSSelector* sel = aSelectorList->mSelectors;
    NS_ASSERTION(sel, "Should have *some* selectors");
    NS_ASSERTION(!sel->IsPseudoElement(), "Shouldn't have been called");
    NodeMatchContext nodeContext(nsEventStates(), false);
    if (SelectorMatches(aElement, sel, nodeContext, aTreeMatchContext)) {
      const nsCSSSelector* next = sel->mNext;
      if (!next ||
          SelectorMatchesTree(aElement, next, aTreeMatchContext, false)) {
        return true;
      }
    }

    aSelectorList = aSelectorList->mNext;
  }

  return false;
}
