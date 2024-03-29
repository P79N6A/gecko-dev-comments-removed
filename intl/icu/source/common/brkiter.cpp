



















#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/rbbi.h"
#include "unicode/brkiter.h"
#include "unicode/udata.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "ucln_cmn.h"
#include "cstring.h"
#include "umutex.h"
#include "servloc.h"
#include "locbased.h"
#include "uresimp.h"
#include "uassert.h"
#include "ubrkimpl.h"
#include "charstr.h"








U_NAMESPACE_BEGIN



BreakIterator*
BreakIterator::buildInstance(const Locale& loc, const char *type, int32_t kind, UErrorCode &status)
{
    char fnbuff[256];
    char ext[4]={'\0'};
    CharString actualLocale;
    int32_t size;
    const UChar* brkfname = NULL;
    UResourceBundle brkRulesStack;
    UResourceBundle brkNameStack;
    UResourceBundle *brkRules = &brkRulesStack;
    UResourceBundle *brkName  = &brkNameStack;
    RuleBasedBreakIterator *result = NULL;

    if (U_FAILURE(status))
        return NULL;

    ures_initStackObject(brkRules);
    ures_initStackObject(brkName);

    
    UResourceBundle *b = ures_openNoDefault(U_ICUDATA_BRKITR, loc.getName(), &status);

    
    if (U_SUCCESS(status)) {
        brkRules = ures_getByKeyWithFallback(b, "boundaries", brkRules, &status);
        
        brkName = ures_getByKeyWithFallback(brkRules, type, brkName, &status);
        
        brkfname = ures_getString(brkName, &size, &status);
        U_ASSERT((size_t)size<sizeof(fnbuff));
        if ((size_t)size>=sizeof(fnbuff)) {
            size=0;
            if (U_SUCCESS(status)) {
                status = U_BUFFER_OVERFLOW_ERROR;
            }
        }

        
        if (U_SUCCESS(status) && brkfname) {
            actualLocale.append(ures_getLocaleInternal(brkName, &status), -1, status);

            UChar* extStart=u_strchr(brkfname, 0x002e);
            int len = 0;
            if(extStart!=NULL){
                len = (int)(extStart-brkfname);
                u_UCharsToChars(extStart+1, ext, sizeof(ext)); 
                u_UCharsToChars(brkfname, fnbuff, len);
            }
            fnbuff[len]=0; 
        }
    }

    ures_close(brkRules);
    ures_close(brkName);

    UDataMemory* file = udata_open(U_ICUDATA_BRKITR, ext, fnbuff, &status);
    if (U_FAILURE(status)) {
        ures_close(b);
        return NULL;
    }

    
    result = new RuleBasedBreakIterator(file, status);

    
    if (U_SUCCESS(status) && result != NULL) {
        U_LOCALE_BASED(locBased, *(BreakIterator*)result);
        locBased.setLocaleIDs(ures_getLocaleByType(b, ULOC_VALID_LOCALE, &status), 
                              actualLocale.data());
        result->setBreakType(kind);
    }

    ures_close(b);

    if (U_FAILURE(status) && result != NULL) {  
        delete result;
        return NULL;
    }

    if (result == NULL) {
        udata_close(file);
        if (U_SUCCESS(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }

    return result;
}


BreakIterator* U_EXPORT2
BreakIterator::createWordInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_WORD, status);
}




BreakIterator* U_EXPORT2
BreakIterator::createLineInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_LINE, status);
}




BreakIterator* U_EXPORT2
BreakIterator::createCharacterInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_CHARACTER, status);
}




BreakIterator* U_EXPORT2
BreakIterator::createSentenceInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_SENTENCE, status);
}




BreakIterator* U_EXPORT2
BreakIterator::createTitleInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_TITLE, status);
}




const Locale* U_EXPORT2
BreakIterator::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}







BreakIterator::BreakIterator()
{
    *validLocale = *actualLocale = 0;
}

BreakIterator::~BreakIterator()
{
}






#if !UCONFIG_NO_SERVICE



class ICUBreakIteratorFactory : public ICUResourceBundleFactory {
public:
    virtual ~ICUBreakIteratorFactory();
protected:
    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* , UErrorCode& status) const {
        return BreakIterator::makeInstance(loc, kind, status);
    }
};

ICUBreakIteratorFactory::~ICUBreakIteratorFactory() {}



class ICUBreakIteratorService : public ICULocaleService {
public:
    ICUBreakIteratorService()
        : ICULocaleService(UNICODE_STRING("Break Iterator", 14))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new ICUBreakIteratorFactory(), status);
    }

    virtual ~ICUBreakIteratorService();

    virtual UObject* cloneInstance(UObject* instance) const {
        return ((BreakIterator*)instance)->clone();
    }

    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* , UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        int32_t kind = lkey.kind();
        Locale loc;
        lkey.currentLocale(loc);
        return BreakIterator::makeInstance(loc, kind, status);
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

ICUBreakIteratorService::~ICUBreakIteratorService() {}




U_NAMESPACE_END

static icu::UInitOnce gInitOnce;
static icu::ICULocaleService* gService = NULL;






U_CDECL_BEGIN
static UBool U_CALLCONV breakiterator_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    if (gService) {
        delete gService;
        gService = NULL;
    }
    gInitOnce.reset();
#endif
    return TRUE;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static void U_CALLCONV 
initService(void) {
    gService = new ICUBreakIteratorService();
    ucln_common_registerCleanup(UCLN_COMMON_BREAKITERATOR, breakiterator_cleanup);
}

static ICULocaleService*
getService(void)
{
    umtx_initOnce(gInitOnce, &initService);
    return gService;
}




static inline UBool
hasService(void)
{
    return !gInitOnce.isReset() && getService() != NULL;
}



URegistryKey U_EXPORT2
BreakIterator::registerInstance(BreakIterator* toAdopt, const Locale& locale, UBreakIteratorType kind, UErrorCode& status)
{
    ICULocaleService *service = getService();
    if (service == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return service->registerInstance(toAdopt, locale, kind, status);
}



UBool U_EXPORT2
BreakIterator::unregister(URegistryKey key, UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        if (hasService()) {
            return gService->unregister(key, status);
        }
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return FALSE;
}



StringEnumeration* U_EXPORT2
BreakIterator::getAvailableLocales(void)
{
    ICULocaleService *service = getService();
    if (service == NULL) {
        return NULL;
    }
    return service->getAvailableLocales();
}
#endif 



BreakIterator*
BreakIterator::createInstance(const Locale& loc, int32_t kind, UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return NULL;
    }

#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        Locale actualLoc("");
        BreakIterator *result = (BreakIterator*)gService->get(loc, kind, &actualLoc, status);
        
        
        
        
        
        
        
        
        
        
        if (U_SUCCESS(status) && (result != NULL) && *actualLoc.getName() != 0) {
            U_LOCALE_BASED(locBased, *result);
            locBased.setLocaleIDs(actualLoc.getName(), actualLoc.getName());
        }
        return result;
    }
    else
#endif
    {
        return makeInstance(loc, kind, status);
    }
}


enum { kLBTypeLenMax = 32 };

BreakIterator*
BreakIterator::makeInstance(const Locale& loc, int32_t kind, UErrorCode& status)
{

    if (U_FAILURE(status)) {
        return NULL;
    }
    char lbType[kLBTypeLenMax];

    BreakIterator *result = NULL;
    switch (kind) {
    case UBRK_CHARACTER:
        result = BreakIterator::buildInstance(loc, "grapheme", kind, status);
        break;
    case UBRK_WORD:
        result = BreakIterator::buildInstance(loc, "word", kind, status);
        break;
    case UBRK_LINE:
        uprv_strcpy(lbType, "line");
        {
            char lbKeyValue[kLBTypeLenMax] = {0};
            UErrorCode kvStatus = U_ZERO_ERROR;
            int32_t kLen = loc.getKeywordValue("lb", lbKeyValue, kLBTypeLenMax, kvStatus);
            if (U_SUCCESS(kvStatus) && kLen > 0 && (uprv_strcmp(lbKeyValue,"strict")==0 || uprv_strcmp(lbKeyValue,"normal")==0 || uprv_strcmp(lbKeyValue,"loose")==0)) {
                uprv_strcat(lbType, "_");
                uprv_strcat(lbType, lbKeyValue);
            }
        }
        result = BreakIterator::buildInstance(loc, lbType, kind, status);
        break;
    case UBRK_SENTENCE:
        result = BreakIterator::buildInstance(loc, "sentence", kind, status);
        break;
    case UBRK_TITLE:
        result = BreakIterator::buildInstance(loc, "title", kind, status);
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (U_FAILURE(status)) {
        return NULL;
    }

    return result;
}

Locale
BreakIterator::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

const char *
BreakIterator::getLocaleID(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocaleID(type, status);
}





int32_t BreakIterator::getRuleStatus() const {
    return 0;
}




int32_t BreakIterator::getRuleStatusVec(int32_t *fillInVec, int32_t capacity, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    if (capacity < 1) {
        status = U_BUFFER_OVERFLOW_ERROR;
        return 1;
    }
    *fillInVec = 0;
    return 1;
}

BreakIterator::BreakIterator (const Locale& valid, const Locale& actual) {
  U_LOCALE_BASED(locBased, (*this));
  locBased.setLocaleIDs(valid, actual);
}

U_NAMESPACE_END

#endif 


