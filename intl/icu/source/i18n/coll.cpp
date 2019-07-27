







































#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "collationdata.h"
#include "collationroot.h"
#include "collationtailoring.h"
#include "ucol_imp.h"
#include "cstring.h"
#include "cmemory.h"
#include "umutex.h"
#include "servloc.h"
#include "uassert.h"
#include "ustrenum.h"
#include "uresimp.h"
#include "ucln_in.h"

static icu::Locale* availableLocaleList = NULL;
static int32_t  availableLocaleListCount;
static icu::ICULocaleService* gService = NULL;
static icu::UInitOnce gServiceInitOnce = U_INITONCE_INITIALIZER;
static icu::UInitOnce gAvailableLocaleListInitOnce;




U_CDECL_BEGIN
static UBool U_CALLCONV collator_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    if (gService) {
        delete gService;
        gService = NULL;
    }
    gServiceInitOnce.reset();
#endif
    if (availableLocaleList) {
        delete []availableLocaleList;
        availableLocaleList = NULL;
    }
    availableLocaleListCount = 0;
    gAvailableLocaleListInitOnce.reset();
    return TRUE;
}

U_CDECL_END

U_NAMESPACE_BEGIN

#if !UCONFIG_NO_SERVICE








CollatorFactory::~CollatorFactory() {}



UBool
CollatorFactory::visible(void) const {
    return TRUE;
}



UnicodeString& 
CollatorFactory::getDisplayName(const Locale& objectLocale, 
                                const Locale& displayLocale,
                                UnicodeString& result)
{
  return objectLocale.getDisplayName(displayLocale, result);
}



class ICUCollatorFactory : public ICUResourceBundleFactory {
 public:
    ICUCollatorFactory() : ICUResourceBundleFactory(UnicodeString(U_ICUDATA_COLL, -1, US_INV)) { }
    virtual ~ICUCollatorFactory();
 protected:
    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const;
};

ICUCollatorFactory::~ICUCollatorFactory() {}

UObject*
ICUCollatorFactory::create(const ICUServiceKey& key, const ICUService* , UErrorCode& status) const {
    if (handlesKey(key, status)) {
        const LocaleKey& lkey = (const LocaleKey&)key;
        Locale loc;
        
        
        
        lkey.canonicalLocale(loc);
        
        return Collator::makeInstance(loc, status);
    }
    return NULL;
}



class ICUCollatorService : public ICULocaleService {
public:
    ICUCollatorService()
        : ICULocaleService(UNICODE_STRING_SIMPLE("Collator"))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new ICUCollatorFactory(), status);
    }

    virtual ~ICUCollatorService();

    virtual UObject* cloneInstance(UObject* instance) const {
        return ((Collator*)instance)->clone();
    }
    
    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* actualID, UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        if (actualID) {
            
            
            
            actualID->truncate(0);
        }
        Locale loc("");
        lkey.canonicalLocale(loc);
        return Collator::makeInstance(loc, status);
    }
    
    virtual UObject* getKey(ICUServiceKey& key, UnicodeString* actualReturn, UErrorCode& status) const {
        UnicodeString ar;
        if (actualReturn == NULL) {
            actualReturn = &ar;
        }
        return (Collator*)ICULocaleService::getKey(key, actualReturn, status);
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

ICUCollatorService::~ICUCollatorService() {}



static void U_CALLCONV initService() {
    gService = new ICUCollatorService();
    ucln_i18n_registerCleanup(UCLN_I18N_COLLATOR, collator_cleanup);
}


static ICULocaleService* 
getService(void)
{
    umtx_initOnce(gServiceInitOnce, &initService);
    return gService;
}



static inline UBool
hasService(void) 
{
    UBool retVal = !gServiceInitOnce.isReset() && (getService() != NULL);
    return retVal;
}

#endif 

static void U_CALLCONV 
initAvailableLocaleList(UErrorCode &status) {
    U_ASSERT(availableLocaleListCount == 0);
    U_ASSERT(availableLocaleList == NULL);
    
    UResourceBundle *index = NULL;
    UResourceBundle installed;
    int32_t i = 0;
    
    ures_initStackObject(&installed);
    index = ures_openDirect(U_ICUDATA_COLL, "res_index", &status);
    ures_getByKey(index, "InstalledLocales", &installed, &status);
    
    if(U_SUCCESS(status)) {
        availableLocaleListCount = ures_getSize(&installed);
        availableLocaleList = new Locale[availableLocaleListCount];
        
        if (availableLocaleList != NULL) {
            ures_resetIterator(&installed);
            while(ures_hasNext(&installed)) {
                const char *tempKey = NULL;
                ures_getNextString(&installed, NULL, &tempKey, &status);
                availableLocaleList[i++] = Locale(tempKey);
            }
        }
        U_ASSERT(availableLocaleListCount == i);
        ures_close(&installed);
    }
    ures_close(index);
    ucln_i18n_registerCleanup(UCLN_I18N_COLLATOR, collator_cleanup);
}

static UBool isAvailableLocaleListInitialized(UErrorCode &status) {
    umtx_initOnce(gAvailableLocaleListInitOnce, &initAvailableLocaleList, status);
    return U_SUCCESS(status);
}




namespace {

static const struct {
    const char *name;
    UColAttribute attr;
} collAttributes[] = {
    { "colStrength", UCOL_STRENGTH },
    { "colBackwards", UCOL_FRENCH_COLLATION },
    { "colCaseLevel", UCOL_CASE_LEVEL },
    { "colCaseFirst", UCOL_CASE_FIRST },
    { "colAlternate", UCOL_ALTERNATE_HANDLING },
    { "colNormalization", UCOL_NORMALIZATION_MODE },
    { "colNumeric", UCOL_NUMERIC_COLLATION }
};

static const struct {
    const char *name;
    UColAttributeValue value;
} collAttributeValues[] = {
    { "primary", UCOL_PRIMARY },
    { "secondary", UCOL_SECONDARY },
    { "tertiary", UCOL_TERTIARY },
    { "quaternary", UCOL_QUATERNARY },
    
    { "identical", UCOL_IDENTICAL },
    { "no", UCOL_OFF },
    { "yes", UCOL_ON },
    { "shifted", UCOL_SHIFTED },
    { "non-ignorable", UCOL_NON_IGNORABLE },
    { "lower", UCOL_LOWER_FIRST },
    { "upper", UCOL_UPPER_FIRST }
};

static const char *collReorderCodes[UCOL_REORDER_CODE_LIMIT - UCOL_REORDER_CODE_FIRST] = {
    "space", "punct", "symbol", "currency", "digit"
};

int32_t getReorderCode(const char *s) {
    for (int32_t i = 0; i < UPRV_LENGTHOF(collReorderCodes); ++i) {
        if (uprv_stricmp(s, collReorderCodes[i]) == 0) {
            return UCOL_REORDER_CODE_FIRST + i;
        }
    }
    
    
    
    return -1;
}









void setAttributesFromKeywords(const Locale &loc, Collator &coll, UErrorCode &errorCode) {
    if (U_FAILURE(errorCode)) {
        return;
    }
    if (uprv_strcmp(loc.getName(), loc.getBaseName()) == 0) {
        
        return;
    }
    char value[1024];  
    
    
    int32_t length = loc.getKeywordValue("colHiraganaQuaternary", value, UPRV_LENGTHOF(value), errorCode);
    if (U_FAILURE(errorCode)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (length != 0) {
        errorCode = U_UNSUPPORTED_ERROR;
        return;
    }
    length = loc.getKeywordValue("variableTop", value, UPRV_LENGTHOF(value), errorCode);
    if (U_FAILURE(errorCode)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (length != 0) {
        errorCode = U_UNSUPPORTED_ERROR;
        return;
    }
    
    if (errorCode == U_STRING_NOT_TERMINATED_WARNING) {
        errorCode = U_ZERO_ERROR;
    }
    for (int32_t i = 0; i < UPRV_LENGTHOF(collAttributes); ++i) {
        length = loc.getKeywordValue(collAttributes[i].name, value, UPRV_LENGTHOF(value), errorCode);
        if (U_FAILURE(errorCode) || errorCode == U_STRING_NOT_TERMINATED_WARNING) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        if (length == 0) { continue; }
        for (int32_t j = 0;; ++j) {
            if (j == UPRV_LENGTHOF(collAttributeValues)) {
                errorCode = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            if (uprv_stricmp(value, collAttributeValues[j].name) == 0) {
                coll.setAttribute(collAttributes[i].attr, collAttributeValues[j].value, errorCode);
                break;
            }
        }
    }
    length = loc.getKeywordValue("colReorder", value, UPRV_LENGTHOF(value), errorCode);
    if (U_FAILURE(errorCode) || errorCode == U_STRING_NOT_TERMINATED_WARNING) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (length != 0) {
        int32_t codes[USCRIPT_CODE_LIMIT + UCOL_REORDER_CODE_LIMIT - UCOL_REORDER_CODE_FIRST];
        int32_t codesLength = 0;
        char *scriptName = value;
        for (;;) {
            if (codesLength == UPRV_LENGTHOF(codes)) {
                errorCode = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            char *limit = scriptName;
            char c;
            while ((c = *limit) != 0 && c != '-') { ++limit; }
            *limit = 0;
            int32_t code;
            if ((limit - scriptName) == 4) {
                
                code = u_getPropertyValueEnum(UCHAR_SCRIPT, scriptName);
            } else {
                code = getReorderCode(scriptName);
            }
            if (code < 0) {
                errorCode = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            codes[codesLength++] = code;
            if (c == 0) { break; }
            scriptName = limit + 1;
        }
        coll.setReorderCodes(codes, codesLength, errorCode);
    }
    length = loc.getKeywordValue("kv", value, UPRV_LENGTHOF(value), errorCode);
    if (U_FAILURE(errorCode) || errorCode == U_STRING_NOT_TERMINATED_WARNING) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (length != 0) {
        int32_t code = getReorderCode(value);
        if (code < 0) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        coll.setMaxVariable((UColReorderCode)code, errorCode);
    }
    if (U_FAILURE(errorCode)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

}  

Collator* U_EXPORT2 Collator::createInstance(UErrorCode& success) 
{
    return createInstance(Locale::getDefault(), success);
}

Collator* U_EXPORT2 Collator::createInstance(const Locale& desiredLocale,
                                   UErrorCode& status)
{
    if (U_FAILURE(status)) 
        return 0;
    if (desiredLocale.isBogus()) {
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    Collator* coll;
#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        Locale actualLoc;
        coll = (Collator*)gService->get(desiredLocale, &actualLoc, status);
    } else
#endif
    {
        coll = makeInstance(desiredLocale, status);
    }
    setAttributesFromKeywords(desiredLocale, *coll, status);
    if (U_FAILURE(status)) {
        delete coll;
        return NULL;
    }
    return coll;
}


Collator* Collator::makeInstance(const Locale&  desiredLocale, UErrorCode& status) {
    const CollationCacheEntry *entry = CollationLoader::loadTailoring(desiredLocale, status);
    if (U_SUCCESS(status)) {
        Collator *result = new RuleBasedCollator(entry);
        if (result != NULL) {
            
            
            entry->removeRef();
            return result;
        }
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (entry != NULL) {
        
        entry->removeRef();
    }
    return NULL;
}

Collator *
Collator::safeClone() const {
    return clone();
}


Collator::EComparisonResult Collator::compare(const UnicodeString& source, 
                                    const UnicodeString& target) const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (EComparisonResult)compare(source, target, ec);
}


Collator::EComparisonResult Collator::compare(const UnicodeString& source,
                                    const UnicodeString& target,
                                    int32_t length) const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (EComparisonResult)compare(source, target, length, ec);
}


Collator::EComparisonResult Collator::compare(const UChar* source, int32_t sourceLength,
                                    const UChar* target, int32_t targetLength) 
                                    const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (EComparisonResult)compare(source, sourceLength, target, targetLength, ec);
}

UCollationResult Collator::compare(UCharIterator &,
                                   UCharIterator &,
                                   UErrorCode &status) const {
    if(U_SUCCESS(status)) {
        
        status = U_UNSUPPORTED_ERROR;
    }
    return UCOL_EQUAL;
}

UCollationResult Collator::compareUTF8(const StringPiece &source,
                                       const StringPiece &target,
                                       UErrorCode &status) const {
    if(U_FAILURE(status)) {
        return UCOL_EQUAL;
    }
    UCharIterator sIter, tIter;
    uiter_setUTF8(&sIter, source.data(), source.length());
    uiter_setUTF8(&tIter, target.data(), target.length());
    return compare(sIter, tIter, status);
}

UBool Collator::equals(const UnicodeString& source, 
                       const UnicodeString& target) const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (compare(source, target, ec) == UCOL_EQUAL);
}

UBool Collator::greaterOrEqual(const UnicodeString& source, 
                               const UnicodeString& target) const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (compare(source, target, ec) != UCOL_LESS);
}

UBool Collator::greater(const UnicodeString& source, 
                        const UnicodeString& target) const
{
    UErrorCode ec = U_ZERO_ERROR;
    return (compare(source, target, ec) == UCOL_GREATER);
}



const Locale* U_EXPORT2 Collator::getAvailableLocales(int32_t& count) 
{
    UErrorCode status = U_ZERO_ERROR;
    Locale *result = NULL;
    count = 0;
    if (isAvailableLocaleListInitialized(status))
    {
        result = availableLocaleList;
        count = availableLocaleListCount;
    }
    return result;
}

UnicodeString& U_EXPORT2 Collator::getDisplayName(const Locale& objectLocale,
                                        const Locale& displayLocale,
                                        UnicodeString& name)
{
#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        UnicodeString locNameStr;
        LocaleUtility::initNameFromLocale(objectLocale, locNameStr);
        return gService->getDisplayName(locNameStr, name, displayLocale);
    }
#endif
    return objectLocale.getDisplayName(displayLocale, name);
}

UnicodeString& U_EXPORT2 Collator::getDisplayName(const Locale& objectLocale,
                                        UnicodeString& name)
{   
    return getDisplayName(objectLocale, Locale::getDefault(), name);
}

















Collator::Collator()
: UObject()
{
}











Collator::Collator(UCollationStrength, UNormalizationMode )
: UObject()
{
}

Collator::~Collator()
{
}

Collator::Collator(const Collator &other)
    : UObject(other)
{
}

UBool Collator::operator==(const Collator& other) const
{
    
    return typeid(*this) == typeid(other);
}

UBool Collator::operator!=(const Collator& other) const
{
    return (UBool)!(*this == other);
}

int32_t U_EXPORT2 Collator::getBound(const uint8_t       *source,
                           int32_t             sourceLength,
                           UColBoundMode       boundType,
                           uint32_t            noOfLevels,
                           uint8_t             *result,
                           int32_t             resultLength,
                           UErrorCode          &status)
{
    return ucol_getBound(source, sourceLength, boundType, noOfLevels, result, resultLength, &status);
}

void
Collator::setLocales(const Locale& , const Locale& , const Locale& ) {
}

UnicodeSet *Collator::getTailoredSet(UErrorCode &status) const
{
    if(U_FAILURE(status)) {
        return NULL;
    }
    
    return new UnicodeSet(0, 0x10FFFF);
}



#if !UCONFIG_NO_SERVICE
URegistryKey U_EXPORT2
Collator::registerInstance(Collator* toAdopt, const Locale& locale, UErrorCode& status) 
{
    if (U_SUCCESS(status)) {
        
        
        
        toAdopt->setLocales(locale, locale, locale);
        return getService()->registerInstance(toAdopt, locale, status);
    }
    return NULL;
}



class CFactory : public LocaleKeyFactory {
private:
    CollatorFactory* _delegate;
    Hashtable* _ids;
    
public:
    CFactory(CollatorFactory* delegate, UErrorCode& status) 
        : LocaleKeyFactory(delegate->visible() ? VISIBLE : INVISIBLE)
        , _delegate(delegate)
        , _ids(NULL)
    {
        if (U_SUCCESS(status)) {
            int32_t count = 0;
            _ids = new Hashtable(status);
            if (_ids) {
                const UnicodeString * idlist = _delegate->getSupportedIDs(count, status);
                for (int i = 0; i < count; ++i) {
                    _ids->put(idlist[i], (void*)this, status);
                    if (U_FAILURE(status)) {
                        delete _ids;
                        _ids = NULL;
                        return;
                    }
                }
            } else {
                status = U_MEMORY_ALLOCATION_ERROR;
            }
        }
    }

    virtual ~CFactory();

    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const;
    
protected:
    virtual const Hashtable* getSupportedIDs(UErrorCode& status) const
    {
        if (U_SUCCESS(status)) {
            return _ids;
        }
        return NULL;
    }
    
    virtual UnicodeString&
        getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const;
};

CFactory::~CFactory()
{
    delete _delegate;
    delete _ids;
}

UObject* 
CFactory::create(const ICUServiceKey& key, const ICUService* , UErrorCode& status) const
{
    if (handlesKey(key, status)) {
        const LocaleKey& lkey = (const LocaleKey&)key;
        Locale validLoc;
        lkey.currentLocale(validLoc);
        return _delegate->createCollator(validLoc);
    }
    return NULL;
}

UnicodeString&
CFactory::getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const 
{
    if ((_coverage & 0x1) == 0) {
        UErrorCode status = U_ZERO_ERROR;
        const Hashtable* ids = getSupportedIDs(status);
        if (ids && (ids->get(id) != NULL)) {
            Locale loc;
            LocaleUtility::initLocaleFromName(id, loc);
            return _delegate->getDisplayName(loc, locale, result);
        }
    }
    result.setToBogus();
    return result;
}

URegistryKey U_EXPORT2
Collator::registerFactory(CollatorFactory* toAdopt, UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        CFactory* f = new CFactory(toAdopt, status);
        if (f) {
            return getService()->registerFactory(f, status);
        }
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return NULL;
}



UBool U_EXPORT2
Collator::unregister(URegistryKey key, UErrorCode& status) 
{
    if (U_SUCCESS(status)) {
        if (hasService()) {
            return gService->unregister(key, status);
        }
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return FALSE;
}
#endif 

class CollationLocaleListEnumeration : public StringEnumeration {
private:
    int32_t index;
public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
public:
    CollationLocaleListEnumeration()
        : index(0)
    {
        
        
    }

    virtual ~CollationLocaleListEnumeration();

    virtual StringEnumeration * clone() const
    {
        CollationLocaleListEnumeration *result = new CollationLocaleListEnumeration();
        if (result) {
            result->index = index;
        }
        return result;
    }

    virtual int32_t count(UErrorCode &) const {
        return availableLocaleListCount;
    }

    virtual const char* next(int32_t* resultLength, UErrorCode& ) {
        const char* result;
        if(index < availableLocaleListCount) {
            result = availableLocaleList[index++].getName();
            if(resultLength != NULL) {
                *resultLength = (int32_t)uprv_strlen(result);
            }
        } else {
            if(resultLength != NULL) {
                *resultLength = 0;
            }
            result = NULL;
        }
        return result;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        int32_t resultLength = 0;
        const char *s = next(&resultLength, status);
        return setChars(s, resultLength, status);
    }

    virtual void reset(UErrorCode& ) {
        index = 0;
    }
};

CollationLocaleListEnumeration::~CollationLocaleListEnumeration() {}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollationLocaleListEnumeration)




StringEnumeration* U_EXPORT2
Collator::getAvailableLocales(void)
{
#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        return getService()->getAvailableLocales();
    }
#endif 
    UErrorCode status = U_ZERO_ERROR;
    if (isAvailableLocaleListInitialized(status)) {
        return new CollationLocaleListEnumeration();
    }
    return NULL;
}

StringEnumeration* U_EXPORT2
Collator::getKeywords(UErrorCode& status) {
    return UStringEnumeration::fromUEnumeration(
            ucol_getKeywords(&status), status);
}

StringEnumeration* U_EXPORT2
Collator::getKeywordValues(const char *keyword, UErrorCode& status) {
    return UStringEnumeration::fromUEnumeration(
            ucol_getKeywordValues(keyword, &status), status);
}

StringEnumeration* U_EXPORT2
Collator::getKeywordValuesForLocale(const char* key, const Locale& locale,
                                    UBool commonlyUsed, UErrorCode& status) {
    return UStringEnumeration::fromUEnumeration(
            ucol_getKeywordValuesForLocale(
                    key, locale.getName(), commonlyUsed, &status),
            status);
}

Locale U_EXPORT2
Collator::getFunctionalEquivalent(const char* keyword, const Locale& locale,
                                  UBool& isAvailable, UErrorCode& status) {
    
    char loc[ULOC_FULLNAME_CAPACITY];
     ucol_getFunctionalEquivalent(loc, sizeof(loc),
                    keyword, locale.getName(), &isAvailable, &status);
    if (U_FAILURE(status)) {
        *loc = 0; 
    }
    return Locale::createFromName(loc);
}

Collator::ECollationStrength
Collator::getStrength(void) const {
    UErrorCode intStatus = U_ZERO_ERROR;
    return (ECollationStrength)getAttribute(UCOL_STRENGTH, intStatus);
}

void
Collator::setStrength(ECollationStrength newStrength) {
    UErrorCode intStatus = U_ZERO_ERROR;
    setAttribute(UCOL_STRENGTH, (UColAttributeValue)newStrength, intStatus);
}

Collator &
Collator::setMaxVariable(UColReorderCode , UErrorCode &errorCode) {
    if (U_SUCCESS(errorCode)) {
        errorCode = U_UNSUPPORTED_ERROR;
    }
    return *this;
}

UColReorderCode
Collator::getMaxVariable() const {
    return UCOL_REORDER_CODE_PUNCTUATION;
}

int32_t
Collator::getReorderCodes(int32_t* ,
                          int32_t ,
                          UErrorCode& status) const
{
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return 0;
}

void
Collator::setReorderCodes(const int32_t* ,
                          int32_t ,
                          UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
}

int32_t
Collator::getEquivalentReorderCodes(int32_t reorderCode,
                                    int32_t *dest, int32_t capacity,
                                    UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    if(capacity < 0 || (dest == NULL && capacity > 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    const CollationData *baseData = CollationRoot::getData(errorCode);
    if(U_FAILURE(errorCode)) { return 0; }
    return baseData->getEquivalentScripts(reorderCode, dest, capacity, errorCode);
}

int32_t
Collator::internalGetShortDefinitionString(const char * ,
                                                             char * ,
                                                             int32_t ,
                                                             UErrorCode &status) const {
  if(U_SUCCESS(status)) {
    status = U_UNSUPPORTED_ERROR; 
  }
  return 0;
}

UCollationResult
Collator::internalCompareUTF8(const char *left, int32_t leftLength,
                              const char *right, int32_t rightLength,
                              UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return UCOL_EQUAL; }
    if((left == NULL && leftLength != 0) || (right == NULL && rightLength != 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return UCOL_EQUAL;
    }
    return compareUTF8(
            StringPiece(left, (leftLength < 0) ? uprv_strlen(left) : leftLength),
            StringPiece(right, (rightLength < 0) ? uprv_strlen(right) : rightLength),
            errorCode);
}

int32_t
Collator::internalNextSortKeyPart(UCharIterator * , uint32_t [2],
                                  uint8_t * , int32_t , UErrorCode &errorCode) const {
    if (U_SUCCESS(errorCode)) {
        errorCode = U_UNSUPPORTED_ERROR;
    }
    return 0;
}








U_NAMESPACE_END

#endif 


