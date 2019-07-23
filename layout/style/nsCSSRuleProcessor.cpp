












































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
  nsIAtom *atom = NS_CONST_CAST(nsIAtom*, NS_STATIC_CAST(const nsIAtom*, key));

  nsAutoString str;
  atom->ToString(str);
  ToUpperCase(str);
  return HashString(str);
}

PR_STATIC_CALLBACK(PRBool)
RuleHash_CIMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                      const void *key)
{
  nsIAtom *match_atom = NS_CONST_CAST(nsIAtom*, NS_STATIC_CAST(const nsIAtom*,
                            key));
  
  
  nsIAtom *entry_atom = NS_CONST_CAST(nsIAtom*, NS_STATIC_CAST(const nsIAtom*,
             table->ops->getKey(table, NS_CONST_CAST(PLDHashEntryHdr*, hdr))));

  
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
  nsIAtom *match_atom = NS_CONST_CAST(nsIAtom*, NS_STATIC_CAST(const nsIAtom*,
                            key));
  
  
  nsIAtom *entry_atom = NS_CONST_CAST(nsIAtom*, NS_STATIC_CAST(const nsIAtom*,
             table->ops->getKey(table, NS_CONST_CAST(PLDHashEntryHdr*, hdr))));

  return match_atom == entry_atom;
}

PR_STATIC_CALLBACK(const void*)
RuleHash_TagTable_GetKey(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*, hdr);
  return entry->mRules->mSelector->mTag;
}

PR_STATIC_CALLBACK(const void*)
RuleHash_ClassTable_GetKey(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*, hdr);
  return entry->mRules->mSelector->mClassList->mAtom;
}

PR_STATIC_CALLBACK(const void*)
RuleHash_IdTable_GetKey(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*, hdr);
  return entry->mRules->mSelector->mIDList->mAtom;
}

PR_STATIC_CALLBACK(const void*)
RuleHash_NameSpaceTable_GetKey(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*, hdr);
  return NS_INT32_TO_PTR(entry->mRules->mSelector->mNameSpace);
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
    NS_STATIC_CAST(const RuleHashTableEntry*, hdr);

  return NS_PTR_TO_INT32(key) ==
         entry->mRules->mSelector->mNameSpace;
}

static PLDHashTableOps RuleHash_TagTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_TagTable_GetKey,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};


static PLDHashTableOps RuleHash_ClassTable_CSOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_ClassTable_GetKey,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};


static PLDHashTableOps RuleHash_ClassTable_CIOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_ClassTable_GetKey,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};


static PLDHashTableOps RuleHash_IdTable_CSOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_IdTable_GetKey,
  PL_DHashVoidPtrKeyStub,
  RuleHash_CSMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};


static PLDHashTableOps RuleHash_IdTable_CIOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_IdTable_GetKey,
  RuleHash_CIHashKey,
  RuleHash_CIMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};

static PLDHashTableOps RuleHash_NameSpaceTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  RuleHash_NameSpaceTable_GetKey,
  RuleHash_NameSpaceTable_HashKey,
  RuleHash_NameSpaceTable_MatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
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

  PL_DHashTableInit(&mTagTable, &RuleHash_TagTable_Ops, nsnull,
                    sizeof(RuleHashTableEntry), 64);
  PL_DHashTableInit(&mIdTable,
                    aQuirksMode ? &RuleHash_IdTable_CIOps
                                : &RuleHash_IdTable_CSOps,
                    nsnull, sizeof(RuleHashTableEntry), 16);
  PL_DHashTableInit(&mClassTable,
                    aQuirksMode ? &RuleHash_ClassTable_CIOps
                                : &RuleHash_ClassTable_CSOps,
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
         NS_STATIC_CAST(void*, this),
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
  
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
      PL_DHashTableOperate(aTable, aKey, PL_DHASH_ADD));
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
    RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
        PL_DHashTableOperate(&mNameSpaceTable, NS_INT32_TO_PTR(aNameSpace),
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementNameSpaceCalls);
    }
  }
  if (nsnull != aTag) {
    RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
        PL_DHashTableOperate(&mTagTable, aTag, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementTagCalls);
    }
  }
  if (nsnull != aID) {
    RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
        PL_DHashTableOperate(&mIdTable, aID, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      RuleValue *value = entry->mRules;
      mEnumList[valueCount++] = value;
      RULE_HASH_STAT_INCREMENT_LIST_COUNT(value, mElementIdCalls);
    }
  }
  { 
    for (PRInt32 index = 0; index < classCount; ++index) {
      RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
        PL_DHashTableOperate(&mClassTable, aClassList->AtomAt(index),
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
  RuleHashTableEntry *entry = NS_STATIC_CAST(RuleHashTableEntry*,
      PL_DHashTableOperate(&mTagTable, aTag, PL_DHASH_LOOKUP));

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
  AttributeSelectorEntry *entry = NS_STATIC_CAST(AttributeSelectorEntry*, hdr);
  delete entry->mSelectors;
  memset(entry, 0, table->entrySize);
}

static PLDHashTableOps AttributeSelectorOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashGetKeyStub,
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
      mMedium(aMedium),
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

  nsCOMPtr<nsIAtom> mMedium;
  RuleCascadeData*  mNext; 
};

nsVoidArray*
RuleCascadeData::AttributeListFor(nsIAtom* aAttribute)
{
  AttributeSelectorEntry *entry = NS_STATIC_CAST(AttributeSelectorEntry*,
      PL_DHashTableOperate(&mAttributeSelectors, aAttribute, PL_DHASH_ADD));
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
  : mSheets(aSheets),
    mRuleCascades(nsnull)
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

RuleProcessorData::RuleProcessorData(nsPresContext* aPresContext,
                                     nsIContent* aContent, 
                                     nsRuleWalker* aRuleWalker,
                                     nsCompatibility* aCompat )
{
  MOZ_COUNT_CTOR(RuleProcessorData);

  NS_PRECONDITION(aPresContext, "null pointer");
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

  
  if (!aCompat) {
    mCompatMode = mPresContext->CompatibilityMode();
  } else {
    mCompatMode = *aCompat;
  }


  if (aContent) {
    
    mContentTag = aContent->Tag();
    mParentContent = aContent->GetParent();

    
    mPresContext->EventStateManager()->GetContentState(aContent, mEventState);

    
    mContentID = aContent->GetID();
    mClasses = aContent->GetClasses();

    
    mHasAttributes = aContent->GetAttrCount() > 0;

    
    if (aContent->IsNodeOfType(nsINode::eHTML)) {
      mIsHTMLContent = PR_TRUE;
      
      
      mNameSpaceID = kNameSpaceID_XHTML;
    } else {
      
      mNameSpaceID = aContent->GetNameSpaceID();
    }



    
    
    if (mIsHTMLContent && mHasAttributes) {
      
      if(nsStyleUtil::IsHTMLLink(aContent, mContentTag, mPresContext, &mLinkState)) {
        mIsLink = PR_TRUE;
      }
    } 

    
    
    if(!mIsLink &&
       mHasAttributes && 
       !(mIsHTMLContent || aContent->IsNodeOfType(nsINode::eXUL)) && 
       nsStyleUtil::IsLink(aContent, mPresContext, &mLinkState)) {
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
      RuleProcessorData *d = NS_STATIC_CAST(RuleProcessorData*,
        destroyQueue.FastElementAt(destroyQueue.Count() - 1));
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
        d->Destroy(mPresContext);
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

static const PRUnichar kNullCh = PRUnichar('\0');

static PRBool ValueIncludes(const nsSubstring& aValueList,
                            const nsSubstring& aValue,
                            const nsStringComparator& aComparator)
{
  const PRUnichar *p = aValueList.BeginReading(),
              *p_end = aValueList.EndReading();

  while (p < p_end) {
    
    while (p != p_end && nsCRT::IsAsciiSpace(*p))
      ++p;

    const PRUnichar *val_start = p;

    
    while (p != p_end && !nsCRT::IsAsciiSpace(*p))
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


static PRBool IsSignificantChild(nsIContent* aChild, PRBool aTextIsSignificant, PRBool aWhitespaceIsSignificant)
{
  NS_ASSERTION(!aWhitespaceIsSignificant || aTextIsSignificant,
               "Nonsensical arguments");

  PRBool isText = aChild->IsNodeOfType(nsINode::eTEXT);

  if (!isText && !aChild->IsNodeOfType(nsINode::eCOMMENT) &&
      !aChild->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
    return PR_TRUE;
  }

  return aTextIsSignificant && isText && aChild->TextLength() != 0 &&
         (aWhitespaceIsSignificant ||
          !aChild->TextIsOnlyWhitespace());
}




static PRBool AttrMatchesValue(const nsAttrSelector* aAttrSelector,
                               const nsString& aValue)
{
  NS_PRECONDITION(aAttrSelector, "Must have an attribute selector");
  const nsDefaultStringComparator defaultComparator;
  const nsCaseInsensitiveStringComparator ciComparator;
  const nsStringComparator& comparator = aAttrSelector->mCaseSensitive
                ? NS_STATIC_CAST(const nsStringComparator&, defaultComparator)
                : NS_STATIC_CAST(const nsStringComparator&, ciComparator);
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
                              PRBool* const aDependence = nsnull) 

{
  
  if ((kNameSpaceID_Unknown != aSelector->mNameSpace &&
       data.mNameSpaceID != aSelector->mNameSpace) ||
      (aSelector->mTag && aSelector->mTag != data.mContentTag)) {
    
    return PR_FALSE;
  }

  PRBool result = PR_TRUE;
  const PRBool isNegated = (aDependence != nsnull);

  
  
  
  for (nsAtomStringList* pseudoClass = aSelector->mPseudoClassList;
       pseudoClass && result; pseudoClass = pseudoClass->mNext) {
    PRInt32 stateToCheck = 0;
    if ((nsCSSPseudoClasses::firstChild == pseudoClass->mAtom) ||
        (nsCSSPseudoClasses::firstNode == pseudoClass->mAtom) ) {
      nsIContent *firstChild = nsnull;
      nsIContent *parent = data.mParentContent;
      if (parent) {
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
    else if (nsCSSPseudoClasses::empty == pseudoClass->mAtom ||
             nsCSSPseudoClasses::mozOnlyWhitespace == pseudoClass->mAtom) {
      nsIContent *child = nsnull;
      nsIContent *element = data.mContent;
      const PRBool isWhitespaceSignificant =
        nsCSSPseudoClasses::empty == pseudoClass->mAtom;
      PRInt32 index = -1;

      do {
        child = element->GetChildAt(++index);
        
        
      } while (child && !IsSignificantChild(child, PR_TRUE, isWhitespaceSignificant));
      result = (child == nsnull);
    }
    else if (nsCSSPseudoClasses::mozEmptyExceptChildrenWithLocalname == pseudoClass->mAtom) {
      NS_ASSERTION(pseudoClass->mString, "Must have string!");
      nsIContent *child = nsnull;
      nsIContent *element = data.mContent;
      PRInt32 index = -1;

      do {
        child = element->GetChildAt(++index);
      } while (child &&
               (!IsSignificantChild(child, PR_TRUE, PR_FALSE) ||
                (child->GetNameSpaceID() == element->GetNameSpaceID() &&
                 child->Tag()->Equals(nsDependentString(pseudoClass->mString)))));
      result = (child == nsnull);
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
      result = (data.mParentContent == nsnull);
    }
    else if (nsCSSPseudoClasses::mozBoundElement == pseudoClass->mAtom) {
      
      
      
      result = (data.mScopedRoot && data.mScopedRoot == data.mContent);
    }
    else if (nsCSSPseudoClasses::lang == pseudoClass->mAtom) {
      NS_ASSERTION(nsnull != pseudoClass->mString, "null lang parameter");
      result = PR_FALSE;
      if (pseudoClass->mString && *pseudoClass->mString) {
        
        
        
        
        const nsString* lang = data.GetLang();
        if (lang && !lang->IsEmpty()) { 
          result = nsStyleUtil::DashMatchCompare(*lang,
                                    nsDependentString(pseudoClass->mString), 
                                    nsCaseInsensitiveStringComparator());
        }
        else if (data.mContent) {
          nsIDocument* doc = data.mContent->GetDocument();
          if (doc) {
            
            
            
            
            nsAutoString language;
            doc->GetContentLanguage(language);

            nsDependentString langString(pseudoClass->mString);
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
                                aAttribute, &dependence);
      
      
      
      
      
      result = result || dependence;
    }
  }
  return result;
}

#undef STATE_CHECK





#define NS_IS_GREEDY_OPERATOR(ch) (ch == PRUnichar(0) || ch == PRUnichar('~'))

static PRBool SelectorMatchesTree(RuleProcessorData& aPrevData,
                                  nsCSSSelector* aSelector) 
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
          PRInt32 index = parent->IndexOf(content);
          while (0 <= --index) {
            content = parent->GetChildAt(index);
            if (content->IsNodeOfType(nsINode::eELEMENT)) {
              data = new (prevdata->mPresContext)
                          RuleProcessorData(prevdata->mPresContext, content,
                                            prevdata->mRuleWalker,
                                            &prevdata->mCompatMode);
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
        if (content) {
          data = new (prevdata->mPresContext)
                      RuleProcessorData(prevdata->mPresContext, content,
                                        prevdata->mRuleWalker,
                                        &prevdata->mCompatMode);
          prevdata->mParentData = data;    
        }
      }
    }
    if (! data) {
      return PR_FALSE;
    }
    if (SelectorMatches(*data, selector, 0, nsnull)) {
      
      
      if ((NS_IS_GREEDY_OPERATOR(selector->mOperator)) &&
          (selector->mNext) &&
          (!NS_IS_GREEDY_OPERATOR(selector->mNext->mOperator))) {

        
        

        
        
        
        
        if (SelectorMatchesTree(*data, selector)) {
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

  if (SelectorMatches(*data, aSelector, 0, nsnull)) {
    nsCSSSelector *next = aSelector->mNext;
    if (!next || SelectorMatchesTree(*data, next)) {
      
      
      
#ifdef DEBUG
      nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
      NS_ASSERTION(NS_STATIC_CAST(nsIStyleRule*, aRule) == iRule.get(),
                   "Please fix QI so this performance optimization is valid");
#endif
      data->mRuleWalker->Forward(NS_STATIC_CAST(nsIStyleRule*, aRule));
      
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
      if (SelectorMatches(*data, selector, 0, nsnull)) {
        selector = selector->mNext;
      }
      else {
        if (PRUnichar('>') == selector->mOperator) {
          return; 
        }
      }
    }

    if (selector && 
        (! SelectorMatchesTree(*data, selector))) {
      return; 
    }

    
    
    
#ifdef DEBUG
    nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
    NS_ASSERTION(NS_STATIC_CAST(nsIStyleRule*, aRule) == iRule.get(),
                 "Please fix QI so this performance optimization is valid");
#endif
    data->mRuleWalker->Forward(NS_STATIC_CAST(nsIStyleRule*, aRule));
    
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
  StateEnumData *enumData = NS_STATIC_CAST(StateEnumData*, aData);
  StateRuleProcessorData *data = enumData->data;
  nsCSSSelector* selector = NS_STATIC_CAST(nsCSSSelector*, aSelector);

  nsReStyleHint possibleChange = IsSiblingOperator(selector->mOperator) ?
    eReStyle_LaterSiblings : eReStyle_Self;

  
  
  
  if ((possibleChange & ~(enumData->change)) &&
      SelectorMatches(*data, selector, data->mStateMask, nsnull) &&
      SelectorMatchesTree(*data, selector->mNext)) {
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
  AttributeEnumData *enumData = NS_STATIC_CAST(AttributeEnumData*, aData);
  AttributeRuleProcessorData *data = enumData->data;
  nsCSSSelector* selector = NS_STATIC_CAST(nsCSSSelector*, aSelector);

  nsReStyleHint possibleChange = IsSiblingOperator(selector->mOperator) ?
    eReStyle_LaterSiblings : eReStyle_Self;

  
  
  
  if ((possibleChange & ~(enumData->change)) &&
      SelectorMatches(*data, selector, 0, data->mAttribute) &&
      SelectorMatchesTree(*data, selector->mNext)) {
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
  
  
  

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext);

  
  
  

  if (cascade) {
    if (aData->mAttribute == aData->mContent->GetIDAttributeName()) {
      cascade->mIDSelectors.EnumerateForwards(AttributeEnumFunc, &data);
    }
    
    if (aData->mAttribute == aData->mContent->GetClassAttributeName()) {
      cascade->mClassSelectors.EnumerateForwards(AttributeEnumFunc, &data);
    }

    AttributeSelectorEntry *entry = NS_STATIC_CAST(AttributeSelectorEntry*,
        PL_DHashTableOperate(&cascade->mAttributeSelectors, aData->mAttribute,
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      entry->mSelectors->EnumerateForwards(AttributeEnumFunc, &data);
    }
  }

  *aResult = data.change;
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
  for (nsAtomStringList* pseudoClass = aSelector.mPseudoClassList;
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
        (pseudoClass->mAtom == nsCSSPseudoClasses::defaultPseudo)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
AddRule(void* aRuleInfo, void* aCascade)
{
  RuleValue* ruleInfo = NS_STATIC_CAST(RuleValue*, aRuleInfo);
  RuleCascadeData *cascade = NS_STATIC_CAST(RuleCascadeData*, aCascade);

  
  cascade->mRuleHash.PrependRule(ruleInfo);

  nsVoidArray* stateArray = &cascade->mStateSelectors;
  nsVoidArray* classArray = &cascade->mClassSelectors;
  nsVoidArray* idArray = &cascade->mIDSelectors;
  
  for (nsCSSSelector* selector = ruleInfo->mSelector;
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

PR_STATIC_CALLBACK(PRIntn)
RuleArraysDestroy(nsHashKey *aKey, void *aData, void *aClosure)
{
  delete NS_STATIC_CAST(nsAutoVoidArray*, aData);
  return PR_TRUE;
}

struct CascadeEnumData {
  CascadeEnumData(nsPresContext* aPresContext, PLArenaPool& aArena)
    : mPresContext(aPresContext),
      mRuleArrays(nsnull, nsnull, RuleArraysDestroy, nsnull, 64),
      mArena(aArena)
  {
  }

  nsPresContext* mPresContext;
  nsObjectHashtable mRuleArrays; 
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
      nsPRUint32Key key(weight);
      nsAutoVoidArray *rules =
        NS_STATIC_CAST(nsAutoVoidArray*, data->mRuleArrays.Get(&key));
      if (!rules) {
        rules = new nsAutoVoidArray();
        if (!rules) return PR_FALSE; 
        data->mRuleArrays.Put(&key, rules);
      }
      RuleValue *info =
        new (data->mArena) RuleValue(styleRule, sel->mSelectors);
      rules->AppendElement(info);
    }
  }
  else if (nsICSSRule::MEDIA_RULE == type ||
           nsICSSRule::DOCUMENT_RULE == type) {
    nsICSSGroupRule* groupRule = (nsICSSGroupRule*)aRule;
    if (groupRule->UseForPresentation(data->mPresContext))
      groupRule->EnumerateRulesForwards(InsertRuleByWeight, aData);
  }
  return PR_TRUE;
}


static PRBool
CascadeSheetRulesInto(nsICSSStyleSheet* aSheet, void* aData)
{
  nsCSSStyleSheet*  sheet = NS_STATIC_CAST(nsCSSStyleSheet*, aSheet);
  CascadeEnumData* data = NS_STATIC_CAST(CascadeEnumData*, aData);
  PRBool bSheetApplicable = PR_TRUE;
  sheet->GetApplicable(bSheetApplicable);

  if (bSheetApplicable && sheet->UseForMedium(data->mPresContext)) {
    nsCSSStyleSheet* child = sheet->mFirstChild;
    while (child) {
      CascadeSheetRulesInto(child, data);
      child = child->mNext;
    }

    if (sheet->mInner) {
      sheet->mInner->mOrderedRules.EnumerateForwards(InsertRuleByWeight, data);
    }
  }
  return PR_TRUE;
}

struct RuleArrayData {
  PRInt32 mWeight;
  nsVoidArray* mRuleArray;
};

PR_STATIC_CALLBACK(int) CompareArrayData(const void* aArg1, const void* aArg2,
                                         void* closure)
{
  const RuleArrayData* arg1 = NS_STATIC_CAST(const RuleArrayData*, aArg1);
  const RuleArrayData* arg2 = NS_STATIC_CAST(const RuleArrayData*, aArg2);
  return arg1->mWeight - arg2->mWeight; 
}


struct FillArrayData {
  FillArrayData(RuleArrayData* aArrayData) :
    mIndex(0),
    mArrayData(aArrayData)
  {
  }
  PRInt32 mIndex;
  RuleArrayData* mArrayData;
};

PR_STATIC_CALLBACK(PRBool)
FillArray(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsPRUint32Key* key = NS_STATIC_CAST(nsPRUint32Key*, aKey);
  nsVoidArray* weightArray = NS_STATIC_CAST(nsVoidArray*, aData);
  FillArrayData* data = NS_STATIC_CAST(FillArrayData*, aClosure);

  RuleArrayData& ruleData = data->mArrayData[data->mIndex++];
  ruleData.mRuleArray = weightArray;
  ruleData.mWeight = key->GetValue();

  return PR_TRUE;
}






static void PutRulesInList(nsObjectHashtable* aRuleArrays,
                           nsVoidArray* aWeightedRules)
{
  PRInt32 arrayCount = aRuleArrays->Count();
  RuleArrayData* arrayData = new RuleArrayData[arrayCount];
  FillArrayData faData(arrayData);
  aRuleArrays->Enumerate(FillArray, &faData);
  NS_QuickSort(arrayData, arrayCount, sizeof(RuleArrayData),
               CompareArrayData, nsnull);
  for (PRInt32 i = 0; i < arrayCount; ++i)
    aWeightedRules->AppendElements(*arrayData[i].mRuleArray);

  delete [] arrayData;
}

RuleCascadeData*
nsCSSRuleProcessor::GetRuleCascade(nsPresContext* aPresContext)
{
  
  
  
  
  

  RuleCascadeData **cascadep = &mRuleCascades;
  RuleCascadeData *cascade;
  nsIAtom *medium = aPresContext->Medium();
  while ((cascade = *cascadep)) {
    if (cascade->mMedium == medium)
      return cascade;
    cascadep = &cascade->mNext;
  }

  if (mSheets.Count() != 0) {
    cascade = new RuleCascadeData(medium,
                                  eCompatibility_NavQuirks == aPresContext->CompatibilityMode());
    if (cascade) {
      CascadeEnumData data(aPresContext, cascade->mRuleHash.Arena());
      mSheets.EnumerateForwards(CascadeSheetRulesInto, &data);
      nsVoidArray weightedRules;
      PutRulesInList(&data.mRuleArrays, &weightedRules);

      
      
      if (!weightedRules.EnumerateBackwards(AddRule, cascade)) {
        delete cascade;
        cascade = nsnull;
      }

      *cascadep = cascade;
    }
  }
  return cascade;
}
