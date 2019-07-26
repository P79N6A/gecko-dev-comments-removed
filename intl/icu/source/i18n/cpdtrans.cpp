









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifilt.h"
#include "unicode/uniset.h"
#include "cpdtrans.h"
#include "uvector.h"
#include "tridpars.h"
#include "cmemory.h"



static const UChar ID_DELIM = 0x003B; 
static const UChar NEWLINE  = 10;

static const UChar COLON_COLON[] = {0x3A, 0x3A, 0}; 

U_NAMESPACE_BEGIN

const UChar CompoundTransliterator::PASS_STRING[] = { 0x0025, 0x0050, 0x0061, 0x0073, 0x0073, 0 }; 

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CompoundTransliterator)















CompoundTransliterator::CompoundTransliterator(
                           Transliterator* const transliterators[],
                           int32_t transliteratorCount,
                           UnicodeFilter* adoptedFilter) :
    Transliterator(joinIDs(transliterators, transliteratorCount), adoptedFilter),
    trans(0), count(0), numAnonymousRBTs(0)  {
    setTransliterators(transliterators, transliteratorCount);
}








CompoundTransliterator::CompoundTransliterator(const UnicodeString& id,
                              UTransDirection direction,
                              UnicodeFilter* adoptedFilter,
                              UParseError& ,
                              UErrorCode& status) :
    Transliterator(id, adoptedFilter),
    trans(0), numAnonymousRBTs(0) {
    
    
    init(id, direction, TRUE, status);
}

CompoundTransliterator::CompoundTransliterator(const UnicodeString& id,
                              UParseError& ,
                              UErrorCode& status) :
    Transliterator(id, 0), 
    trans(0), numAnonymousRBTs(0) {
    
    
    init(id, UTRANS_FORWARD, TRUE, status);
}





CompoundTransliterator::CompoundTransliterator(const UnicodeString& newID,
                                              UVector& list,
                                              UnicodeFilter* adoptedFilter,
                                              int32_t anonymousRBTs,
                                              UParseError& ,
                                              UErrorCode& status) :
    Transliterator(newID, adoptedFilter),
    trans(0), numAnonymousRBTs(anonymousRBTs)
{
    init(list, UTRANS_FORWARD, FALSE, status);
}






CompoundTransliterator::CompoundTransliterator(UVector& list,
                                               UParseError& ,
                                               UErrorCode& status) :
    Transliterator(UnicodeString(), NULL),
    trans(0), numAnonymousRBTs(0)
{
    
    
    init(list, UTRANS_FORWARD, FALSE, status);
    
}

CompoundTransliterator::CompoundTransliterator(UVector& list,
                                               int32_t anonymousRBTs,
                                               UParseError& ,
                                               UErrorCode& status) :
    Transliterator(UnicodeString(), NULL),
    trans(0), numAnonymousRBTs(anonymousRBTs)
{
    init(list, UTRANS_FORWARD, FALSE, status);
}

















void CompoundTransliterator::init(const UnicodeString& id,
                                  UTransDirection direction,
                                  UBool fixReverseID,
                                  UErrorCode& status) {
    

    if (U_FAILURE(status)) {
        return;
    }

    UVector list(status);
    UnicodeSet* compoundFilter = NULL;
    UnicodeString regenID;
    if (!TransliteratorIDParser::parseCompoundID(id, direction,
                                      regenID, list, compoundFilter)) {
        status = U_INVALID_ID;
        delete compoundFilter;
        return;
    }

    TransliteratorIDParser::instantiateList(list, status);

    init(list, direction, fixReverseID, status);

    if (compoundFilter != NULL) {
        adoptFilter(compoundFilter);
    }
}














void CompoundTransliterator::init(UVector& list,
                                  UTransDirection direction,
                                  UBool fixReverseID,
                                  UErrorCode& status) {
    

    
    if (U_SUCCESS(status)) {
        count = list.size();
        trans = (Transliterator **)uprv_malloc(count * sizeof(Transliterator *));
        
        if (trans == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    if (U_FAILURE(status) || trans == 0) {
         
        return;
    }

    
    
    int32_t i;
    for (i=0; i<count; ++i) {
        int32_t j = (direction == UTRANS_FORWARD) ? i : count - 1 - i;
        trans[i] = (Transliterator*) list.elementAt(j);
    }

    
    
    if (direction == UTRANS_REVERSE && fixReverseID) {
        UnicodeString newID;
        for (i=0; i<count; ++i) {
            if (i > 0) {
                newID.append(ID_DELIM);
            }
            newID.append(trans[i]->getID());
        }
        setID(newID);
    }

    computeMaximumContextLength();
}






UnicodeString CompoundTransliterator::joinIDs(Transliterator* const transliterators[],
                                              int32_t transCount) {
    UnicodeString id;
    for (int32_t i=0; i<transCount; ++i) {
        if (i > 0) {
            id.append(ID_DELIM);
        }
        id.append(transliterators[i]->getID());
    }
    return id; 
}




CompoundTransliterator::CompoundTransliterator(const CompoundTransliterator& t) :
    Transliterator(t), trans(0), count(0), numAnonymousRBTs(-1) {
    *this = t;
}




CompoundTransliterator::~CompoundTransliterator() {
    freeTransliterators();
}

void CompoundTransliterator::freeTransliterators(void) {
    if (trans != 0) {
        for (int32_t i=0; i<count; ++i) {
            delete trans[i];
        }
        uprv_free(trans);
    }
    trans = 0;
    count = 0;
}




CompoundTransliterator& CompoundTransliterator::operator=(
                                             const CompoundTransliterator& t)
{
    Transliterator::operator=(t);
    int32_t i = 0;
    UBool failed = FALSE;
    if (trans != NULL) {
        for (i=0; i<count; ++i) {
            delete trans[i];
            trans[i] = 0;
        }
    }
    if (t.count > count) {
        if (trans != NULL) {
            uprv_free(trans);
        }
        trans = (Transliterator **)uprv_malloc(t.count * sizeof(Transliterator *));
    }
    count = t.count;
    if (trans != NULL) {
        for (i=0; i<count; ++i) {
            trans[i] = t.trans[i]->clone();
            if (trans[i] == NULL) {
                failed = TRUE;
                break;
            }
        }
    }

    
    if (failed && i > 0) {
        int32_t n;
        for (n = i-1; n >= 0; n--) {
            uprv_free(trans[n]);
            trans[n] = NULL;
        }
    }
    numAnonymousRBTs = t.numAnonymousRBTs;
    return *this;
}




Transliterator* CompoundTransliterator::clone(void) const {
    return new CompoundTransliterator(*this);
}





int32_t CompoundTransliterator::getCount(void) const {
    return count;
}






const Transliterator& CompoundTransliterator::getTransliterator(int32_t index) const {
    return *trans[index];
}

void CompoundTransliterator::setTransliterators(Transliterator* const transliterators[],
                                                int32_t transCount) {
    Transliterator** a = (Transliterator **)uprv_malloc(transCount * sizeof(Transliterator *));
    if (a == NULL) {
        return;
    }
    int32_t i = 0;
    UBool failed = FALSE;
    for (i=0; i<transCount; ++i) {
        a[i] = transliterators[i]->clone();
        if (a[i] == NULL) {
            failed = TRUE;
            break;
        }
    }
    if (failed && i > 0) {
        int32_t n;
        for (n = i-1; n >= 0; n--) {
            uprv_free(a[n]);
            a[n] = NULL;
        }
        return;
    }
    adoptTransliterators(a, transCount);
}

void CompoundTransliterator::adoptTransliterators(Transliterator* adoptedTransliterators[],
                                                  int32_t transCount) {
    
    
    freeTransliterators();
    trans = adoptedTransliterators;
    count = transCount;
    computeMaximumContextLength();
    setID(joinIDs(trans, count));
}




static void _smartAppend(UnicodeString& buf, UChar c) {
    if (buf.length() != 0 &&
        buf.charAt(buf.length() - 1) != c) {
        buf.append(c);
    }
}

UnicodeString& CompoundTransliterator::toRules(UnicodeString& rulesSource,
                                               UBool escapeUnprintable) const {
    
    
    
    
    
    
    rulesSource.truncate(0);
    if (numAnonymousRBTs >= 1 && getFilter() != NULL) {
        
        
        UnicodeString pat;
        rulesSource.append(COLON_COLON, 2).append(getFilter()->toPattern(pat, escapeUnprintable)).append(ID_DELIM);
    }
    for (int32_t i=0; i<count; ++i) {
        UnicodeString rule;

        
        
        
        
        if (trans[i]->getID().startsWith(PASS_STRING, 5)) {
            trans[i]->toRules(rule, escapeUnprintable);
            if (numAnonymousRBTs > 1 && i > 0 && trans[i - 1]->getID().startsWith(PASS_STRING, 5))
                rule = UNICODE_STRING_SIMPLE("::Null;") + rule;

        
        
        
        
        } else if (trans[i]->getID().indexOf(ID_DELIM) >= 0) {
            trans[i]->toRules(rule, escapeUnprintable);

        
        } else {
            trans[i]->Transliterator::toRules(rule, escapeUnprintable);
        }
        _smartAppend(rulesSource, NEWLINE);
        rulesSource.append(rule);
        _smartAppend(rulesSource, ID_DELIM);
    }
    return rulesSource;
}




void CompoundTransliterator::handleGetSourceSet(UnicodeSet& result) const {
    UnicodeSet set;
    result.clear();
    for (int32_t i=0; i<count; ++i) {
    result.addAll(trans[i]->getSourceSet(set));
    
    
    
    
    
    

    
    if (!result.isEmpty()) {
        break;
    }
    }
}




UnicodeSet& CompoundTransliterator::getTargetSet(UnicodeSet& result) const {
    UnicodeSet set;
    result.clear();
    for (int32_t i=0; i<count; ++i) {
    
    result.addAll(trans[i]->getTargetSet(set));
    }
    return result;
}




void CompoundTransliterator::handleTransliterate(Replaceable& text, UTransPosition& index,
                                                 UBool incremental) const {
    






















    
    






























    if (count < 1) {
        index.start = index.limit;
        return; 
    }

    
    
    
    
    int32_t compoundLimit = index.limit;

    
    
    int32_t compoundStart = index.start;
    
    int32_t delta = 0; 

    
    
    for (int32_t i=0; i<count; ++i) {
        index.start = compoundStart; 
        int32_t limit = index.limit;
        
        if (index.start == index.limit) {
            
            break;
        }

        trans[i]->filteredTransliterate(text, index, incremental);
        
        
        
        
        
        
        
        
        if (!incremental && index.start != index.limit) {
            
            index.start = index.limit;
        }

        
        delta += index.limit - limit;
        
        if (incremental) {
            
            
            
            
            
            index.limit = index.start;
        }
    }

    compoundLimit += delta;

    
    
    
    index.limit = compoundLimit;
}





void CompoundTransliterator::computeMaximumContextLength(void) {
    int32_t max = 0;
    for (int32_t i=0; i<count; ++i) {
        int32_t len = trans[i]->getMaximumContextLength();
        if (len > max) {
            max = len;
        }
    }
    setMaximumContextLength(max);
}

U_NAMESPACE_END

#endif 


