









#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/putil.h"
#include "unicode/translit.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/rep.h"
#include "unicode/resbund.h"
#include "unicode/unifilt.h"
#include "unicode/uniset.h"
#include "unicode/uscript.h"
#include "unicode/strenum.h"
#include "unicode/utf16.h"
#include "cpdtrans.h"
#include "nultrans.h"
#include "rbt_data.h"
#include "rbt_pars.h"
#include "rbt.h"
#include "transreg.h"
#include "name2uni.h"
#include "nortrans.h"
#include "remtrans.h"
#include "titletrn.h"
#include "tolowtrn.h"
#include "toupptrn.h"
#include "uni2name.h"
#include "brktrans.h"
#include "esctrn.h"
#include "unesctrn.h"
#include "tridpars.h"
#include "anytrans.h"
#include "util.h"
#include "hash.h"
#include "mutex.h"
#include "ucln_in.h"
#include "uassert.h"
#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"

static const UChar TARGET_SEP  = 0x002D; 
static const UChar ID_DELIM    = 0x003B; 
static const UChar VARIANT_SEP = 0x002F; 






static const char RB_DISPLAY_NAME_PREFIX[] = "%Translit%%";






static const char RB_SCRIPT_DISPLAY_NAME_PREFIX[] = "%Translit%";







static const char RB_DISPLAY_NAME_PATTERN[] = "TransliteratorNamePattern";







static const char RB_RULE_BASED_IDS[] = "RuleBasedTransliteratorIDs";




static UMutex registryMutex = U_MUTEX_INITIALIZER;




static icu::TransliteratorRegistry* registry = 0;



#define HAVE_REGISTRY(status) (registry!=0 || initializeRegistry(status))

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(Transliterator)





static inline UBool positionIsValid(UTransPosition& index, int32_t len) {
    return !(index.contextStart < 0 ||
             index.start < index.contextStart ||
             index.limit < index.start ||
             index.contextLimit < index.limit ||
             len < index.contextLimit);
}









Transliterator::Transliterator(const UnicodeString& theID,
                               UnicodeFilter* adoptedFilter) :
    UObject(), ID(theID), filter(adoptedFilter),
    maximumContextLength(0)
{
    
    ID.append((UChar)0);
    ID.truncate(ID.length()-1);
}




Transliterator::~Transliterator() {
    if (filter) {
        delete filter;
    }
}




Transliterator::Transliterator(const Transliterator& other) :
    UObject(other), ID(other.ID), filter(0),
    maximumContextLength(other.maximumContextLength)
{
    
    ID.append((UChar)0);
    ID.truncate(ID.length()-1);

    if (other.filter != 0) {
        
        filter = (UnicodeFilter*) other.filter->clone();
    }
}

Transliterator* Transliterator::clone() const {
    return NULL;
}




Transliterator& Transliterator::operator=(const Transliterator& other) {
    ID = other.ID;
    
    ID.getTerminatedBuffer();

    maximumContextLength = other.maximumContextLength;
    adoptFilter((other.filter == 0) ? 0 : (UnicodeFilter*) other.filter->clone());
    return *this;
}










int32_t Transliterator::transliterate(Replaceable& text,
                                      int32_t start, int32_t limit) const {
    if (start < 0 ||
        limit < start ||
        text.length() < limit) {
        return -1;
    }

    UTransPosition offsets;
    offsets.contextStart= start;
    offsets.contextLimit = limit;
    offsets.start = start;
    offsets.limit = limit;
    filteredTransliterate(text, offsets, FALSE, TRUE);
    return offsets.limit;
}





void Transliterator::transliterate(Replaceable& text) const {
    transliterate(text, 0, text.length());
}

































































void Transliterator::transliterate(Replaceable& text,
                                   UTransPosition& index,
                                   const UnicodeString& insertion,
                                   UErrorCode &status) const {
    _transliterate(text, index, &insertion, status);
}
















void Transliterator::transliterate(Replaceable& text,
                                   UTransPosition& index,
                                   UChar32 insertion,
                                   UErrorCode& status) const {
    UnicodeString str(insertion);
    _transliterate(text, index, &str, status);
}












void Transliterator::transliterate(Replaceable& text,
                                   UTransPosition& index,
                                   UErrorCode& status) const {
    _transliterate(text, index, 0, status);
}











void Transliterator::finishTransliteration(Replaceable& text,
                                           UTransPosition& index) const {
    if (!positionIsValid(index, text.length())) {
        return;
    }

    filteredTransliterate(text, index, FALSE, TRUE);
}








void Transliterator::_transliterate(Replaceable& text,
                                    UTransPosition& index,
                                    const UnicodeString* insertion,
                                    UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return;
    }

    if (!positionIsValid(index, text.length())) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }


    if (insertion != 0) {
        text.handleReplaceBetween(index.limit, index.limit, *insertion);
        index.limit += insertion->length();
        index.contextLimit += insertion->length();
    }

    if (index.limit > 0 &&
        U16_IS_LEAD(text.charAt(index.limit - 1))) {
        
        
        
        
        return;
    }

    filteredTransliterate(text, index, TRUE, TRUE);

#if 0
    
    
    

    

    
    
    
    
    

    
    
    
    

    

    
    
    
    
    
    int32_t newCS = index.start;
    int32_t n = getMaximumContextLength();
    while (newCS > originalStart && n-- > 0) {
        --newCS;
        newCS -= U16_LENGTH(text.char32At(newCS)) - 1;
    }
    index.contextStart = uprv_max(newCS, originalStart);
#endif
}









void Transliterator::filteredTransliterate(Replaceable& text,
                                           UTransPosition& index,
                                           UBool incremental,
                                           UBool rollback) const {
    
    
    if (filter == 0 && !rollback) {
        handleTransliterate(text, index, incremental);
        return;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t globalLimit = index.limit;
    
    
    
    
    
    
    
    
    
    
    
    for (;;) {

        if (filter != NULL) {
            
            

            
            UChar32 c;
            while (index.start < globalLimit &&
                   !filter->contains(c=text.char32At(index.start))) {
                index.start += U16_LENGTH(c);
            }

            
            index.limit = index.start;
            while (index.limit < globalLimit &&
                   filter->contains(c=text.char32At(index.limit))) {
                index.limit += U16_LENGTH(c);
            }
        }

        
        
        
        if (index.limit == index.start) {
            
            break;
        }

        
        
        
        
        UBool isIncrementalRun =
            (index.limit < globalLimit ? FALSE : incremental);
        
        int32_t delta;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (rollback && isIncrementalRun) {

            int32_t runStart = index.start;
            int32_t runLimit = index.limit;
            int32_t runLength =  runLimit - runStart;

            
            int32_t rollbackOrigin = text.length();
            text.copy(runStart, runLimit, rollbackOrigin);

            
            
            
            
            
            int32_t passStart = runStart;
            int32_t rollbackStart = rollbackOrigin;

            
            
            int32_t passLimit = index.start;

            
            
            int32_t uncommittedLength = 0;

            
            int32_t totalDelta = 0;

            
            
            
            for (;;) {
                
                int32_t charLength = U16_LENGTH(text.char32At(passLimit));
                passLimit += charLength;
                if (passLimit > runLimit) {
                    break;
                }
                uncommittedLength += charLength;

                index.limit = passLimit;

                
                
                
                
                handleTransliterate(text, index, TRUE);

                delta = index.limit - passLimit; 

                
                
                
                if (index.start != index.limit) {
                    
                    
                    int32_t rs = rollbackStart + delta - (index.limit - passStart);

                    
                    text.handleReplaceBetween(passStart, index.limit, UnicodeString());

                    
                    text.copy(rs, rs + uncommittedLength, passStart);

                    
                    index.start = passStart;
                    index.limit = passLimit;
                    index.contextLimit -= delta;
                }

                
                
                
                else {
                    
                    passStart = passLimit = index.start;

                    
                    
                    
                    
                    rollbackStart += delta + uncommittedLength;
                    uncommittedLength = 0;

                    
                    runLimit += delta;
                    totalDelta += delta;
                }
            }

            
            
            
            rollbackOrigin += totalDelta;
            globalLimit += totalDelta;

            
            text.handleReplaceBetween(rollbackOrigin, rollbackOrigin + runLength, UnicodeString());

            
            index.start = passStart;
        }

        else {
            
            int32_t limit = index.limit;
            handleTransliterate(text, index, isIncrementalRun);
            delta = index.limit - limit; 

            
            
            
            
            
            
            
            if (!incremental && index.start != index.limit) {
                
                index.start = index.limit;
            }

            
            
            
            globalLimit += delta;
        }

        if (filter == NULL || isIncrementalRun) {
            break;
        }

        
        
    }

    
    
    index.limit = globalLimit;
}

void Transliterator::filteredTransliterate(Replaceable& text,
                                           UTransPosition& index,
                                           UBool incremental) const {
    filteredTransliterate(text, index, incremental, FALSE);
}





void Transliterator::setMaximumContextLength(int32_t maxContextLength) {
    maximumContextLength = maxContextLength;
}








const UnicodeString& Transliterator::getID(void) const {
    return ID;
}






UnicodeString& U_EXPORT2 Transliterator::getDisplayName(const UnicodeString& ID,
                                              UnicodeString& result) {
    return getDisplayName(ID, Locale::getDefault(), result);
}



















UnicodeString& U_EXPORT2 Transliterator::getDisplayName(const UnicodeString& id,
                                              const Locale& inLocale,
                                              UnicodeString& result) {
    UErrorCode status = U_ZERO_ERROR;

    ResourceBundle bundle(U_ICUDATA_TRANSLIT, inLocale, status);

    

    result.truncate(0);

    
    UnicodeString source, target, variant;
    UBool sawSource;
    TransliteratorIDParser::IDtoSTV(id, source, target, variant, sawSource);
    if (target.length() < 1) {
        
        return result;
    }
    if (variant.length() > 0) { 
        variant.insert(0, VARIANT_SEP);
    }
    UnicodeString ID(source);
    ID.append(TARGET_SEP).append(target).append(variant);

    
    if (uprv_isInvariantUString(ID.getBuffer(), ID.length())) {
        char key[200];
        uprv_strcpy(key, RB_DISPLAY_NAME_PREFIX);
        int32_t length=(int32_t)uprv_strlen(RB_DISPLAY_NAME_PREFIX);
        ID.extract(0, (int32_t)(sizeof(key)-length), key+length, (int32_t)(sizeof(key)-length), US_INV);

        
        UnicodeString resString = bundle.getStringEx(key, status);

        if (U_SUCCESS(status) && resString.length() != 0) {
            return result = resString; 
        }

#if !UCONFIG_NO_FORMATTING
        
        
        
        
        

        status = U_ZERO_ERROR;
        resString = bundle.getStringEx(RB_DISPLAY_NAME_PATTERN, status);

        if (U_SUCCESS(status) && resString.length() != 0) {
            MessageFormat msg(resString, inLocale, status);
            

            
            Formattable args[3];
            int32_t nargs;
            args[0].setLong(2); 
            args[1].setString(source);
            args[2].setString(target);
            nargs = 3;

            
            UnicodeString s;
            length=(int32_t)uprv_strlen(RB_SCRIPT_DISPLAY_NAME_PREFIX);
            for (int j=1; j<=2; ++j) {
                status = U_ZERO_ERROR;
                uprv_strcpy(key, RB_SCRIPT_DISPLAY_NAME_PREFIX);
                args[j].getString(s);
                if (uprv_isInvariantUString(s.getBuffer(), s.length())) {
                    s.extract(0, sizeof(key)-length-1, key+length, (int32_t)sizeof(key)-length-1, US_INV);

                    resString = bundle.getStringEx(key, status);

                    if (U_SUCCESS(status)) {
                        args[j] = resString;
                    }
                }
            }

            status = U_ZERO_ERROR;
            FieldPosition pos; 
            msg.format(args, nargs, result, pos, status);
            if (U_SUCCESS(status)) {
                result.append(variant);
                return result;
            }
        }
#endif
    }

    
    
    
    result = ID;
    return result;
}






const UnicodeFilter* Transliterator::getFilter(void) const {
    return filter;
}







UnicodeFilter* Transliterator::orphanFilter(void) {
    UnicodeFilter *result = filter;
    filter = NULL;
    return result;
}









void Transliterator::adoptFilter(UnicodeFilter* filterToAdopt) {
    delete filter;
    filter = filterToAdopt;
}





















Transliterator* Transliterator::createInverse(UErrorCode& status) const {
    UParseError parseError;
    return Transliterator::createInstance(ID, UTRANS_REVERSE,parseError,status);
}

Transliterator* U_EXPORT2
Transliterator::createInstance(const UnicodeString& ID,
                                UTransDirection dir,
                                UErrorCode& status)
{
    UParseError parseError;
    return createInstance(ID, dir, parseError, status);
}












Transliterator* U_EXPORT2
Transliterator::createInstance(const UnicodeString& ID,
                                UTransDirection dir,
                                UParseError& parseError,
                                UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return 0;
    }

    UnicodeString canonID;
    UVector list(status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    UnicodeSet* globalFilter;
    
    
    if (!TransliteratorIDParser::parseCompoundID(ID, dir, canonID, list, globalFilter)) {
        status = U_INVALID_ID;
        return NULL;
    }
    
    TransliteratorIDParser::instantiateList(list, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    
    U_ASSERT(list.size() > 0);
    Transliterator* t = NULL;
    
    if (list.size() > 1 || canonID.indexOf(ID_DELIM) >= 0) {
        
        
        
        
        
        t = new CompoundTransliterator(list, parseError, status);
    }
    else {
        t = (Transliterator*)list.elementAt(0);
    }
    
    if (t != NULL) {
        t->setID(canonID);
        if (globalFilter != NULL) {
            t->adoptFilter(globalFilter);
        }
    }
    else if (U_SUCCESS(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return t;
}









Transliterator* Transliterator::createBasicInstance(const UnicodeString& id,
                                                    const UnicodeString* canon) {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    TransliteratorAlias* alias = 0;
    Transliterator* t = 0;

    umtx_lock(&registryMutex);
    if (HAVE_REGISTRY(ec)) {
        t = registry->get(id, alias, ec);
    }
    umtx_unlock(&registryMutex);

    if (U_FAILURE(ec)) {
        delete t;
        delete alias;
        return 0;
    }

    
    
    
    
    
    
    
    while (alias != 0) {
        U_ASSERT(t==0);
        
        
        
        if (alias->isRuleBased()) {
            
            TransliteratorParser parser(ec);
            alias->parse(parser, pe, ec);
            delete alias;
            alias = 0;

            
            umtx_lock(&registryMutex);
            if (HAVE_REGISTRY(ec)) {
                t = registry->reget(id, parser, alias, ec);
            }
            umtx_unlock(&registryMutex);

            
        } else {
            t = alias->create(pe, ec);
            delete alias;
            alias = 0;
            break;
        }
        if (U_FAILURE(ec)) {
            delete t;
            delete alias;
            t = NULL;
            break;
        }
    }

    if (t != NULL && canon != NULL) {
        t->setID(*canon);
    }

    return t;
}









Transliterator* U_EXPORT2
Transliterator::createFromRules(const UnicodeString& ID,
                                const UnicodeString& rules,
                                UTransDirection dir,
                                UParseError& parseError,
                                UErrorCode& status)
{
    Transliterator* t = NULL;

    TransliteratorParser parser(status);
    parser.parse(rules, dir, parseError, status);

    if (U_FAILURE(status)) {
        return 0;
    }

    
    if (parser.idBlockVector.size() == 0 && parser.dataVector.size() == 0) {
        t = new NullTransliterator();
    }
    else if (parser.idBlockVector.size() == 0 && parser.dataVector.size() == 1) {
        t = new RuleBasedTransliterator(ID, (TransliterationRuleData*)parser.dataVector.orphanElementAt(0), TRUE);
    }
    else if (parser.idBlockVector.size() == 1 && parser.dataVector.size() == 0) {
        
        
        
        
        if (parser.compoundFilter != NULL) {
            UnicodeString filterPattern;
            parser.compoundFilter->toPattern(filterPattern, FALSE);
            t = createInstance(filterPattern + UnicodeString(ID_DELIM)
                    + *((UnicodeString*)parser.idBlockVector.elementAt(0)), UTRANS_FORWARD, parseError, status);
        }
        else
            t = createInstance(*((UnicodeString*)parser.idBlockVector.elementAt(0)), UTRANS_FORWARD, parseError, status);


        if (t != NULL) {
            t->setID(ID);
        }
    }
    else {
        UVector transliterators(status);
        int32_t passNumber = 1;

        int32_t limit = parser.idBlockVector.size();
        if (parser.dataVector.size() > limit)
            limit = parser.dataVector.size();

        for (int32_t i = 0; i < limit; i++) {
            if (i < parser.idBlockVector.size()) {
                UnicodeString* idBlock = (UnicodeString*)parser.idBlockVector.elementAt(i);
                if (!idBlock->isEmpty()) {
                    Transliterator* temp = createInstance(*idBlock, UTRANS_FORWARD, parseError, status);
                    if (temp != NULL && typeid(*temp) != typeid(NullTransliterator))
                        transliterators.addElement(temp, status);
                    else
                        delete temp;
                }
            }
            if (!parser.dataVector.isEmpty()) {
                TransliterationRuleData* data = (TransliterationRuleData*)parser.dataVector.orphanElementAt(0);
                
                RuleBasedTransliterator* temprbt = new RuleBasedTransliterator(UnicodeString(CompoundTransliterator::PASS_STRING) + UnicodeString(passNumber++),
                        data, TRUE);
                
                if (temprbt == NULL) {
                	status = U_MEMORY_ALLOCATION_ERROR;
                	return t;
                }
                transliterators.addElement(temprbt, status);
            }
        }

        t = new CompoundTransliterator(transliterators, passNumber - 1, parseError, status);
        
        if (t != NULL) {
            t->setID(ID);
            t->adoptFilter(parser.orphanCompoundFilter());
        }
    }
    if (U_SUCCESS(status) && t == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return t;
}

UnicodeString& Transliterator::toRules(UnicodeString& rulesSource,
                                       UBool escapeUnprintable) const {
    
    
    if (escapeUnprintable) {
        rulesSource.truncate(0);
        UnicodeString id = getID();
        for (int32_t i=0; i<id.length();) {
            UChar32 c = id.char32At(i);
            if (!ICU_Utility::escapeUnprintable(rulesSource, c)) {
                rulesSource.append(c);
            }
            i += U16_LENGTH(c);
        }
    } else {
        rulesSource = getID();
    }
    
    rulesSource.insert(0, UNICODE_STRING_SIMPLE("::"));
    rulesSource.append(ID_DELIM);
    return rulesSource;
}

int32_t Transliterator::countElements() const {
    const CompoundTransliterator* ct = dynamic_cast<const CompoundTransliterator*>(this);
    return ct != NULL ? ct->getCount() : 0;
}

const Transliterator& Transliterator::getElement(int32_t index, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return *this;
    }
    const CompoundTransliterator* cpd = dynamic_cast<const CompoundTransliterator*>(this);
    int32_t n = (cpd == NULL) ? 1 : cpd->getCount();
    if (index < 0 || index >= n) {
        ec = U_INDEX_OUTOFBOUNDS_ERROR;
        return *this;
    } else {
        return (n == 1) ? *this : cpd->getTransliterator(index);
    }
}

UnicodeSet& Transliterator::getSourceSet(UnicodeSet& result) const {
    handleGetSourceSet(result);
    if (filter != NULL) {
        UnicodeSet* filterSet = dynamic_cast<UnicodeSet*>(filter);
        UBool deleteFilterSet = FALSE;
        
        
        if (filterSet == NULL) {
            filterSet = new UnicodeSet();
            
            if (filterSet == NULL) {
                return result;
            }
            deleteFilterSet = TRUE;
            filter->addMatchSetTo(*filterSet);
        }
        result.retainAll(*filterSet);
        if (deleteFilterSet) {
            delete filterSet;
        }
    }
    return result;
}

void Transliterator::handleGetSourceSet(UnicodeSet& result) const {
    result.clear();
}

UnicodeSet& Transliterator::getTargetSet(UnicodeSet& result) const {
    return result.clear();
}


void U_EXPORT2 Transliterator::registerFactory(const UnicodeString& id,
                                     Transliterator::Factory factory,
                                     Transliterator::Token context) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _registerFactory(id, factory, context);
    }
}



void Transliterator::_registerFactory(const UnicodeString& id,
                                      Transliterator::Factory factory,
                                      Transliterator::Token context) {
    UErrorCode ec = U_ZERO_ERROR;
    registry->put(id, factory, context, TRUE, ec);
}



void Transliterator::_registerSpecialInverse(const UnicodeString& target,
                                             const UnicodeString& inverseTarget,
                                             UBool bidirectional) {
    UErrorCode status = U_ZERO_ERROR;
    TransliteratorIDParser::registerSpecialInverse(target, inverseTarget, bidirectional, status);
}














void U_EXPORT2 Transliterator::registerInstance(Transliterator* adoptedPrototype) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _registerInstance(adoptedPrototype);
    }
}

void Transliterator::_registerInstance(Transliterator* adoptedPrototype) {
    UErrorCode ec = U_ZERO_ERROR;
    registry->put(adoptedPrototype, TRUE, ec);
}

void U_EXPORT2 Transliterator::registerAlias(const UnicodeString& aliasID,
                                             const UnicodeString& realID) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _registerAlias(aliasID, realID);
    }
}

void Transliterator::_registerAlias(const UnicodeString& aliasID,
                                    const UnicodeString& realID) {
    UErrorCode ec = U_ZERO_ERROR;
    registry->put(aliasID, realID, FALSE, TRUE, ec);
}









void U_EXPORT2 Transliterator::unregister(const UnicodeString& ID) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        registry->remove(ID);
    }
}







int32_t U_EXPORT2 Transliterator::countAvailableIDs(void) {
    int32_t retVal = 0;
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        retVal = registry->countAvailableIDs();
    }
    return retVal;
}







const UnicodeString& U_EXPORT2 Transliterator::getAvailableID(int32_t index) {
    const UnicodeString* result = NULL;
    umtx_lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        result = &registry->getAvailableID(index);
    }
    umtx_unlock(&registryMutex);
    U_ASSERT(result != NULL); 
    return *result;
}

StringEnumeration* U_EXPORT2 Transliterator::getAvailableIDs(UErrorCode& ec) {
    if (U_FAILURE(ec)) return NULL;
    StringEnumeration* result = NULL;
    umtx_lock(&registryMutex);
    if (HAVE_REGISTRY(ec)) {
        result = registry->getAvailableIDs();
    }
    umtx_unlock(&registryMutex);
    if (result == NULL) {
        ec = U_INTERNAL_TRANSLITERATOR_ERROR;
    }
    return result;
}

int32_t U_EXPORT2 Transliterator::countAvailableSources(void) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    return HAVE_REGISTRY(ec) ? _countAvailableSources() : 0;
}

UnicodeString& U_EXPORT2 Transliterator::getAvailableSource(int32_t index,
                                                  UnicodeString& result) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _getAvailableSource(index, result);
    }
    return result;
}

int32_t U_EXPORT2 Transliterator::countAvailableTargets(const UnicodeString& source) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    return HAVE_REGISTRY(ec) ? _countAvailableTargets(source) : 0;
}

UnicodeString& U_EXPORT2 Transliterator::getAvailableTarget(int32_t index,
                                                  const UnicodeString& source,
                                                  UnicodeString& result) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _getAvailableTarget(index, source, result);
    }
    return result;
}

int32_t U_EXPORT2 Transliterator::countAvailableVariants(const UnicodeString& source,
                                               const UnicodeString& target) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    return HAVE_REGISTRY(ec) ? _countAvailableVariants(source, target) : 0;
}

UnicodeString& U_EXPORT2 Transliterator::getAvailableVariant(int32_t index,
                                                   const UnicodeString& source,
                                                   const UnicodeString& target,
                                                   UnicodeString& result) {
    Mutex lock(&registryMutex);
    UErrorCode ec = U_ZERO_ERROR;
    if (HAVE_REGISTRY(ec)) {
        _getAvailableVariant(index, source, target, result);
    }
    return result;
}

int32_t Transliterator::_countAvailableSources(void) {
    return registry->countAvailableSources();
}

UnicodeString& Transliterator::_getAvailableSource(int32_t index,
                                                  UnicodeString& result) {
    return registry->getAvailableSource(index, result);
}

int32_t Transliterator::_countAvailableTargets(const UnicodeString& source) {
    return registry->countAvailableTargets(source);
}

UnicodeString& Transliterator::_getAvailableTarget(int32_t index,
                                                  const UnicodeString& source,
                                                  UnicodeString& result) {
    return registry->getAvailableTarget(index, source, result);
}

int32_t Transliterator::_countAvailableVariants(const UnicodeString& source,
                                               const UnicodeString& target) {
    return registry->countAvailableVariants(source, target);
}

UnicodeString& Transliterator::_getAvailableVariant(int32_t index,
                                                   const UnicodeString& source,
                                                   const UnicodeString& target,
                                                   UnicodeString& result) {
    return registry->getAvailableVariant(index, source, target, result);
}

#ifdef U_USE_DEPRECATED_TRANSLITERATOR_API







UChar Transliterator::filteredCharAt(const Replaceable& text, int32_t i) const {
    UChar c;
    const UnicodeFilter* localFilter = getFilter();
    return (localFilter == 0) ? text.charAt(i) :
        (localFilter->contains(c = text.charAt(i)) ? c : (UChar)0xFFFE);
}

#endif











UBool Transliterator::initializeRegistry(UErrorCode &status) {
    if (registry != 0) {
        return TRUE;
    }

    registry = new TransliteratorRegistry(status);
    if (registry == 0 || U_FAILURE(status)) {
        delete registry;
        registry = 0;
        return FALSE; 
    }

    

































    

    UResourceBundle *bundle, *transIDs, *colBund;
    bundle = ures_open(U_ICUDATA_TRANSLIT, NULL, &status);
    transIDs = ures_getByKey(bundle, RB_RULE_BASED_IDS, 0, &status);

    int32_t row, maxRows;
    if (U_SUCCESS(status)) {
        maxRows = ures_getSize(transIDs);
        for (row = 0; row < maxRows; row++) {
            colBund = ures_getByIndex(transIDs, row, 0, &status);
            if (U_SUCCESS(status)) {
                UnicodeString id(ures_getKey(colBund), -1, US_INV);
                UResourceBundle* res = ures_getNextResource(colBund, NULL, &status);
                const char* typeStr = ures_getKey(res);
                UChar type;
                u_charsToUChars(typeStr, &type, 1);

                if (U_SUCCESS(status)) {
                    int32_t len = 0;
                    const UChar *resString;
                    switch (type) {
                    case 0x66: 
                    case 0x69: 
                        
                        
                        {
                            
                            resString = ures_getStringByKey(res, "resource", &len, &status);
                            UBool visible = (type == 0x0066 );
                            UTransDirection dir = 
                                (ures_getUnicodeStringByKey(res, "direction", &status).charAt(0) ==
                                 0x0046 ) ?
                                UTRANS_FORWARD : UTRANS_REVERSE;
                            registry->put(id, UnicodeString(TRUE, resString, len), dir, TRUE, visible, status);
                        }
                        break;
                    case 0x61: 
                        
                        resString = ures_getString(res, &len, &status);
                        registry->put(id, UnicodeString(TRUE, resString, len), TRUE, TRUE, status);
                        break;
                    }
                }
                ures_close(res);
            }
            ures_close(colBund);
        }
    }

    ures_close(transIDs);
    ures_close(bundle);

    
    
    
    
    
    NullTransliterator* tempNullTranslit = new NullTransliterator();
    LowercaseTransliterator* tempLowercaseTranslit = new LowercaseTransliterator();
    UppercaseTransliterator* tempUppercaseTranslit = new UppercaseTransliterator();
    TitlecaseTransliterator* tempTitlecaseTranslit = new TitlecaseTransliterator();
    UnicodeNameTransliterator* tempUnicodeTranslit = new UnicodeNameTransliterator();
    NameUnicodeTransliterator* tempNameUnicodeTranslit = new NameUnicodeTransliterator();
#if !UCONFIG_NO_BREAK_ITERATION
     
     BreakTransliterator* tempBreakTranslit         = new BreakTransliterator();
#endif
    
    if (tempNullTranslit == NULL || tempLowercaseTranslit == NULL || tempUppercaseTranslit == NULL ||
        tempTitlecaseTranslit == NULL || tempUnicodeTranslit == NULL || 
#if !UCONFIG_NO_BREAK_ITERATION
        tempBreakTranslit == NULL ||
#endif
        tempNameUnicodeTranslit == NULL )
    {
        delete tempNullTranslit;
        delete tempLowercaseTranslit;
        delete tempUppercaseTranslit;
        delete tempTitlecaseTranslit;
        delete tempUnicodeTranslit;
        delete tempNameUnicodeTranslit;
#if !UCONFIG_NO_BREAK_ITERATION
        delete tempBreakTranslit;
#endif
        
        delete registry;
        registry = NULL;

        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    registry->put(tempNullTranslit, TRUE, status);
    registry->put(tempLowercaseTranslit, TRUE, status);
    registry->put(tempUppercaseTranslit, TRUE, status);
    registry->put(tempTitlecaseTranslit, TRUE, status);
    registry->put(tempUnicodeTranslit, TRUE, status);
    registry->put(tempNameUnicodeTranslit, TRUE, status);
#if !UCONFIG_NO_BREAK_ITERATION
    registry->put(tempBreakTranslit, FALSE, status);   
#endif

    RemoveTransliterator::registerIDs(); 
    EscapeTransliterator::registerIDs();
    UnescapeTransliterator::registerIDs();
    NormalizationTransliterator::registerIDs();
    AnyTransliterator::registerIDs();

    _registerSpecialInverse(UNICODE_STRING_SIMPLE("Null"),
                            UNICODE_STRING_SIMPLE("Null"), FALSE);
    _registerSpecialInverse(UNICODE_STRING_SIMPLE("Upper"),
                            UNICODE_STRING_SIMPLE("Lower"), TRUE);
    _registerSpecialInverse(UNICODE_STRING_SIMPLE("Title"),
                            UNICODE_STRING_SIMPLE("Lower"), FALSE);

    ucln_i18n_registerCleanup(UCLN_I18N_TRANSLITERATOR, utrans_transliterator_cleanup);

    return TRUE;
}

U_NAMESPACE_END








U_CFUNC UBool utrans_transliterator_cleanup(void) {
    U_NAMESPACE_USE
    TransliteratorIDParser::cleanup();
    if (registry) {
        delete registry;
        registry = NULL;
    }
    return TRUE;
}

#endif 


