









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/rep.h"
#include "unicode/uniset.h"
#include "rbt_pars.h"
#include "rbt_data.h"
#include "rbt_rule.h"
#include "rbt.h"
#include "umutex.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedTransliterator)

static UMutex transliteratorDataMutex = U_MUTEX_INITIALIZER;
static Replaceable *gLockedText = NULL;

void RuleBasedTransliterator::_construct(const UnicodeString& rules,
                                         UTransDirection direction,
                                         UParseError& parseError,
                                         UErrorCode& status) {
    fData = 0;
    isDataOwned = TRUE;
    if (U_FAILURE(status)) {
        return;
    }

    TransliteratorParser parser(status);
    parser.parse(rules, direction, parseError, status);
    if (U_FAILURE(status)) {
        return;
    }

    if (parser.idBlockVector.size() != 0 ||
        parser.compoundFilter != NULL ||
        parser.dataVector.size() == 0) {
        status = U_INVALID_RBT_SYNTAX; 
        return;
    }

    fData = (TransliterationRuleData*)parser.dataVector.orphanElementAt(0);
    setMaximumContextLength(fData->ruleSet.getMaximumContextLength());
}













RuleBasedTransliterator::RuleBasedTransliterator(
                            const UnicodeString& id,
                            const UnicodeString& rules,
                            UTransDirection direction,
                            UnicodeFilter* adoptedFilter,
                            UParseError& parseError,
                            UErrorCode& status) :
    Transliterator(id, adoptedFilter) {
    _construct(rules, direction,parseError,status);
}




























































RuleBasedTransliterator::RuleBasedTransliterator(const UnicodeString& id,
                                 const TransliterationRuleData* theData,
                                 UnicodeFilter* adoptedFilter) :
    Transliterator(id, adoptedFilter),
    fData((TransliterationRuleData*)theData), 
    isDataOwned(FALSE) {
    setMaximumContextLength(fData->ruleSet.getMaximumContextLength());
}




RuleBasedTransliterator::RuleBasedTransliterator(const UnicodeString& id,
                                                 TransliterationRuleData* theData,
                                                 UBool isDataAdopted) :
    Transliterator(id, 0),
    fData(theData),
    isDataOwned(isDataAdopted) {
    setMaximumContextLength(fData->ruleSet.getMaximumContextLength());
}




RuleBasedTransliterator::RuleBasedTransliterator(
        const RuleBasedTransliterator& other) :
    Transliterator(other), fData(other.fData),
    isDataOwned(other.isDataOwned) {

    
    
    
    
    

    
    
    
    if (isDataOwned) {
        fData = new TransliterationRuleData(*other.fData);
    }
}




RuleBasedTransliterator::~RuleBasedTransliterator() {
    
    if (isDataOwned) {
        delete fData;
    }
}

Transliterator* 
RuleBasedTransliterator::clone(void) const {
    return new RuleBasedTransliterator(*this);
}




void
RuleBasedTransliterator::handleTransliterate(Replaceable& text, UTransPosition& index,
                                             UBool isIncremental) const {
    














    








    uint32_t loopCount = 0;
    uint32_t loopLimit = index.limit - index.start;
    if (loopLimit >= 0x10000000) {
        loopLimit = 0xFFFFFFFF;
    } else {
        loopLimit <<= 4;
    }

    
    
    
    
    
    

    
    
    
    UBool    lockedMutexAtThisLevel = FALSE;
    if (isDataOwned == FALSE) {
        
        
        
        
        UBool needToLock;
        UMTX_CHECK(NULL, (&text != gLockedText), needToLock);
        if (needToLock) {
            umtx_lock(&transliteratorDataMutex);
            gLockedText = &text;
            lockedMutexAtThisLevel = TRUE;
        }
    }
    
    
    if (fData != NULL) {
	    while (index.start < index.limit &&
	           loopCount <= loopLimit &&
	           fData->ruleSet.transliterate(text, index, isIncremental)) {
	        ++loopCount;
	    }
    }
    if (lockedMutexAtThisLevel) {
        gLockedText = NULL;
        umtx_unlock(&transliteratorDataMutex);
    }
}

UnicodeString& RuleBasedTransliterator::toRules(UnicodeString& rulesSource,
                                                UBool escapeUnprintable) const {
    return fData->ruleSet.toRules(rulesSource, escapeUnprintable);
}




void RuleBasedTransliterator::handleGetSourceSet(UnicodeSet& result) const {
    fData->ruleSet.getSourceTargetSet(result, FALSE);
}




UnicodeSet& RuleBasedTransliterator::getTargetSet(UnicodeSet& result) const {
    return fData->ruleSet.getSourceTargetSet(result, TRUE);
}

U_NAMESPACE_END

#endif 
