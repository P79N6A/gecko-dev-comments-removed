









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "tridpars.h"
#include "hash.h"
#include "mutex.h"
#include "ucln_in.h"
#include "unicode/parsepos.h"
#include "unicode/translit.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/utrans.h"
#include "util.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

static const UChar ID_DELIM    = 0x003B; 
static const UChar TARGET_SEP  = 0x002D; 
static const UChar VARIANT_SEP = 0x002F; 
static const UChar OPEN_REV    = 0x0028; 
static const UChar CLOSE_REV   = 0x0029; 


static const UChar ANY[]       = {65,110,121,0}; 
static const UChar ANY_NULL[]  = {65,110,121,45,78,117,108,108,0}; 

static const int32_t FORWARD = UTRANS_FORWARD;
static const int32_t REVERSE = UTRANS_REVERSE;

static Hashtable* SPECIAL_INVERSES = NULL;




static UMutex LOCK = U_MUTEX_INITIALIZER;

TransliteratorIDParser::Specs::Specs(const UnicodeString& s, const UnicodeString& t,
                                     const UnicodeString& v, UBool sawS,
                                     const UnicodeString& f) {
    source = s;
    target = t;
    variant = v;
    sawSource = sawS;
    filter = f;
}

TransliteratorIDParser::SingleID::SingleID(const UnicodeString& c, const UnicodeString& b,
                                           const UnicodeString& f) {
    canonID = c;
    basicID = b;
    filter = f;
}

TransliteratorIDParser::SingleID::SingleID(const UnicodeString& c, const UnicodeString& b) {
    canonID = c;
    basicID = b;
}

Transliterator* TransliteratorIDParser::SingleID::createInstance() {
    Transliterator* t;
    if (basicID.length() == 0) {
        t = createBasicInstance(UnicodeString(TRUE, ANY_NULL, 8), &canonID);
    } else {
        t = createBasicInstance(basicID, &canonID);
    }
    if (t != NULL) {
        if (filter.length() != 0) {
            UErrorCode ec = U_ZERO_ERROR;
            UnicodeSet *set = new UnicodeSet(filter, ec);
            if (U_FAILURE(ec)) {
                delete set;
            } else {
                t->adoptFilter(set);
            }
        }
    }
    return t;
}














TransliteratorIDParser::SingleID*
TransliteratorIDParser::parseSingleID(const UnicodeString& id, int32_t& pos,
                                      int32_t dir, UErrorCode& status) {

    int32_t start = pos;

    
    
    Specs* specsA = NULL;
    Specs* specsB = NULL;
    UBool sawParen = FALSE;

    
    
    for (int32_t pass=1; pass<=2; ++pass) {
        if (pass == 2) {
            specsA = parseFilterID(id, pos, TRUE);
            if (specsA == NULL) {
                pos = start;
                return NULL;
            }
        }
        if (ICU_Utility::parseChar(id, pos, OPEN_REV)) {
            sawParen = TRUE;
            if (!ICU_Utility::parseChar(id, pos, CLOSE_REV)) {
                specsB = parseFilterID(id, pos, TRUE);
                
                if (specsB == NULL || !ICU_Utility::parseChar(id, pos, CLOSE_REV)) {
                    delete specsA;
                    pos = start;
                    return NULL;
                }
            }
            break;
        }
    }

    
    SingleID* single;
    if (sawParen) {
        if (dir == FORWARD) {
            SingleID* b = specsToID(specsB, FORWARD);
            single = specsToID(specsA, FORWARD);
            
            if (b == NULL || single == NULL) {
            	delete b;
            	delete single;
            	status = U_MEMORY_ALLOCATION_ERROR;
            	return NULL;
            }
            single->canonID.append(OPEN_REV)
                .append(b->canonID).append(CLOSE_REV);
            if (specsA != NULL) {
                single->filter = specsA->filter;
            }
            delete b;
        } else {
            SingleID* a = specsToID(specsA, FORWARD);
            single = specsToID(specsB, FORWARD);
            
            if (a == NULL || single == NULL) {
            	delete a;
            	delete single;
            	status = U_MEMORY_ALLOCATION_ERROR;
            	return NULL;
            }
            single->canonID.append(OPEN_REV)
                .append(a->canonID).append(CLOSE_REV);
            if (specsB != NULL) {
                single->filter = specsB->filter;
            }
            delete a;
        }
    } else {
        
        if (dir == FORWARD) {
            single = specsToID(specsA, FORWARD);
        } else {
            single = specsToSpecialInverse(*specsA, status);
            if (single == NULL) {
                single = specsToID(specsA, REVERSE);
            }
        }
        
        if (single == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return NULL;
        }
        single->filter = specsA->filter;
    }

    delete specsA;
    delete specsB;

    return single;
}










TransliteratorIDParser::SingleID*
TransliteratorIDParser::parseFilterID(const UnicodeString& id, int32_t& pos) {

    int32_t start = pos;

    Specs* specs = parseFilterID(id, pos, TRUE);
    if (specs == NULL) {
        pos = start;
        return NULL;
    }

    
    SingleID* single = specsToID(specs, FORWARD);
    if (single != NULL) {
        single->filter = specs->filter;
    }
    delete specs;
    return single;
}























UnicodeSet* TransliteratorIDParser::parseGlobalFilter(const UnicodeString& id, int32_t& pos,
                                                      int32_t dir,
                                                      int32_t& withParens,
                                                      UnicodeString* canonID) {
    UnicodeSet* filter = NULL;
    int32_t start = pos;

    if (withParens == -1) {
        withParens = ICU_Utility::parseChar(id, pos, OPEN_REV) ? 1 : 0;
    } else if (withParens == 1) {
        if (!ICU_Utility::parseChar(id, pos, OPEN_REV)) {
            pos = start;
            return NULL;
        }
    }

    ICU_Utility::skipWhitespace(id, pos, TRUE);

    if (UnicodeSet::resemblesPattern(id, pos)) {
        ParsePosition ppos(pos);
        UErrorCode ec = U_ZERO_ERROR;
        filter = new UnicodeSet(id, ppos, USET_IGNORE_SPACE, NULL, ec);
        
        if (filter == 0) {
            pos = start;
            return 0;
        }
        if (U_FAILURE(ec)) {
            delete filter;
            pos = start;
            return NULL;
        }

        UnicodeString pattern;
        id.extractBetween(pos, ppos.getIndex(), pattern);
        pos = ppos.getIndex();

        if (withParens == 1 && !ICU_Utility::parseChar(id, pos, CLOSE_REV)) {
            pos = start;
            return NULL;
        }

        
        
        
        if (canonID != NULL) {
            if (dir == FORWARD) {
                if (withParens == 1) {
                    pattern.insert(0, OPEN_REV);
                    pattern.append(CLOSE_REV);
                }
                canonID->append(pattern).append(ID_DELIM);
            } else {
                if (withParens == 0) {
                    pattern.insert(0, OPEN_REV);
                    pattern.append(CLOSE_REV);
                }
                canonID->insert(0, pattern);
                canonID->insert(pattern.length(), ID_DELIM);
            }
        }
    }

    return filter;
}

U_CDECL_BEGIN
static void U_CALLCONV _deleteSingleID(void* obj) {
    delete (TransliteratorIDParser::SingleID*) obj;
}

static void U_CALLCONV _deleteTransliteratorTrIDPars(void* obj) {
    delete (Transliterator*) obj;
}
U_CDECL_END























UBool TransliteratorIDParser::parseCompoundID(const UnicodeString& id, int32_t dir,
                                              UnicodeString& canonID,
                                              UVector& list,
                                              UnicodeSet*& globalFilter) {
    UErrorCode ec = U_ZERO_ERROR;
    int32_t i;
    int32_t pos = 0;
    int32_t withParens = 1;
    list.removeAllElements();
    UnicodeSet* filter;
    globalFilter = NULL;
    canonID.truncate(0);

    
    withParens = 0; 
    filter = parseGlobalFilter(id, pos, dir, withParens, &canonID);
    if (filter != NULL) {
        if (!ICU_Utility::parseChar(id, pos, ID_DELIM)) {
            
            canonID.truncate(0);
            pos = 0;
        }
        if (dir == FORWARD) {
            globalFilter = filter;
        } else {
            delete filter;
        }
        filter = NULL;
    }

    UBool sawDelimiter = TRUE;
    for (;;) {
        SingleID* single = parseSingleID(id, pos, dir, ec);
        if (single == NULL) {
            break;
        }
        if (dir == FORWARD) {
            list.addElement(single, ec);
        } else {
            list.insertElementAt(single, 0, ec);
        }
        if (U_FAILURE(ec)) {
            goto FAIL;
        }
        if (!ICU_Utility::parseChar(id, pos, ID_DELIM)) {
            sawDelimiter = FALSE;
            break;
        }
    }

    if (list.size() == 0) {
        goto FAIL;
    }

    
    for (i=0; i<list.size(); ++i) {
        SingleID* single = (SingleID*) list.elementAt(i);
        canonID.append(single->canonID);
        if (i != (list.size()-1)) {
            canonID.append(ID_DELIM);
        }
    }

    
    
    if (sawDelimiter) {
        withParens = 1; 
        filter = parseGlobalFilter(id, pos, dir, withParens, &canonID);
        if (filter != NULL) {
            
            ICU_Utility::parseChar(id, pos, ID_DELIM);

            if (dir == REVERSE) {
                globalFilter = filter;
            } else {
                delete filter;
            }
            filter = NULL;
        }
    }

    
    ICU_Utility::skipWhitespace(id, pos, TRUE);
    if (pos != id.length()) {
        goto FAIL;
    }

    return TRUE;

 FAIL:
    UObjectDeleter *save = list.setDeleter(_deleteSingleID);
    list.removeAllElements();
    list.setDeleter(save);
    delete globalFilter;
    globalFilter = NULL;
    return FALSE;
}


















void TransliteratorIDParser::instantiateList(UVector& list,
                                                UErrorCode& ec) {
    UVector tlist(ec);
    if (U_FAILURE(ec)) {
        goto RETURN;
    }
    tlist.setDeleter(_deleteTransliteratorTrIDPars);

    Transliterator* t;
    int32_t i;
    for (i=0; i<=list.size(); ++i) { 
        
        
        if (i==list.size()) {
            break;
        }

        SingleID* single = (SingleID*) list.elementAt(i);
        if (single->basicID.length() != 0) {
            t = single->createInstance();
            if (t == NULL) {
                ec = U_INVALID_ID;
                goto RETURN;
            }
            tlist.addElement(t, ec);
            if (U_FAILURE(ec)) {
                delete t;
                goto RETURN;
            }
        }
    }

    
    if (tlist.size() == 0) {
        t = createBasicInstance(UnicodeString(TRUE, ANY_NULL, 8), NULL);
        if (t == NULL) {
            
            ec = U_INTERNAL_TRANSLITERATOR_ERROR;
        }
        tlist.addElement(t, ec);
        if (U_FAILURE(ec)) {
            delete t;
        }
    }

 RETURN:

    UObjectDeleter *save = list.setDeleter(_deleteSingleID);
    list.removeAllElements();

    if (U_SUCCESS(ec)) {
        list.setDeleter(_deleteTransliteratorTrIDPars);

        while (tlist.size() > 0) {
            t = (Transliterator*) tlist.orphanElementAt(0);
            list.addElement(t, ec);
            if (U_FAILURE(ec)) {
                delete t;
                list.removeAllElements();
                break;
            }
        }
    }

    list.setDeleter(save);
}












void TransliteratorIDParser::IDtoSTV(const UnicodeString& id,
                                     UnicodeString& source,
                                     UnicodeString& target,
                                     UnicodeString& variant,
                                     UBool& isSourcePresent) {
    source.setTo(ANY, 3);
    target.truncate(0);
    variant.truncate(0);

    int32_t sep = id.indexOf(TARGET_SEP);
    int32_t var = id.indexOf(VARIANT_SEP);
    if (var < 0) {
        var = id.length();
    }
    isSourcePresent = FALSE;

    if (sep < 0) {
        
        id.extractBetween(0, var, target);
        id.extractBetween(var, id.length(), variant);
    } else if (sep < var) {
        
        if (sep > 0) {
            id.extractBetween(0, sep, source);
            isSourcePresent = TRUE;
        }
        id.extractBetween(++sep, var, target);
        id.extractBetween(var, id.length(), variant);
    } else {
        
        if (var > 0) {
            id.extractBetween(0, var, source);
            isSourcePresent = TRUE;
        }
        id.extractBetween(var, sep++, variant);
        id.extractBetween(sep, id.length(), target);
    }

    if (variant.length() > 0) {
        variant.remove(0, 1);
    }
}






void TransliteratorIDParser::STVtoID(const UnicodeString& source,
                                     const UnicodeString& target,
                                     const UnicodeString& variant,
                                     UnicodeString& id) {
    id = source;
    if (id.length() == 0) {
        id.setTo(ANY, 3);
    }
    id.append(TARGET_SEP).append(target);
    if (variant.length() != 0) {
        id.append(VARIANT_SEP).append(variant);
    }
    
    
    id.append((UChar)0);
    id.truncate(id.length()-1);
}

































void TransliteratorIDParser::registerSpecialInverse(const UnicodeString& target,
                                                    const UnicodeString& inverseTarget,
                                                    UBool bidirectional,
                                                    UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }

    
    if (bidirectional && 0==target.caseCompare(inverseTarget, U_FOLD_CASE_DEFAULT)) {
        bidirectional = FALSE;
    }

    Mutex lock(&LOCK);

    UnicodeString *tempus = new UnicodeString(inverseTarget);  
    if (tempus == NULL) {
    	status = U_MEMORY_ALLOCATION_ERROR;
    	return;
    }
    SPECIAL_INVERSES->put(target, tempus, status);
    if (bidirectional) {
    	tempus = new UnicodeString(target);
    	if (tempus == NULL) {
    		status = U_MEMORY_ALLOCATION_ERROR;
    		return;
    	}
        SPECIAL_INVERSES->put(inverseTarget, tempus, status);
    }
}
























TransliteratorIDParser::Specs*
TransliteratorIDParser::parseFilterID(const UnicodeString& id, int32_t& pos,
                                      UBool allowFilter) {
    UnicodeString first;
    UnicodeString source;
    UnicodeString target;
    UnicodeString variant;
    UnicodeString filter;
    UChar delimiter = 0;
    int32_t specCount = 0;
    int32_t start = pos;

    
    
    
    for (;;) {
        ICU_Utility::skipWhitespace(id, pos, TRUE);
        if (pos == id.length()) {
            break;
        }

        
        if (allowFilter && filter.length() == 0 &&
            UnicodeSet::resemblesPattern(id, pos)) {

            ParsePosition ppos(pos);
            UErrorCode ec = U_ZERO_ERROR;
            UnicodeSet set(id, ppos, USET_IGNORE_SPACE, NULL, ec);
            if (U_FAILURE(ec)) {
                pos = start;
                return NULL;
            }
            id.extractBetween(pos, ppos.getIndex(), filter);
            pos = ppos.getIndex();
            continue;
        }

        if (delimiter == 0) {
            UChar c = id.charAt(pos);
            if ((c == TARGET_SEP && target.length() == 0) ||
                (c == VARIANT_SEP && variant.length() == 0)) {
                delimiter = c;
                ++pos;
                continue;
            }
        }

        
        
        
        if (delimiter == 0 && specCount > 0) {
            break;
        }

        UnicodeString spec = ICU_Utility::parseUnicodeIdentifier(id, pos);
        if (spec.length() == 0) {
            
            
            
            break;
        }

        switch (delimiter) {
        case 0:
            first = spec;
            break;
        case TARGET_SEP:
            target = spec;
            break;
        case VARIANT_SEP:
            variant = spec;
            break;
        }
        ++specCount;
        delimiter = 0;
    }

    
    
    if (first.length() != 0) {
        if (target.length() == 0) {
            target = first;
        } else {
            source = first;
        }
    }

    
    if (source.length() == 0 && target.length() == 0) {
        pos = start;
        return NULL;
    }

    
    UBool sawSource = TRUE;
    if (source.length() == 0) {
        source.setTo(ANY, 3);
        sawSource = FALSE;
    }
    if (target.length() == 0) {
        target.setTo(ANY, 3);
    }

    return new Specs(source, target, variant, sawSource, filter);
}








TransliteratorIDParser::SingleID*
TransliteratorIDParser::specsToID(const Specs* specs, int32_t dir) {
    UnicodeString canonID;
    UnicodeString basicID;
    UnicodeString basicPrefix;
    if (specs != NULL) {
        UnicodeString buf;
        if (dir == FORWARD) {
            if (specs->sawSource) {
                buf.append(specs->source).append(TARGET_SEP);
            } else {
                basicPrefix = specs->source;
                basicPrefix.append(TARGET_SEP);
            }
            buf.append(specs->target);
        } else {
            buf.append(specs->target).append(TARGET_SEP).append(specs->source);
        }
        if (specs->variant.length() != 0) {
            buf.append(VARIANT_SEP).append(specs->variant);
        }
        basicID = basicPrefix;
        basicID.append(buf);
        if (specs->filter.length() != 0) {
            buf.insert(0, specs->filter);
        }
        canonID = buf;
    }
    return new SingleID(canonID, basicID);
}








TransliteratorIDParser::SingleID*
TransliteratorIDParser::specsToSpecialInverse(const Specs& specs, UErrorCode &status) {
    if (0!=specs.source.caseCompare(ANY, 3, U_FOLD_CASE_DEFAULT)) {
        return NULL;
    }
    init(status);

    UnicodeString* inverseTarget;

    umtx_lock(&LOCK);
    inverseTarget = (UnicodeString*) SPECIAL_INVERSES->get(specs.target);
    umtx_unlock(&LOCK);

    if (inverseTarget != NULL) {
        
        
        
        UnicodeString buf;
        if (specs.filter.length() != 0) {
            buf.append(specs.filter);
        }
        if (specs.sawSource) {
            buf.append(ANY, 3).append(TARGET_SEP);
        }
        buf.append(*inverseTarget);

        UnicodeString basicID(TRUE, ANY, 3);
        basicID.append(TARGET_SEP).append(*inverseTarget);

        if (specs.variant.length() != 0) {
            buf.append(VARIANT_SEP).append(specs.variant);
            basicID.append(VARIANT_SEP).append(specs.variant);
        }
        return new SingleID(buf, basicID);
    }
    return NULL;
}






Transliterator* TransliteratorIDParser::createBasicInstance(const UnicodeString& id, const UnicodeString* canonID) {
    return Transliterator::createBasicInstance(id, canonID);
}




void TransliteratorIDParser::init(UErrorCode &status) {
    if (SPECIAL_INVERSES != NULL) {
        return;
    }

    Hashtable* special_inverses = new Hashtable(TRUE, status);
    
    if (special_inverses == NULL) {
    	status = U_MEMORY_ALLOCATION_ERROR;
    	return;
    }
    special_inverses->setValueDeleter(uprv_deleteUObject);

    umtx_lock(&LOCK);
    if (SPECIAL_INVERSES == NULL) {
        SPECIAL_INVERSES = special_inverses;
        special_inverses = NULL;
    }
    umtx_unlock(&LOCK);
    delete special_inverses; 

    ucln_i18n_registerCleanup(UCLN_I18N_TRANSLITERATOR, utrans_transliterator_cleanup);
}




void TransliteratorIDParser::cleanup() {
    if (SPECIAL_INVERSES) {
        delete SPECIAL_INVERSES;
        SPECIAL_INVERSES = NULL;
    }
}

U_NAMESPACE_END

#endif 


