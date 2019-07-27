






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "zonemeta.h"

#include "unicode/timezone.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"
#include "unicode/simpletz.h"

#include "umutex.h"
#include "uvector.h"
#include "cmemory.h"
#include "gregoimp.h"
#include "cstring.h"
#include "ucln_in.h"
#include "uassert.h"
#include "uresimp.h"
#include "uhash.h"
#include "olsontz.h"

static UMutex gZoneMetaLock = U_MUTEX_INITIALIZER;


static UHashtable *gCanonicalIDCache = NULL;
static icu::UInitOnce gCanonicalIDCacheInitOnce = U_INITONCE_INITIALIZER;


static UHashtable *gOlsonToMeta = NULL;
static icu::UInitOnce gOlsonToMetaInitOnce = U_INITONCE_INITIALIZER;


static icu::UVector *gMetaZoneIDs = NULL;
static UHashtable *gMetaZoneIDTable = NULL;
static icu::UInitOnce gMetaZoneIDsInitOnce = U_INITONCE_INITIALIZER;


static icu::UVector *gSingleZoneCountries = NULL;
static icu::UVector *gMultiZonesCountries = NULL;
static icu::UInitOnce gCountryInfoVectorsInitOnce = U_INITONCE_INITIALIZER;

U_CDECL_BEGIN




static UBool U_CALLCONV zoneMeta_cleanup(void)
{
    if (gCanonicalIDCache != NULL) {
        uhash_close(gCanonicalIDCache);
        gCanonicalIDCache = NULL;
    }
    gCanonicalIDCacheInitOnce.reset();

    if (gOlsonToMeta != NULL) {
        uhash_close(gOlsonToMeta);
        gOlsonToMeta = NULL;
    }
    gOlsonToMetaInitOnce.reset();

    if (gMetaZoneIDTable != NULL) {
        uhash_close(gMetaZoneIDTable);
        gMetaZoneIDTable = NULL;
    }
    
    
    delete gMetaZoneIDs;
    gMetaZoneIDs = NULL;
    gMetaZoneIDsInitOnce.reset();

    delete gSingleZoneCountries;
    gSingleZoneCountries = NULL;
    delete gMultiZonesCountries;
    gMultiZonesCountries = NULL;
    gCountryInfoVectorsInitOnce.reset();

    return TRUE;
}




static void U_CALLCONV
deleteUCharString(void *obj) {
    UChar *entry = (UChar*)obj;
    uprv_free(entry);
}




static void U_CALLCONV
deleteUVector(void *obj) {
   delete (icu::UVector*) obj;
}




static void U_CALLCONV
deleteOlsonToMetaMappingEntry(void *obj) {
    icu::OlsonToMetaMappingEntry *entry = (icu::OlsonToMetaMappingEntry*)obj;
    uprv_free(entry);
}

U_CDECL_END

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX 128

static const char gMetaZones[]          = "metaZones";
static const char gMetazoneInfo[]       = "metazoneInfo";
static const char gMapTimezonesTag[]    = "mapTimezones";

static const char gKeyTypeData[]        = "keyTypeData";
static const char gTypeAliasTag[]       = "typeAlias";
static const char gTypeMapTag[]         = "typeMap";
static const char gTimezoneTag[]        = "timezone";

static const char gPrimaryZonesTag[]    = "primaryZones";

static const char gWorldTag[]           = "001";

static const UChar gWorld[] = {0x30, 0x30, 0x31, 0x00}; 

static const UChar gDefaultFrom[] = {0x31, 0x39, 0x37, 0x30, 0x2D, 0x30, 0x31, 0x2D, 0x30, 0x31,
                                     0x20, 0x30, 0x30, 0x3A, 0x30, 0x30, 0x00}; 
static const UChar gDefaultTo[]   = {0x39, 0x39, 0x39, 0x39, 0x2D, 0x31, 0x32, 0x2D, 0x33, 0x31,
                                     0x20, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x00}; 

static const UChar gCustomTzPrefix[]    = {0x47, 0x4D, 0x54, 0};    

#define ASCII_DIGIT(c) (((c)>=0x30 && (c)<=0x39) ? (c)-0x30 : -1)





static UDate
parseDate (const UChar *text, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    int32_t len = u_strlen(text);
    if (len != 16 && len != 10) {
        
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }

    int32_t year = 0, month = 0, day = 0, hour = 0, min = 0, n;
    int32_t idx;

    
    for (idx = 0; idx <= 3 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            year = 10*year + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    
    for (idx = 5; idx <= 6 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            month = 10*month + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    
    for (idx = 8; idx <= 9 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            day = 10*day + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    if (len == 16) {
        
        for (idx = 11; idx <= 12 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT((int32_t)text[idx]);
            if (n >= 0) {
                hour = 10*hour + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
        
        for (idx = 14; idx <= 15 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT((int32_t)text[idx]);
            if (n >= 0) {
                min = 10*min + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
    }

    if (U_SUCCESS(status)) {
        UDate date = Grego::fieldsToDay(year, month - 1, day) * U_MILLIS_PER_DAY
            + hour * U_MILLIS_PER_HOUR + min * U_MILLIS_PER_MINUTE;
        return date;
    }
    return 0;
}

static void U_CALLCONV initCanonicalIDCache(UErrorCode &status) {
    gCanonicalIDCache = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL, &status);
    if (gCanonicalIDCache == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        gCanonicalIDCache = NULL;
    }
    
    ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
}


const UChar* U_EXPORT2
ZoneMeta::getCanonicalCLDRID(const UnicodeString &tzid, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }

    if (tzid.isBogus() || tzid.length() > ZID_KEY_MAX) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    
    umtx_initOnce(gCanonicalIDCacheInitOnce, &initCanonicalIDCache, status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    const UChar *canonicalID = NULL;

    UErrorCode tmpStatus = U_ZERO_ERROR;
    UChar utzid[ZID_KEY_MAX + 1];
    tzid.extract(utzid, ZID_KEY_MAX + 1, tmpStatus);
    U_ASSERT(tmpStatus == U_ZERO_ERROR);    

    
    umtx_lock(&gZoneMetaLock);
    {
        canonicalID = (const UChar *)uhash_get(gCanonicalIDCache, utzid);
    }
    umtx_unlock(&gZoneMetaLock);

    if (canonicalID != NULL) {
        return canonicalID;
    }

    
    UBool isInputCanonical = FALSE;
    char id[ZID_KEY_MAX + 1];
    tzid.extract(0, 0x7fffffff, id, UPRV_LENGTHOF(id), US_INV);

    
    char *p = id;
    while (*p++) {
        if (*p == '/') {
            *p = ':';
        }
    }

    UResourceBundle *top = ures_openDirect(NULL, gKeyTypeData, &tmpStatus);
    UResourceBundle *rb = ures_getByKey(top, gTypeMapTag, NULL, &tmpStatus);
    ures_getByKey(rb, gTimezoneTag, rb, &tmpStatus);
    ures_getByKey(rb, id, rb, &tmpStatus);
    if (U_SUCCESS(tmpStatus)) {
        
        
        canonicalID = TimeZone::findID(tzid);
        isInputCanonical = TRUE;
    }

    if (canonicalID == NULL) {
        
        tmpStatus = U_ZERO_ERROR;
        ures_getByKey(top, gTypeAliasTag, rb, &tmpStatus);
        ures_getByKey(rb, gTimezoneTag, rb, &tmpStatus);
        const UChar *canonical = ures_getStringByKey(rb,id,NULL,&tmpStatus);
        if (U_SUCCESS(tmpStatus)) {
            
            canonicalID = canonical;
        }

        if (canonicalID == NULL) {
            
            const UChar *derefer = TimeZone::dereferOlsonLink(tzid);
            if (derefer == NULL) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
            } else {
                int32_t len = u_strlen(derefer);
                u_UCharsToChars(derefer,id,len);
                id[len] = (char) 0; 

                
                char *p = id;
                while (*p++) {
                    if (*p == '/') {
                        *p = ':';
                    }
                }

                
                
                
                tmpStatus = U_ZERO_ERROR;
                canonical = ures_getStringByKey(rb,id,NULL,&tmpStatus);
                if (U_SUCCESS(tmpStatus)) {
                    
                    canonicalID = canonical;
                } else {
                    canonicalID = derefer;
                    isInputCanonical = TRUE;
                }
            }
        }
    }
    ures_close(rb);
    ures_close(top);

    if (U_SUCCESS(status)) {
        U_ASSERT(canonicalID != NULL);  

        
        umtx_lock(&gZoneMetaLock);
        {
            const UChar* idInCache = (const UChar *)uhash_get(gCanonicalIDCache, utzid);
            if (idInCache == NULL) {
                const UChar* key = ZoneMeta::findTimeZoneID(tzid);
                U_ASSERT(key != NULL);
                if (key != NULL) {
                    idInCache = (const UChar *)uhash_put(gCanonicalIDCache, (void *)key, (void *)canonicalID, &status);
                    U_ASSERT(idInCache == NULL);
                }
            }
            if (U_SUCCESS(status) && isInputCanonical) {
                
                const UChar *canonicalInCache = (const UChar*)uhash_get(gCanonicalIDCache, canonicalID);
                if (canonicalInCache == NULL) {
                    canonicalInCache = (const UChar *)uhash_put(gCanonicalIDCache, (void *)canonicalID, (void *)canonicalID, &status);
                    U_ASSERT(canonicalInCache == NULL);
                }
            }
        }
        umtx_unlock(&gZoneMetaLock);
    }

    return canonicalID;
}

UnicodeString& U_EXPORT2
ZoneMeta::getCanonicalCLDRID(const UnicodeString &tzid, UnicodeString &systemID, UErrorCode& status) {
    const UChar *canonicalID = getCanonicalCLDRID(tzid, status);
    if (U_FAILURE(status) || canonicalID == NULL) {
        systemID.setToBogus();
        return systemID;
    }
    systemID.setTo(TRUE, canonicalID, -1);
    return systemID;
}

const UChar* U_EXPORT2
ZoneMeta::getCanonicalCLDRID(const TimeZone& tz) {
    if (dynamic_cast<const OlsonTimeZone *>(&tz) != NULL) {
        
        const OlsonTimeZone *otz = (const OlsonTimeZone*)&tz;
        return otz->getCanonicalID();
    }
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString tzID;
    return getCanonicalCLDRID(tz.getID(tzID), status);
}

static void U_CALLCONV countryInfoVectorsInit(UErrorCode &status) {
    
    
    gSingleZoneCountries = new UVector(NULL, uhash_compareUChars, status);
    if (gSingleZoneCountries == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    gMultiZonesCountries = new UVector(NULL, uhash_compareUChars, status);
    if (gMultiZonesCountries == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

    if (U_FAILURE(status)) {
        delete gSingleZoneCountries;
        delete gMultiZonesCountries;
        gSingleZoneCountries = NULL;
        gMultiZonesCountries  = NULL;
    }
    ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
}


UnicodeString& U_EXPORT2
ZoneMeta::getCanonicalCountry(const UnicodeString &tzid, UnicodeString &country, UBool *isPrimary ) {
    if (isPrimary != NULL) {
        *isPrimary = FALSE;
    }

    const UChar *region = TimeZone::getRegion(tzid);
    if (region != NULL && u_strcmp(gWorld, region) != 0) {
        country.setTo(region, -1);
    } else {
        country.setToBogus();
        return country;
    }

    if (isPrimary != NULL) {
        char regionBuf[] = {0, 0, 0};

        
        UErrorCode status = U_ZERO_ERROR;
        umtx_initOnce(gCountryInfoVectorsInitOnce, &countryInfoVectorsInit, status);
        if (U_FAILURE(status)) {
            return country;
        }

        
        UBool cached = FALSE;
        UBool singleZone = FALSE;
        umtx_lock(&gZoneMetaLock);
        {
            singleZone = cached = gSingleZoneCountries->contains((void*)region);
            if (!cached) {
                cached = gMultiZonesCountries->contains((void*)region);
            }
        }
        umtx_unlock(&gZoneMetaLock);

        if (!cached) {
            
            

            U_ASSERT(u_strlen(region) == 2);

            u_UCharsToChars(region, regionBuf, 2);

            StringEnumeration *ids = TimeZone::createTimeZoneIDEnumeration(UCAL_ZONE_TYPE_CANONICAL_LOCATION, regionBuf, NULL, status);
            int32_t idsLen = ids->count(status);
            if (U_SUCCESS(status) && idsLen == 1) {
                
                singleZone = TRUE;
            }
            delete ids;

            
            umtx_lock(&gZoneMetaLock);
            {
                UErrorCode ec = U_ZERO_ERROR;
                if (singleZone) {
                    if (!gSingleZoneCountries->contains((void*)region)) {
                        gSingleZoneCountries->addElement((void*)region, ec);
                    }
                } else {
                    if (!gMultiZonesCountries->contains((void*)region)) {
                        gMultiZonesCountries->addElement((void*)region, ec);
                    }
                }
            }
            umtx_unlock(&gZoneMetaLock);
        }

        if (singleZone) {
            *isPrimary = TRUE;
        } else {
            

            
            
            int32_t idLen = 0;
            if (regionBuf[0] == 0) {
                u_UCharsToChars(region, regionBuf, 2);
            }

            UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
            ures_getByKey(rb, gPrimaryZonesTag, rb, &status);
            const UChar *primaryZone = ures_getStringByKey(rb, regionBuf, &idLen, &status);
            if (U_SUCCESS(status)) {
                if (tzid.compare(primaryZone, idLen) == 0) {
                    *isPrimary = TRUE;
                } else {
                    
                    UnicodeString canonicalID;
                    TimeZone::getCanonicalID(tzid, canonicalID, status);
                    if (U_SUCCESS(status) && canonicalID.compare(primaryZone, idLen) == 0) {
                        *isPrimary = TRUE;
                    }
                }
            }
            ures_close(rb);
        }
    }

    return country;
}

UnicodeString& U_EXPORT2
ZoneMeta::getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result) {
    UBool isSet = FALSE;
    const UVector *mappings = getMetazoneMappings(tzid);
    if (mappings != NULL) {
        for (int32_t i = 0; i < mappings->size(); i++) {
            OlsonToMetaMappingEntry *mzm = (OlsonToMetaMappingEntry*)mappings->elementAt(i);
            if (mzm->from <= date && mzm->to > date) {
                result.setTo(mzm->mzid, -1);
                isSet = TRUE;
                break;
            }
        }
    }
    if (!isSet) {
        result.setToBogus();
    }
    return result;
}

static void U_CALLCONV olsonToMetaInit(UErrorCode &status) {
    U_ASSERT(gOlsonToMeta == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
    gOlsonToMeta = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL, &status);
    if (U_FAILURE(status)) {
        gOlsonToMeta = NULL;
    } else {
        uhash_setKeyDeleter(gOlsonToMeta, deleteUCharString);
        uhash_setValueDeleter(gOlsonToMeta, deleteUVector);
    }
}


const UVector* U_EXPORT2
ZoneMeta::getMetazoneMappings(const UnicodeString &tzid) {
    UErrorCode status = U_ZERO_ERROR;
    UChar tzidUChars[ZID_KEY_MAX + 1];
    tzid.extract(tzidUChars, ZID_KEY_MAX + 1, status);
    if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
        return NULL;
    }

    umtx_initOnce(gOlsonToMetaInitOnce, &olsonToMetaInit, status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    
    const UVector *result = NULL;

    umtx_lock(&gZoneMetaLock);
    {
        result = (UVector*) uhash_get(gOlsonToMeta, tzidUChars);
    }
    umtx_unlock(&gZoneMetaLock);

    if (result != NULL) {
        return result;
    }

    
    UVector *tmpResult = createMetazoneMappings(tzid);
    if (tmpResult == NULL) {
        
        return NULL;
    }

    
    umtx_lock(&gZoneMetaLock);
    {
        
        result = (UVector*) uhash_get(gOlsonToMeta, tzidUChars);
        if (result == NULL) {
            
            int32_t tzidLen = tzid.length() + 1;
            UChar *key = (UChar*)uprv_malloc(tzidLen * sizeof(UChar));
            if (key == NULL) {
                
                result = NULL;
                delete tmpResult;
            } else {
                tzid.extract(key, tzidLen, status);
                uhash_put(gOlsonToMeta, key, tmpResult, &status);
                if (U_FAILURE(status)) {
                    
                    result = NULL;
                    delete tmpResult;
                } else {
                    result = tmpResult;
                }
            }
        } else {
            
            delete tmpResult;
        }
    }
    umtx_unlock(&gZoneMetaLock);

    return result;
}

UVector*
ZoneMeta::createMetazoneMappings(const UnicodeString &tzid) {
    UVector *mzMappings = NULL;
    UErrorCode status = U_ZERO_ERROR;

    UnicodeString canonicalID;
    UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
    ures_getByKey(rb, gMetazoneInfo, rb, &status);
    getCanonicalCLDRID(tzid, canonicalID, status);

    if (U_SUCCESS(status)) {
        char tzKey[ZID_KEY_MAX + 1];
        int32_t tzKeyLen = canonicalID.extract(0, canonicalID.length(), tzKey, sizeof(tzKey), US_INV);
        tzKey[tzKeyLen] = 0;

        
        char *p = tzKey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
            }
            p++;
        }

        ures_getByKey(rb, tzKey, rb, &status);

        if (U_SUCCESS(status)) {
            UResourceBundle *mz = NULL;
            while (ures_hasNext(rb)) {
                mz = ures_getNextResource(rb, mz, &status);

                const UChar *mz_name = ures_getStringByIndex(mz, 0, NULL, &status);
                const UChar *mz_from = gDefaultFrom;
                const UChar *mz_to = gDefaultTo;

                if (ures_getSize(mz) == 3) {
                    mz_from = ures_getStringByIndex(mz, 1, NULL, &status);
                    mz_to   = ures_getStringByIndex(mz, 2, NULL, &status);
                }

                if(U_FAILURE(status)){
                    status = U_ZERO_ERROR;
                    continue;
                }
                
                
                
                UDate from = parseDate(mz_from, status);
                UDate to = parseDate(mz_to, status);
                if (U_FAILURE(status)) {
                    status = U_ZERO_ERROR;
                    continue;
                }

                OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
                if (entry == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                entry->mzid = mz_name;
                entry->from = from;
                entry->to = to;

                if (mzMappings == NULL) {
                    mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                    if (U_FAILURE(status)) {
                        delete mzMappings;
                        deleteOlsonToMetaMappingEntry(entry);
                        uprv_free(entry);
                        break;
                    }
                }

                mzMappings->addElement(entry, status);
                if (U_FAILURE(status)) {
                    break;
                }
            }
            ures_close(mz);
            if (U_FAILURE(status)) {
                if (mzMappings != NULL) {
                    delete mzMappings;
                    mzMappings = NULL;
                }
            }
        }
    }
    ures_close(rb);
    return mzMappings;
}

UnicodeString& U_EXPORT2
ZoneMeta::getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result) {
    UErrorCode status = U_ZERO_ERROR;
    const UChar *tzid = NULL;
    int32_t tzidLen = 0;
    char keyBuf[ZID_KEY_MAX + 1];
    int32_t keyLen = 0;

    if (mzid.isBogus() || mzid.length() > ZID_KEY_MAX) {
        result.setToBogus();
        return result;
    }

    keyLen = mzid.extract(0, mzid.length(), keyBuf, ZID_KEY_MAX + 1, US_INV);
    keyBuf[keyLen] = 0;

    UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
    ures_getByKey(rb, gMapTimezonesTag, rb, &status);
    ures_getByKey(rb, keyBuf, rb, &status);

    if (U_SUCCESS(status)) {
        
        if (region.length() == 2 || region.length() == 3) {
            keyLen = region.extract(0, region.length(), keyBuf, ZID_KEY_MAX + 1, US_INV);
            keyBuf[keyLen] = 0;
            tzid = ures_getStringByKey(rb, keyBuf, &tzidLen, &status);
            if (status == U_MISSING_RESOURCE_ERROR) {
                status = U_ZERO_ERROR;
            }
        }
        if (U_SUCCESS(status) && tzid == NULL) {
            
            tzid = ures_getStringByKey(rb, gWorldTag, &tzidLen, &status);
        }
    }
    ures_close(rb);

    if (tzid == NULL) {
        result.setToBogus();
    } else {
        result.setTo(tzid, tzidLen);
    }

    return result;
}

static void U_CALLCONV initAvailableMetaZoneIDs () {
    U_ASSERT(gMetaZoneIDs == NULL);
    U_ASSERT(gMetaZoneIDTable == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);

    UErrorCode status = U_ZERO_ERROR;
    gMetaZoneIDTable = uhash_open(uhash_hashUnicodeString, uhash_compareUnicodeString, NULL, &status);
    if (U_FAILURE(status) || gMetaZoneIDTable == NULL) {
        gMetaZoneIDTable = NULL;
        return;
    }
    uhash_setKeyDeleter(gMetaZoneIDTable, uprv_deleteUObject);
    
    gMetaZoneIDs = new UVector(NULL, uhash_compareUChars, status);
    if (U_FAILURE(status) || gMetaZoneIDs == NULL) {
        gMetaZoneIDs = NULL;
        uhash_close(gMetaZoneIDTable);
        gMetaZoneIDTable = NULL;
        return;
    }
    gMetaZoneIDs->setDeleter(uprv_free);

    UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
    UResourceBundle *bundle = ures_getByKey(rb, gMapTimezonesTag, NULL, &status);
    UResourceBundle res;
    ures_initStackObject(&res);
    while (U_SUCCESS(status) && ures_hasNext(bundle)) {
        ures_getNextResource(bundle, &res, &status);
        if (U_FAILURE(status)) {
            break;
        }
        const char *mzID = ures_getKey(&res);
        int32_t len = uprv_strlen(mzID);
        UChar *uMzID = (UChar*)uprv_malloc(sizeof(UChar) * (len + 1));
        if (uMzID == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            break;
        }
        u_charsToUChars(mzID, uMzID, len);
        uMzID[len] = 0;
        UnicodeString *usMzID = new UnicodeString(uMzID);
        if (uhash_get(gMetaZoneIDTable, usMzID) == NULL) {
            gMetaZoneIDs->addElement((void *)uMzID, status);
            uhash_put(gMetaZoneIDTable, (void *)usMzID, (void *)uMzID, &status);
        } else {
            uprv_free(uMzID);
            delete usMzID;
        }
    }
    ures_close(&res);
    ures_close(bundle);
    ures_close(rb);

    if (U_FAILURE(status)) {
        uhash_close(gMetaZoneIDTable);
        delete gMetaZoneIDs;
        gMetaZoneIDTable = NULL;
        gMetaZoneIDs = NULL;
    }
}

const UVector*
ZoneMeta::getAvailableMetazoneIDs() {
    umtx_initOnce(gMetaZoneIDsInitOnce, &initAvailableMetaZoneIDs);
    return gMetaZoneIDs;
}

const UChar*
ZoneMeta::findMetaZoneID(const UnicodeString& mzid) {
    umtx_initOnce(gMetaZoneIDsInitOnce, &initAvailableMetaZoneIDs);
    if (gMetaZoneIDTable == NULL) {
        return NULL;
    }
    return (const UChar*)uhash_get(gMetaZoneIDTable, &mzid);
}

const UChar*
ZoneMeta::findTimeZoneID(const UnicodeString& tzid) {
    return TimeZone::findID(tzid);
}


TimeZone*
ZoneMeta::createCustomTimeZone(int32_t offset) {
    UBool negative = FALSE;
    int32_t tmp = offset;
    if (offset < 0) {
        negative = TRUE;
        tmp = -offset;
    }
    int32_t hour, min, sec;

    tmp /= 1000;
    sec = tmp % 60;
    tmp /= 60;
    min = tmp % 60;
    hour = tmp / 60;

    UnicodeString zid;
    formatCustomID(hour, min, sec, negative, zid);
    return new SimpleTimeZone(offset, zid);
}

UnicodeString&
ZoneMeta::formatCustomID(uint8_t hour, uint8_t min, uint8_t sec, UBool negative, UnicodeString& id) {
    
    id.setTo(gCustomTzPrefix, -1);
    if (hour != 0 || min != 0) {
        if (negative) {
          id.append((UChar)0x2D);    
        } else {
          id.append((UChar)0x2B);    
        }
        
        id.append((UChar)(0x30 + (hour%100)/10));
        id.append((UChar)(0x30 + (hour%10)));
        id.append((UChar)0x3A);    
        id.append((UChar)(0x30 + (min%100)/10));
        id.append((UChar)(0x30 + (min%10)));
        if (sec != 0) {
          id.append((UChar)0x3A);    
          id.append((UChar)(0x30 + (sec%100)/10));
          id.append((UChar)(0x30 + (sec%10)));
        }
    }
    return id;
}

const UChar*
ZoneMeta::getShortID(const TimeZone& tz) {
    const UChar* canonicalID = NULL;
    if (dynamic_cast<const OlsonTimeZone *>(&tz) != NULL) {
        
        const OlsonTimeZone *otz = (const OlsonTimeZone*)&tz;
        canonicalID = otz->getCanonicalID();
    }
    if (canonicalID == NULL) {
        return NULL;
    }
    return getShortIDFromCanonical(canonicalID);
}

const UChar*
ZoneMeta::getShortID(const UnicodeString& id) {
    UErrorCode status = U_ZERO_ERROR;
    const UChar* canonicalID = ZoneMeta::getCanonicalCLDRID(id, status);
    if (U_FAILURE(status) || canonicalID == NULL) {
        return NULL;
    }
    return ZoneMeta::getShortIDFromCanonical(canonicalID);
}

const UChar*
ZoneMeta::getShortIDFromCanonical(const UChar* canonicalID) {
    const UChar* shortID = NULL;
    int32_t len = u_strlen(canonicalID);
    char tzidKey[ZID_KEY_MAX + 1];

    u_UCharsToChars(canonicalID, tzidKey, len);
    tzidKey[len] = (char) 0; 

    
    char *p = tzidKey;
    while (*p++) {
        if (*p == '/') {
            *p = ':';
        }
    }

    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *rb = ures_openDirect(NULL, gKeyTypeData, &status);
    ures_getByKey(rb, gTypeMapTag, rb, &status);
    ures_getByKey(rb, gTimezoneTag, rb, &status);
    shortID = ures_getStringByKey(rb, tzidKey, NULL, &status);
    ures_close(rb);

    return shortID;
}

U_NAMESPACE_END

#endif 
