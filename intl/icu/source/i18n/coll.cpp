






































#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "ucol_imp.h"
#include "cstring.h"
#include "cmemory.h"
#include "umutex.h"
#include "servloc.h"
#include "ustrenum.h"
#include "uresimp.h"
#include "ucln_in.h"

static icu::Locale* availableLocaleList = NULL;
static int32_t  availableLocaleListCount;
static icu::ICULocaleService* gService = NULL;




U_CDECL_BEGIN
static UBool U_CALLCONV collator_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    if (gService) {
        delete gService;
        gService = NULL;
    }
#endif
    if (availableLocaleList) {
        delete []availableLocaleList;
        availableLocaleList = NULL;
    }
    availableLocaleListCount = 0;

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
        Collator* result = (Collator*)ICULocaleService::getKey(key, actualReturn, status);
        
        
        
        
        
        
        if (result && actualReturn->length() > 0) {
            const LocaleKey& lkey = (const LocaleKey&)key;
            Locale canonicalLocale("");
            Locale currentLocale("");
            
            LocaleUtility::initLocaleFromName(*actualReturn, currentLocale);
            result->setLocales(lkey.canonicalLocale(canonicalLocale), currentLocale, currentLocale);
        }
        return result;
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

ICUCollatorService::~ICUCollatorService() {}



static ICULocaleService* 
getService(void)
{
    UBool needInit;
    UMTX_CHECK(NULL, (UBool)(gService == NULL), needInit);
    if(needInit) {
        ICULocaleService *newservice = new ICUCollatorService();
        if(newservice) {
            umtx_lock(NULL);
            if(gService == NULL) {
                gService = newservice;
                newservice = NULL;
            }
            umtx_unlock(NULL);
        }
        if(newservice) {
            delete newservice;
        }
        else {
            ucln_i18n_registerCleanup(UCLN_I18N_COLLATOR, collator_cleanup);
        }
    }
    return gService;
}



static inline UBool
hasService(void) 
{
    UBool retVal;
    UMTX_CHECK(NULL, gService != NULL, retVal);
    return retVal;
}



UCollator* 
Collator::createUCollator(const char *loc,
                          UErrorCode *status)
{
    UCollator *result = 0;
    if (status && U_SUCCESS(*status) && hasService()) {
        Locale desiredLocale(loc);
        Collator *col = (Collator*)gService->get(desiredLocale, *status);
        RuleBasedCollator *rbc;
        if (col && (rbc = dynamic_cast<RuleBasedCollator *>(col))) {
            if (!rbc->dataIsOwned) {
                result = ucol_safeClone(rbc->ucollator, NULL, NULL, status);
            } else {
                result = rbc->ucollator;
                rbc->ucollator = NULL; 
            }
        } else {
          
          result = (UCollator *)uprv_malloc(sizeof(UCollator));
          if(result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
          } else {
            uprv_memset(result, 0, sizeof(UCollator));
            result->delegate = col;
            result->freeOnClose = TRUE; 
            col = NULL; 
          }
        }
        delete col;
    }
    return result;
}
#endif 

static UBool isAvailableLocaleListInitialized(UErrorCode &status) {
    
    UBool needInit;
    UMTX_CHECK(NULL, availableLocaleList == NULL, needInit);

    if (needInit) {
        UResourceBundle *index = NULL;
        UResourceBundle installed;
        Locale * temp;
        int32_t i = 0;
        int32_t localeCount;
        
        ures_initStackObject(&installed);
        index = ures_openDirect(U_ICUDATA_COLL, "res_index", &status);
        ures_getByKey(index, "InstalledLocales", &installed, &status);
        
        if(U_SUCCESS(status)) {
            localeCount = ures_getSize(&installed);
            temp = new Locale[localeCount];
            
            if (temp != NULL) {
                ures_resetIterator(&installed);
                while(ures_hasNext(&installed)) {
                    const char *tempKey = NULL;
                    ures_getNextString(&installed, NULL, &tempKey, &status);
                    temp[i++] = Locale(tempKey);
                }
                
                umtx_lock(NULL);
                if (availableLocaleList == NULL)
                {
                    availableLocaleListCount = localeCount;
                    availableLocaleList = temp;
                    temp = NULL;
                    ucln_i18n_registerCleanup(UCLN_I18N_COLLATOR, collator_cleanup);
                } 
                umtx_unlock(NULL);

                needInit = FALSE;
                if (temp) {
                    delete []temp;
                }
            }

            ures_close(&installed);
        }
        ures_close(index);
    }
    return !needInit;
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
    
#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        Locale actualLoc;
        Collator *result =
            (Collator*)gService->get(desiredLocale, &actualLoc, status);

        
        
        
        
        
        
        if (*actualLoc.getName() != 0) {
            result->setLocales(desiredLocale, actualLoc, actualLoc);
        }
        return result;
    }
#endif
    return makeInstance(desiredLocale, status);
}


Collator* Collator::makeInstance(const Locale&  desiredLocale, 
                                         UErrorCode& status)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    RuleBasedCollator* collation = new RuleBasedCollator(desiredLocale, 
        status);
    
    if (collation == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    if (U_FAILURE(status))
    {
        delete collation;
        collation = 0;
    }
    return collation;
}

#ifdef U_USE_COLLATION_OBSOLETE_2_6


Collator *
Collator::createInstance(const Locale &loc,
                         UVersionInfo version,
                         UErrorCode &status)
{
    Collator *collator;
    UVersionInfo info;
    
    collator=new RuleBasedCollator(loc, status);
    
    if (collator == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    
    if(U_SUCCESS(status)) {
        collator->getVersion(info);
        if(0!=uprv_memcmp(version, info, sizeof(UVersionInfo))) {
            delete collator;
            status=U_MISSING_RESOURCE_ERROR;
            return 0;
        }
    }
    return collator;
}
#endif

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
    
    UEnumeration* uenum = ucol_getKeywords(&status);
    if (U_FAILURE(status)) {
        uenum_close(uenum);
        return NULL;
    }
    return new UStringEnumeration(uenum);
}

StringEnumeration* U_EXPORT2
Collator::getKeywordValues(const char *keyword, UErrorCode& status) {
    
    UEnumeration* uenum = ucol_getKeywordValues(keyword, &status);
    if (U_FAILURE(status)) {
        uenum_close(uenum);
        return NULL;
    }
    return new UStringEnumeration(uenum);
}

StringEnumeration* U_EXPORT2
Collator::getKeywordValuesForLocale(const char* key, const Locale& locale,
                                    UBool commonlyUsed, UErrorCode& status) {
    
    UEnumeration *uenum = ucol_getKeywordValuesForLocale(key, locale.getName(),
                                                        commonlyUsed, &status);
    if (U_FAILURE(status)) {
        uenum_close(uenum);
        return NULL;
    }
    return new UStringEnumeration(uenum);
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

int32_t U_EXPORT2
Collator::getEquivalentReorderCodes(int32_t ,
                                    int32_t* ,
                                    int32_t ,
                                    UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return 0;
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








U_NAMESPACE_END

#endif 


