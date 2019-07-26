









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/uscript.h"
#include "nultrans.h"
#include "anytrans.h"
#include "uvector.h"
#include "tridpars.h"
#include "hash.h"
#include "putilimp.h"
#include "uinvchar.h"




static const UChar TARGET_SEP = 45; 
static const UChar VARIANT_SEP = 47; 
static const UChar ANY[] = {65,110,121,0}; 
static const UChar NULL_ID[] = {78,117,108,108,0}; 
static const UChar LATIN_PIVOT[] = {45,76,97,116,105,110,59,76,97,116,105,110,45,0}; 



U_CDECL_BEGIN



static void U_CALLCONV
_deleteTransliterator(void *obj) {
    delete (icu::Transliterator*) obj;    
}
U_CDECL_END



U_NAMESPACE_BEGIN
















class ScriptRunIterator : public UMemory {
private:
    const Replaceable& text;
    int32_t textStart;
    int32_t textLimit;

public:
    




    UScriptCode scriptCode;

    


    int32_t start;

    


    int32_t limit;
    
    



    ScriptRunIterator(const Replaceable& text, int32_t start, int32_t limit);

    




    UBool next();

    



    void adjustLimit(int32_t delta);

private:
    ScriptRunIterator(const ScriptRunIterator &other); 
    ScriptRunIterator &operator=(const ScriptRunIterator &other); 
};

ScriptRunIterator::ScriptRunIterator(const Replaceable& theText,
                                     int32_t myStart, int32_t myLimit) :
    text(theText)
{
    textStart = myStart;
    textLimit = myLimit;
    limit = myStart;
}

UBool ScriptRunIterator::next() {
    UChar32 ch;
    UScriptCode s;
    UErrorCode ec = U_ZERO_ERROR;

    scriptCode = USCRIPT_INVALID_CODE; 
    start = limit;

    
    if (start == textLimit) {
        return FALSE;
    }

    
    
    while (start > textStart) {
        ch = text.char32At(start - 1); 
        s = uscript_getScript(ch, &ec);
        if (s == USCRIPT_COMMON || s == USCRIPT_INHERITED) {
            --start;
        } else {
            break;
        }
    }

    
    
    while (limit < textLimit) {
        ch = text.char32At(limit); 
        s = uscript_getScript(ch, &ec);
        if (s != USCRIPT_COMMON && s != USCRIPT_INHERITED) {
            if (scriptCode == USCRIPT_INVALID_CODE) {
                scriptCode = s;
            } else if (s != scriptCode) {
                break;
            }
        }
        ++limit;
    }

    
    
    return TRUE;
}

void ScriptRunIterator::adjustLimit(int32_t delta) {
    limit += delta;
    textLimit += delta;
}




UOBJECT_DEFINE_RTTI_IMPLEMENTATION(AnyTransliterator)

AnyTransliterator::AnyTransliterator(const UnicodeString& id,
                                     const UnicodeString& theTarget,
                                     const UnicodeString& theVariant,
                                     UScriptCode theTargetScript,
                                     UErrorCode& ec) :
    Transliterator(id, NULL),
    targetScript(theTargetScript) 
{
    cache = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &ec);
    if (U_FAILURE(ec)) {
        return;
    }
    uhash_setValueDeleter(cache, _deleteTransliterator);

    target = theTarget;
    if (theVariant.length() > 0) {
        target.append(VARIANT_SEP).append(theVariant);
    }
}

AnyTransliterator::~AnyTransliterator() {
    uhash_close(cache);
}




AnyTransliterator::AnyTransliterator(const AnyTransliterator& o) :
    Transliterator(o),
    target(o.target),
    targetScript(o.targetScript)
{
    
    UErrorCode ec = U_ZERO_ERROR;
    cache = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &ec);
    if (U_FAILURE(ec)) {
        return;
    }
    uhash_setValueDeleter(cache, _deleteTransliterator);
}




Transliterator* AnyTransliterator::clone() const {
    return new AnyTransliterator(*this);
}




void AnyTransliterator::handleTransliterate(Replaceable& text, UTransPosition& pos,
                                            UBool isIncremental) const {
    int32_t allStart = pos.start;
    int32_t allLimit = pos.limit;

    ScriptRunIterator it(text, pos.contextStart, pos.contextLimit);

    while (it.next()) {
        
        if (it.limit <= allStart) continue;

        
        
        Transliterator* t = getTransliterator(it.scriptCode);
       
        if (t == NULL) {
            
            
            pos.start = it.limit;
            continue;
        }

        
        
        
        UBool incremental = isIncremental && (it.limit >= allLimit);
        
        pos.start = uprv_max(allStart, it.start);
        pos.limit = uprv_min(allLimit, it.limit);
        int32_t limit = pos.limit;
        t->filteredTransliterate(text, pos, incremental);
        int32_t delta = pos.limit - limit;
        allLimit += delta;
        it.adjustLimit(delta);

        
        if (it.limit >= allLimit) break;
    }

    
    
    pos.limit = allLimit;
}

Transliterator* AnyTransliterator::getTransliterator(UScriptCode source) const {

    if (source == targetScript || source == USCRIPT_INVALID_CODE) {
        return NULL;
    }

    Transliterator* t = (Transliterator*) uhash_iget(cache, (int32_t) source);
    if (t == NULL) {
        UErrorCode ec = U_ZERO_ERROR;
        UnicodeString sourceName(uscript_getName(source), -1, US_INV);
        UnicodeString id(sourceName);
        id.append(TARGET_SEP).append(target);
        
        t = Transliterator::createInstance(id, UTRANS_FORWARD, ec);
        if (U_FAILURE(ec) || t == NULL) {
            delete t;
            
            
            id = sourceName;
            id.append(LATIN_PIVOT, -1).append(target);
            t = Transliterator::createInstance(id, UTRANS_FORWARD, ec);
            if (U_FAILURE(ec) || t == NULL) {
                delete t;
                t = NULL;
            }
        }

        if (t != NULL) {
            uhash_iput(cache, (int32_t) source, t, &ec);
        }
    }

    return t;
}




static UScriptCode scriptNameToCode(const UnicodeString& name) {
    char buf[128];
    UScriptCode code;
    UErrorCode ec = U_ZERO_ERROR;
    int32_t nameLen = name.length();
    UBool isInvariant = uprv_isInvariantUString(name.getBuffer(), nameLen);
    
    if (isInvariant) {
        name.extract(0, nameLen, buf, (int32_t)sizeof(buf), US_INV);
        buf[127] = 0;   
    }
    if (!isInvariant || uscript_getCode(buf, &code, 1, &ec) != 1 || U_FAILURE(ec))
    {
        code = USCRIPT_INVALID_CODE;
    }
    return code;
}






void AnyTransliterator::registerIDs() {

    UErrorCode ec = U_ZERO_ERROR;
    Hashtable seen(TRUE, ec);

    int32_t sourceCount = Transliterator::_countAvailableSources();
    for (int32_t s=0; s<sourceCount; ++s) {
        UnicodeString source;
        Transliterator::_getAvailableSource(s, source);

        
        if (source.caseCompare(ANY, 3, 0 ) == 0) continue;

        int32_t targetCount = Transliterator::_countAvailableTargets(source);
        for (int32_t t=0; t<targetCount; ++t) {
            UnicodeString target;
            Transliterator::_getAvailableTarget(t, source, target);

            
            if (seen.geti(target) != 0) continue;
            ec = U_ZERO_ERROR;
            seen.puti(target, 1, ec);
            
            
            UScriptCode targetScript = scriptNameToCode(target);
            if (targetScript == USCRIPT_INVALID_CODE) continue;

            int32_t variantCount = Transliterator::_countAvailableVariants(source, target);
            
            for (int32_t v=0; v<variantCount; ++v) {
                UnicodeString variant;
                Transliterator::_getAvailableVariant(v, source, target, variant);
                
                UnicodeString id;
                TransliteratorIDParser::STVtoID(UnicodeString(TRUE, ANY, 3), target, variant, id);
                ec = U_ZERO_ERROR;
                AnyTransliterator* t = new AnyTransliterator(id, target, variant,
                                                             targetScript, ec);
                if (U_FAILURE(ec)) {
                    delete t;
                } else {
                    Transliterator::_registerInstance(t);
                    Transliterator::_registerSpecialInverse(target, UnicodeString(TRUE, NULL_ID, 4), FALSE);
                }
            }
        }
    }
}

U_NAMESPACE_END

#endif 


