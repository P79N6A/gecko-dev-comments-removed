






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tzgnames.h"

#include "unicode/basictz.h"
#include "unicode/locdspnm.h"
#include "unicode/msgfmt.h"
#include "unicode/rbtz.h"
#include "unicode/simpletz.h"
#include "unicode/vtzone.h"

#include "cmemory.h"
#include "cstring.h"
#include "mutex.h"
#include "uhash.h"
#include "uassert.h"
#include "umutex.h"
#include "uresimp.h"
#include "ureslocs.h"
#include "zonemeta.h"
#include "tznames_impl.h"
#include "olsontz.h"
#include "ucln_in.h"

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX  128

static const char gZoneStrings[]                = "zoneStrings";

static const char gRegionFormatTag[]            = "regionFormat";
static const char gFallbackFormatTag[]          = "fallbackFormat";

static const UChar gEmpty[]                     = {0x00};

static const UChar gDefRegionPattern[]          = {0x7B, 0x30, 0x7D, 0x00}; 
static const UChar gDefFallbackPattern[]        = {0x7B, 0x31, 0x7D, 0x20, 0x28, 0x7B, 0x30, 0x7D, 0x29, 0x00}; 

static const double kDstCheckRange      = (double)184*U_MILLIS_PER_DAY;



U_CDECL_BEGIN

typedef struct PartialLocationKey {
    const UChar* tzID;
    const UChar* mzID;
    UBool isLong;
} PartialLocationKey;




static int32_t U_CALLCONV
hashPartialLocationKey(const UHashTok key) {
    
    PartialLocationKey *p = (PartialLocationKey *)key.pointer;
    UnicodeString str(p->tzID);
    str.append((UChar)0x26)
        .append(p->mzID, -1)
        .append((UChar)0x23)
        .append((UChar)(p->isLong ? 0x4C : 0x53));
    return str.hashCode();
}




static UBool U_CALLCONV
comparePartialLocationKey(const UHashTok key1, const UHashTok key2) {
    PartialLocationKey *p1 = (PartialLocationKey *)key1.pointer;
    PartialLocationKey *p2 = (PartialLocationKey *)key2.pointer;

    if (p1 == p2) {
        return TRUE;
    }
    if (p1 == NULL || p2 == NULL) {
        return FALSE;
    }
    
    return (p1->tzID == p2->tzID && p1->mzID == p2->mzID && p1->isLong == p2->isLong);
}




static void U_CALLCONV
deleteGNameInfo(void *obj) {
    uprv_free(obj);
}




typedef struct GNameInfo {
    UTimeZoneGenericNameType    type;
    const UChar*                tzID;
} ZNameInfo;




typedef struct GMatchInfo {
    const GNameInfo*    gnameInfo;
    int32_t             matchLength;
    UTimeZoneFormatTimeType   timeType;
} ZMatchInfo;

U_CDECL_END




class TimeZoneGenericNameMatchInfo : public UMemory {
public:
    TimeZoneGenericNameMatchInfo(UVector* matches);
    ~TimeZoneGenericNameMatchInfo();

    int32_t size() const;
    UTimeZoneGenericNameType getGenericNameType(int32_t index) const;
    int32_t getMatchLength(int32_t index) const;
    UnicodeString& getTimeZoneID(int32_t index, UnicodeString& tzID) const;

private:
    UVector* fMatches;  
};

TimeZoneGenericNameMatchInfo::TimeZoneGenericNameMatchInfo(UVector* matches)
: fMatches(matches) {
}

TimeZoneGenericNameMatchInfo::~TimeZoneGenericNameMatchInfo() {
    if (fMatches != NULL) {
        delete fMatches;
    }
}

int32_t
TimeZoneGenericNameMatchInfo::size() const {
    if (fMatches == NULL) {
        return 0;
    }
    return fMatches->size();
}

UTimeZoneGenericNameType
TimeZoneGenericNameMatchInfo::getGenericNameType(int32_t index) const {
    GMatchInfo *minfo = (GMatchInfo *)fMatches->elementAt(index);
    if (minfo != NULL) {
        return static_cast<UTimeZoneGenericNameType>(minfo->gnameInfo->type);
    }
    return UTZGNM_UNKNOWN;
}

int32_t
TimeZoneGenericNameMatchInfo::getMatchLength(int32_t index) const {
    ZMatchInfo *minfo = (ZMatchInfo *)fMatches->elementAt(index);
    if (minfo != NULL) {
        return minfo->matchLength;
    }
    return -1;
}

UnicodeString&
TimeZoneGenericNameMatchInfo::getTimeZoneID(int32_t index, UnicodeString& tzID) const {
    GMatchInfo *minfo = (GMatchInfo *)fMatches->elementAt(index);
    if (minfo != NULL && minfo->gnameInfo->tzID != NULL) {
        tzID.setTo(TRUE, minfo->gnameInfo->tzID, -1);
    } else {
        tzID.setToBogus();
    }
    return tzID;
}




class GNameSearchHandler : public TextTrieMapSearchResultHandler {
public:
    GNameSearchHandler(uint32_t types);
    virtual ~GNameSearchHandler();

    UBool handleMatch(int32_t matchLength, const CharacterNode *node, UErrorCode &status);
    UVector* getMatches(int32_t& maxMatchLen);

private:
    uint32_t fTypes;
    UVector* fResults;
    int32_t fMaxMatchLen;
};

GNameSearchHandler::GNameSearchHandler(uint32_t types)
: fTypes(types), fResults(NULL), fMaxMatchLen(0) {
}

GNameSearchHandler::~GNameSearchHandler() {
    if (fResults != NULL) {
        delete fResults;
    }
}

UBool
GNameSearchHandler::handleMatch(int32_t matchLength, const CharacterNode *node, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (node->hasValues()) {
        int32_t valuesCount = node->countValues();
        for (int32_t i = 0; i < valuesCount; i++) {
            GNameInfo *nameinfo = (ZNameInfo *)node->getValue(i);
            if (nameinfo == NULL) {
                break;
            }
            if ((nameinfo->type & fTypes) != 0) {
                
                if (fResults == NULL) {
                    fResults = new UVector(uprv_free, NULL, status);
                    if (fResults == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                    }
                }
                if (U_SUCCESS(status)) {
                    U_ASSERT(fResults != NULL);
                    GMatchInfo *gmatch = (GMatchInfo *)uprv_malloc(sizeof(GMatchInfo));
                    if (gmatch == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                    } else {
                        
                        gmatch->gnameInfo = nameinfo;
                        gmatch->matchLength = matchLength;
                        gmatch->timeType = UTZFMT_TIME_TYPE_UNKNOWN;
                        fResults->addElement(gmatch, status);
                        if (U_FAILURE(status)) {
                            uprv_free(gmatch);
                        } else {
                            if (matchLength > fMaxMatchLen) {
                                fMaxMatchLen = matchLength;
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

UVector*
GNameSearchHandler::getMatches(int32_t& maxMatchLen) {
    
    UVector *results = fResults;
    maxMatchLen = fMaxMatchLen;

    
    fResults = NULL;
    fMaxMatchLen = 0;
    return results;
}

static UMutex gLock = U_MUTEX_INITIALIZER;

class TZGNCore : public UMemory {
public:
    TZGNCore(const Locale& locale, UErrorCode& status);
    virtual ~TZGNCore();

    UnicodeString& getDisplayName(const TimeZone& tz, UTimeZoneGenericNameType type,
                        UDate date, UnicodeString& name) const;

    UnicodeString& getGenericLocationName(const UnicodeString& tzCanonicalID, UnicodeString& name) const;

    int32_t findBestMatch(const UnicodeString& text, int32_t start, uint32_t types,
        UnicodeString& tzID, UTimeZoneFormatTimeType& timeType, UErrorCode& status) const;

private:
    Locale fLocale;
    const TimeZoneNames* fTimeZoneNames;
    UHashtable* fLocationNamesMap;
    UHashtable* fPartialLocationNamesMap;

    MessageFormat* fRegionFormat;
    MessageFormat* fFallbackFormat;

    LocaleDisplayNames* fLocaleDisplayNames;
    ZNStringPool fStringPool;

    TextTrieMap fGNamesTrie;
    UBool fGNamesTrieFullyLoaded;

    char fTargetRegion[ULOC_COUNTRY_CAPACITY];

    void initialize(const Locale& locale, UErrorCode& status);
    void cleanup();

    void loadStrings(const UnicodeString& tzCanonicalID);

    const UChar* getGenericLocationName(const UnicodeString& tzCanonicalID);

    UnicodeString& formatGenericNonLocationName(const TimeZone& tz, UTimeZoneGenericNameType type,
                        UDate date, UnicodeString& name) const;

    UnicodeString& getPartialLocationName(const UnicodeString& tzCanonicalID,
                        const UnicodeString& mzID, UBool isLong, const UnicodeString& mzDisplayName,
                        UnicodeString& name) const;

    const UChar* getPartialLocationName(const UnicodeString& tzCanonicalID,
                        const UnicodeString& mzID, UBool isLong, const UnicodeString& mzDisplayName);

    TimeZoneGenericNameMatchInfo* findLocal(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const;

    TimeZoneNames::MatchInfoCollection* findTimeZoneNames(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const;
};









TZGNCore::TZGNCore(const Locale& locale, UErrorCode& status)
: fLocale(locale),
  fTimeZoneNames(NULL),
  fLocationNamesMap(NULL),
  fPartialLocationNamesMap(NULL),
  fRegionFormat(NULL),
  fFallbackFormat(NULL),
  fLocaleDisplayNames(NULL),
  fStringPool(status),
  fGNamesTrie(TRUE, deleteGNameInfo),
  fGNamesTrieFullyLoaded(FALSE) {
    initialize(locale, status);
}

TZGNCore::~TZGNCore() {
    cleanup();
}

void
TZGNCore::initialize(const Locale& locale, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }

    
    fTimeZoneNames = TimeZoneNames::createInstance(locale, status);
    if (U_FAILURE(status)) {
        return;
    }

    
    UnicodeString rpat(TRUE, gDefRegionPattern, -1);
    UnicodeString fpat(TRUE, gDefFallbackPattern, -1);

    UErrorCode tmpsts = U_ZERO_ERROR;   
    UResourceBundle *zoneStrings = ures_open(U_ICUDATA_ZONE, locale.getName(), &tmpsts);
    zoneStrings = ures_getByKeyWithFallback(zoneStrings, gZoneStrings, zoneStrings, &tmpsts);

    if (U_SUCCESS(tmpsts)) {
        const UChar *regionPattern = ures_getStringByKeyWithFallback(zoneStrings, gRegionFormatTag, NULL, &tmpsts);
        if (U_SUCCESS(tmpsts) && u_strlen(regionPattern) > 0) {
            rpat.setTo(regionPattern, -1);
        }
        tmpsts = U_ZERO_ERROR;
        const UChar *fallbackPattern = ures_getStringByKeyWithFallback(zoneStrings, gFallbackFormatTag, NULL, &tmpsts);
        if (U_SUCCESS(tmpsts) && u_strlen(fallbackPattern) > 0) {
            fpat.setTo(fallbackPattern, -1);
        }
    }
    ures_close(zoneStrings);

    fRegionFormat = new MessageFormat(rpat, status);
    if (fRegionFormat == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    fFallbackFormat = new MessageFormat(fpat, status);
    if (fFallbackFormat == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        cleanup();
        return;
    }

    
    fLocaleDisplayNames = LocaleDisplayNames::createInstance(locale);

    
    fLocationNamesMap = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL, &status);
    if (U_FAILURE(status)) {
        cleanup();
        return;
    }

    fPartialLocationNamesMap = uhash_open(hashPartialLocationKey, comparePartialLocationKey, NULL, &status);
    if (U_FAILURE(status)) {
        cleanup();
        return;
    }
    uhash_setKeyDeleter(fPartialLocationNamesMap, uprv_free);
    

    
    const char* region = fLocale.getCountry();
    int32_t regionLen = uprv_strlen(region);
    if (regionLen == 0) {
        char loc[ULOC_FULLNAME_CAPACITY];
        uloc_addLikelySubtags(fLocale.getName(), loc, sizeof(loc), &status);

        regionLen = uloc_getCountry(loc, fTargetRegion, sizeof(fTargetRegion), &status);
        if (U_SUCCESS(status)) {
            fTargetRegion[regionLen] = 0;
        } else {
            cleanup();
            return;
        }
    } else if (regionLen < (int32_t)sizeof(fTargetRegion)) {
        uprv_strcpy(fTargetRegion, region);
    } else {
        fTargetRegion[0] = 0;
    }

    
    TimeZone *tz = TimeZone::createDefault();
    const UChar *tzID = ZoneMeta::getCanonicalCLDRID(*tz);
    if (tzID != NULL) {
        loadStrings(UnicodeString(TRUE, tzID, -1));
    }
    delete tz;
}

void
TZGNCore::cleanup() {
    if (fRegionFormat != NULL) {
        delete fRegionFormat;
    }
    if (fFallbackFormat != NULL) {
        delete fFallbackFormat;
    }
    if (fLocaleDisplayNames != NULL) {
        delete fLocaleDisplayNames;
    }
    if (fTimeZoneNames != NULL) {
        delete fTimeZoneNames;
    }

    uhash_close(fLocationNamesMap);
    uhash_close(fPartialLocationNamesMap);
}


UnicodeString&
TZGNCore::getDisplayName(const TimeZone& tz, UTimeZoneGenericNameType type, UDate date, UnicodeString& name) const {
    name.setToBogus();
    switch (type) {
    case UTZGNM_LOCATION:
        {
            const UChar* tzCanonicalID = ZoneMeta::getCanonicalCLDRID(tz);
            if (tzCanonicalID != NULL) {
                getGenericLocationName(UnicodeString(TRUE, tzCanonicalID, -1), name);
            }
        }
        break;
    case UTZGNM_LONG:
    case UTZGNM_SHORT:
        formatGenericNonLocationName(tz, type, date, name);
        if (name.isEmpty()) {
            const UChar* tzCanonicalID = ZoneMeta::getCanonicalCLDRID(tz);
            if (tzCanonicalID != NULL) {
                getGenericLocationName(UnicodeString(TRUE, tzCanonicalID, -1), name);
            }
        }
        break;
    default:
        break;
    }
    return name;
}

UnicodeString&
TZGNCore::getGenericLocationName(const UnicodeString& tzCanonicalID, UnicodeString& name) const {
    if (tzCanonicalID.isEmpty()) {
        name.setToBogus();
        return name;
    }

    const UChar *locname = NULL;
    TZGNCore *nonConstThis = const_cast<TZGNCore *>(this);
    umtx_lock(&gLock);
    {
        locname = nonConstThis->getGenericLocationName(tzCanonicalID);
    }
    umtx_unlock(&gLock);

    if (locname == NULL) {
        name.setToBogus();
    } else {
        name.setTo(locname, u_strlen(locname));
    }

    return name;
}




const UChar*
TZGNCore::getGenericLocationName(const UnicodeString& tzCanonicalID) {
    U_ASSERT(!tzCanonicalID.isEmpty());
    if (tzCanonicalID.length() > ZID_KEY_MAX) {
        return NULL;
    }

    UErrorCode status = U_ZERO_ERROR;
    UChar tzIDKey[ZID_KEY_MAX + 1];
    int32_t tzIDKeyLen = tzCanonicalID.extract(tzIDKey, ZID_KEY_MAX + 1, status);
    U_ASSERT(status == U_ZERO_ERROR);   
    tzIDKey[tzIDKeyLen] = 0;

    const UChar *locname = (const UChar *)uhash_get(fLocationNamesMap, tzIDKey);

    if (locname != NULL) {
        
        if (locname == gEmpty) {
            return NULL;
        }
        return locname;
    }

    
    UnicodeString name;
    UnicodeString usCountryCode;
    UBool isPrimary = FALSE;

    ZoneMeta::getCanonicalCountry(tzCanonicalID, usCountryCode, &isPrimary);

    if (!usCountryCode.isEmpty()) {
        FieldPosition fpos;

        if (isPrimary) {
            
            char countryCode[ULOC_COUNTRY_CAPACITY];
            U_ASSERT(usCountryCode.length() < ULOC_COUNTRY_CAPACITY);
            int32_t ccLen = usCountryCode.extract(0, usCountryCode.length(), countryCode, sizeof(countryCode), US_INV);
            countryCode[ccLen] = 0;

            UnicodeString country;
            fLocaleDisplayNames->regionDisplayName(countryCode, country);

            Formattable param[] = {
                Formattable(country)
            };

            fRegionFormat->format(param, 1, name, fpos, status);
        } else {
            
            

            
            

            UnicodeString city;
            fTimeZoneNames->getExemplarLocationName(tzCanonicalID, city);

            Formattable param[] = {
                Formattable(city),
            };

            fRegionFormat->format(param, 1, name, fpos, status);
        }
        if (U_FAILURE(status)) {
            return NULL;
        }
    }

    locname = name.isEmpty() ? NULL : fStringPool.get(name, status);
    if (U_SUCCESS(status)) {
        
        const UChar* cacheID = ZoneMeta::findTimeZoneID(tzCanonicalID);
        U_ASSERT(cacheID != NULL);
        if (locname == NULL) {
            
            uhash_put(fLocationNamesMap, (void *)cacheID, (void *)gEmpty, &status);
        } else {
            uhash_put(fLocationNamesMap, (void *)cacheID, (void *)locname, &status);
            if (U_FAILURE(status)) {
                locname = NULL;
            } else {
                
                GNameInfo *nameinfo = (ZNameInfo *)uprv_malloc(sizeof(GNameInfo));
                if (nameinfo != NULL) {
                    nameinfo->type = UTZGNM_LOCATION;
                    nameinfo->tzID = cacheID;
                    fGNamesTrie.put(locname, nameinfo, status);
                }
            }
        }
    }

    return locname;
}

UnicodeString&
TZGNCore::formatGenericNonLocationName(const TimeZone& tz, UTimeZoneGenericNameType type, UDate date, UnicodeString& name) const {
    U_ASSERT(type == UTZGNM_LONG || type == UTZGNM_SHORT);
    name.setToBogus();

    const UChar* uID = ZoneMeta::getCanonicalCLDRID(tz);
    if (uID == NULL) {
        return name;
    }

    UnicodeString tzID(TRUE, uID, -1);

    
    UTimeZoneNameType nameType = (type == UTZGNM_LONG) ? UTZNM_LONG_GENERIC : UTZNM_SHORT_GENERIC;
    fTimeZoneNames->getTimeZoneDisplayName(tzID, nameType, name);

    if (!name.isEmpty()) {
        return name;
    }

    
    UChar mzIDBuf[32];
    UnicodeString mzID(mzIDBuf, 0, UPRV_LENGTHOF(mzIDBuf));
    fTimeZoneNames->getMetaZoneID(tzID, date, mzID);
    if (!mzID.isEmpty()) {
        UErrorCode status = U_ZERO_ERROR;
        UBool useStandard = FALSE;
        int32_t raw, sav;
        UChar tmpNameBuf[64];

        tz.getOffset(date, FALSE, raw, sav, status);
        if (U_FAILURE(status)) {
            return name;
        }

        if (sav == 0) {
            useStandard = TRUE;

            TimeZone *tmptz = tz.clone();
            
            BasicTimeZone *btz = NULL;
            if (dynamic_cast<OlsonTimeZone *>(tmptz) != NULL
                || dynamic_cast<SimpleTimeZone *>(tmptz) != NULL
                || dynamic_cast<RuleBasedTimeZone *>(tmptz) != NULL
                || dynamic_cast<VTimeZone *>(tmptz) != NULL) {
                btz = (BasicTimeZone*)tmptz;
            }

            if (btz != NULL) {
                TimeZoneTransition before;
                UBool beforTrs = btz->getPreviousTransition(date, TRUE, before);
                if (beforTrs
                        && (date - before.getTime() < kDstCheckRange)
                        && before.getFrom()->getDSTSavings() != 0) {
                    useStandard = FALSE;
                } else {
                    TimeZoneTransition after;
                    UBool afterTrs = btz->getNextTransition(date, FALSE, after);
                    if (afterTrs
                            && (after.getTime() - date < kDstCheckRange)
                            && after.getTo()->getDSTSavings() != 0) {
                        useStandard = FALSE;
                    }
                }
            } else {
                
                
                tmptz->getOffset(date - kDstCheckRange, FALSE, raw, sav, status);
                if (sav != 0) {
                    useStandard = FALSE;
                } else {
                    tmptz->getOffset(date + kDstCheckRange, FALSE, raw, sav, status);
                    if (sav != 0){
                        useStandard = FALSE;
                    }
                }
                if (U_FAILURE(status)) {
                    delete tmptz;
                    return name;
                }
            }
            delete tmptz;
        }
        if (useStandard) {
            UTimeZoneNameType stdNameType = (nameType == UTZNM_LONG_GENERIC)
                ? UTZNM_LONG_STANDARD : UTZNM_SHORT_STANDARD;
            UnicodeString stdName(tmpNameBuf, 0, UPRV_LENGTHOF(tmpNameBuf));
            fTimeZoneNames->getDisplayName(tzID, stdNameType, date, stdName);
            if (!stdName.isEmpty()) {
                name.setTo(stdName);

                
                
                
                
                
                UChar genNameBuf[64];
                UnicodeString mzGenericName(genNameBuf, 0, UPRV_LENGTHOF(genNameBuf));
                fTimeZoneNames->getMetaZoneDisplayName(mzID, nameType, mzGenericName);
                if (stdName.caseCompare(mzGenericName, 0) == 0) {
                    name.setToBogus();
                }
            }
        }
        if (name.isEmpty()) {
            
            UnicodeString mzName(tmpNameBuf, 0, UPRV_LENGTHOF(tmpNameBuf));
            fTimeZoneNames->getMetaZoneDisplayName(mzID, nameType, mzName);
            if (!mzName.isEmpty()) {
                
                
                
                UChar idBuf[32];
                UnicodeString goldenID(idBuf, 0, UPRV_LENGTHOF(idBuf));
                fTimeZoneNames->getReferenceZoneID(mzID, fTargetRegion, goldenID);
                if (!goldenID.isEmpty() && goldenID != tzID) {
                    TimeZone *goldenZone = TimeZone::createTimeZone(goldenID);
                    int32_t raw1, sav1;

                    
                    
                    
                    
                    goldenZone->getOffset(date + raw + sav, TRUE, raw1, sav1, status);
                    delete goldenZone;
                    if (U_SUCCESS(status)) {
                        if (raw != raw1 || sav != sav1) {
                            
                            getPartialLocationName(tzID, mzID, (nameType == UTZNM_LONG_GENERIC), mzName, name);
                        } else {
                            name.setTo(mzName);
                        }
                    }
                } else {
                    name.setTo(mzName);
                }
            }
        }
    }
    return name;
}

UnicodeString&
TZGNCore::getPartialLocationName(const UnicodeString& tzCanonicalID,
                        const UnicodeString& mzID, UBool isLong, const UnicodeString& mzDisplayName,
                        UnicodeString& name) const {
    name.setToBogus();
    if (tzCanonicalID.isEmpty() || mzID.isEmpty() || mzDisplayName.isEmpty()) {
        return name;
    }

    const UChar *uplname = NULL;
    TZGNCore *nonConstThis = const_cast<TZGNCore *>(this);
    umtx_lock(&gLock);
    {
        uplname = nonConstThis->getPartialLocationName(tzCanonicalID, mzID, isLong, mzDisplayName);
    }
    umtx_unlock(&gLock);

    if (uplname == NULL) {
        name.setToBogus();
    } else {
        name.setTo(TRUE, uplname, -1);
    }
    return name;
}




const UChar*
TZGNCore::getPartialLocationName(const UnicodeString& tzCanonicalID,
                        const UnicodeString& mzID, UBool isLong, const UnicodeString& mzDisplayName) {
    U_ASSERT(!tzCanonicalID.isEmpty());
    U_ASSERT(!mzID.isEmpty());
    U_ASSERT(!mzDisplayName.isEmpty());

    PartialLocationKey key;
    key.tzID = ZoneMeta::findTimeZoneID(tzCanonicalID);
    key.mzID = ZoneMeta::findMetaZoneID(mzID);
    key.isLong = isLong;
    U_ASSERT(key.tzID != NULL && key.mzID != NULL);

    const UChar* uplname = (const UChar*)uhash_get(fPartialLocationNamesMap, (void *)&key);
    if (uplname != NULL) {
        return uplname;
    }

    UnicodeString location;
    UnicodeString usCountryCode;
    ZoneMeta::getCanonicalCountry(tzCanonicalID, usCountryCode);
    if (!usCountryCode.isEmpty()) {
        char countryCode[ULOC_COUNTRY_CAPACITY];
        U_ASSERT(usCountryCode.length() < ULOC_COUNTRY_CAPACITY);
        int32_t ccLen = usCountryCode.extract(0, usCountryCode.length(), countryCode, sizeof(countryCode), US_INV);
        countryCode[ccLen] = 0;

        UnicodeString regionalGolden;
        fTimeZoneNames->getReferenceZoneID(mzID, countryCode, regionalGolden);
        if (tzCanonicalID == regionalGolden) {
            
            fLocaleDisplayNames->regionDisplayName(countryCode, location);
        } else {
            
            fTimeZoneNames->getExemplarLocationName(tzCanonicalID, location);
        }
    } else {
        fTimeZoneNames->getExemplarLocationName(tzCanonicalID, location);
        if (location.isEmpty()) {
            
            
            
            location.setTo(tzCanonicalID);
        }
    }

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString name;

    FieldPosition fpos;
    Formattable param[] = {
        Formattable(location),
        Formattable(mzDisplayName)
    };
    fFallbackFormat->format(param, 2, name, fpos, status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    uplname = fStringPool.get(name, status);
    if (U_SUCCESS(status)) {
        
        PartialLocationKey* cacheKey = (PartialLocationKey *)uprv_malloc(sizeof(PartialLocationKey));
        if (cacheKey != NULL) {
            cacheKey->tzID = key.tzID;
            cacheKey->mzID = key.mzID;
            cacheKey->isLong = key.isLong;
            uhash_put(fPartialLocationNamesMap, (void *)cacheKey, (void *)uplname, &status);
            if (U_FAILURE(status)) {
                uprv_free(cacheKey);
            } else {
                
                GNameInfo *nameinfo = (ZNameInfo *)uprv_malloc(sizeof(GNameInfo));
                if (nameinfo != NULL) {
                    nameinfo->type = isLong ? UTZGNM_LONG : UTZGNM_SHORT;
                    nameinfo->tzID = key.tzID;
                    fGNamesTrie.put(uplname, nameinfo, status);
                }
            }
        }
    }
    return uplname;
}





void
TZGNCore::loadStrings(const UnicodeString& tzCanonicalID) {
    
    getGenericLocationName(tzCanonicalID);

    
    UErrorCode status = U_ZERO_ERROR;

    const UnicodeString *mzID;
    UnicodeString goldenID;
    UnicodeString mzGenName;
    UTimeZoneNameType genNonLocTypes[] = {
        UTZNM_LONG_GENERIC, UTZNM_SHORT_GENERIC,
        UTZNM_UNKNOWN 
    };

    StringEnumeration *mzIDs = fTimeZoneNames->getAvailableMetaZoneIDs(tzCanonicalID, status);
    while ((mzID = mzIDs->snext(status))) {
        if (U_FAILURE(status)) {
            break;
        }
        
        
        
        fTimeZoneNames->getReferenceZoneID(*mzID, fTargetRegion, goldenID);
        if (tzCanonicalID != goldenID) {
            for (int32_t i = 0; genNonLocTypes[i] != UTZNM_UNKNOWN; i++) {
                fTimeZoneNames->getMetaZoneDisplayName(*mzID, genNonLocTypes[i], mzGenName);
                if (!mzGenName.isEmpty()) {
                    
                    getPartialLocationName(tzCanonicalID, *mzID,
                        (genNonLocTypes[i] == UTZNM_LONG_GENERIC), mzGenName);
                }
            }
        }
    }
    if (mzIDs != NULL) {
        delete mzIDs;
    }
}

int32_t
TZGNCore::findBestMatch(const UnicodeString& text, int32_t start, uint32_t types,
        UnicodeString& tzID, UTimeZoneFormatTimeType& timeType, UErrorCode& status) const {
    timeType = UTZFMT_TIME_TYPE_UNKNOWN;
    tzID.setToBogus();

    if (U_FAILURE(status)) {
        return 0;
    }

    
    TimeZoneNames::MatchInfoCollection *tznamesMatches = findTimeZoneNames(text, start, types, status);
    if (U_FAILURE(status)) {
        return 0;
    }

    int32_t bestMatchLen = 0;
    UTimeZoneFormatTimeType bestMatchTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
    UnicodeString bestMatchTzID;
    
    UBool isStandard = FALSE;       

    if (tznamesMatches != NULL) {
        UnicodeString mzID;
        for (int32_t i = 0; i < tznamesMatches->size(); i++) {
            int32_t len = tznamesMatches->getMatchLengthAt(i);
            if (len > bestMatchLen) {
                bestMatchLen = len;
                if (!tznamesMatches->getTimeZoneIDAt(i, bestMatchTzID)) {
                    
                    if (tznamesMatches->getMetaZoneIDAt(i, mzID)) {
                        fTimeZoneNames->getReferenceZoneID(mzID, fTargetRegion, bestMatchTzID);
                    }
                }
                UTimeZoneNameType nameType = tznamesMatches->getNameTypeAt(i);
                if (U_FAILURE(status)) {
                    break;
                }
                switch (nameType) {
                case UTZNM_LONG_STANDARD:
                    
                case UTZNM_SHORT_STANDARD:  
                    isStandard = TRUE;      
                    bestMatchTimeType = UTZFMT_TIME_TYPE_STANDARD;
                    break;
                case UTZNM_LONG_DAYLIGHT:
                case UTZNM_SHORT_DAYLIGHT: 
                    bestMatchTimeType = UTZFMT_TIME_TYPE_DAYLIGHT;
                    break;
                default:
                    bestMatchTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
                }
            }
        }
        delete tznamesMatches;
        if (U_FAILURE(status)) {
            return 0;
        }

        if (bestMatchLen == (text.length() - start)) {
            

            
            
            

            
            
            
            







            
            
            
            
            
            
            if (!isStandard) {
                tzID.setTo(bestMatchTzID);
                timeType = bestMatchTimeType;
                return bestMatchLen;
            }
        }
    }

    
    TimeZoneGenericNameMatchInfo *localMatches = findLocal(text, start, types, status);
    if (U_FAILURE(status)) {
        return 0;
    }
    if (localMatches != NULL) {
        for (int32_t i = 0; i < localMatches->size(); i++) {
            int32_t len = localMatches->getMatchLength(i);

            
            
            
            
            
            if (len >= bestMatchLen) {
                bestMatchLen = localMatches->getMatchLength(i);
                bestMatchTimeType = UTZFMT_TIME_TYPE_UNKNOWN;   
                localMatches->getTimeZoneID(i, bestMatchTzID);
            }
        }
        delete localMatches;
    }

    if (bestMatchLen > 0) {
        timeType = bestMatchTimeType;
        tzID.setTo(bestMatchTzID);
    }
    return bestMatchLen;
}

TimeZoneGenericNameMatchInfo*
TZGNCore::findLocal(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const {
    GNameSearchHandler handler(types);

    TZGNCore *nonConstThis = const_cast<TZGNCore *>(this);

    umtx_lock(&gLock);
    {
        fGNamesTrie.search(text, start, (TextTrieMapSearchResultHandler *)&handler, status);
    }
    umtx_unlock(&gLock);

    if (U_FAILURE(status)) {
        return NULL;
    }

    TimeZoneGenericNameMatchInfo *gmatchInfo = NULL;

    int32_t maxLen = 0;
    UVector *results = handler.getMatches(maxLen);
    if (results != NULL && ((maxLen == (text.length() - start)) || fGNamesTrieFullyLoaded)) {
        
        gmatchInfo = new TimeZoneGenericNameMatchInfo(results);
        if (gmatchInfo == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            delete results;
            return NULL;
        }
        return gmatchInfo;
    }

    if (results != NULL) {
        delete results;
    }

    
    
    umtx_lock(&gLock);
    {
        if (!fGNamesTrieFullyLoaded) {
            StringEnumeration *tzIDs = TimeZone::createTimeZoneIDEnumeration(UCAL_ZONE_TYPE_CANONICAL, NULL, NULL, status);
            if (U_SUCCESS(status)) {
                const UnicodeString *tzID;
                while ((tzID = tzIDs->snext(status))) {
                    if (U_FAILURE(status)) {
                        break;
                    }
                    nonConstThis->loadStrings(*tzID);
                }
            }
            if (tzIDs != NULL) {
                delete tzIDs;
            }

            if (U_SUCCESS(status)) {
                nonConstThis->fGNamesTrieFullyLoaded = TRUE;
            }
        }
    }
    umtx_unlock(&gLock);

    if (U_FAILURE(status)) {
        return NULL;
    }

    umtx_lock(&gLock);
    {
        
        fGNamesTrie.search(text, start, (TextTrieMapSearchResultHandler *)&handler, status);
    }
    umtx_unlock(&gLock);

    results = handler.getMatches(maxLen);
    if (results != NULL && maxLen > 0) {
        gmatchInfo = new TimeZoneGenericNameMatchInfo(results);
        if (gmatchInfo == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            delete results;
            return NULL;
        }
    }

    return gmatchInfo;
}

TimeZoneNames::MatchInfoCollection*
TZGNCore::findTimeZoneNames(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const {
    
    uint32_t nameTypes = 0;
    if (types & UTZGNM_LONG) {
        nameTypes |= (UTZNM_LONG_GENERIC | UTZNM_LONG_STANDARD);
    }
    if (types & UTZGNM_SHORT) {
        nameTypes |= (UTZNM_SHORT_GENERIC | UTZNM_SHORT_STANDARD);
    }

    if (types) {
        
        return fTimeZoneNames->find(text, start, nameTypes, status);
    }

    return NULL;
}

typedef struct TZGNCoreRef {
    TZGNCore*       obj;
    int32_t         refCount;
    double          lastAccess;
} TZGNCoreRef;


static UMutex gTZGNLock = U_MUTEX_INITIALIZER;
static UHashtable *gTZGNCoreCache = NULL;
static UBool gTZGNCoreCacheInitialized = FALSE;



static int32_t gAccessCount = 0;


#define SWEEP_INTERVAL 100





#define CACHE_EXPIRATION 180000.0

U_CDECL_BEGIN



static UBool U_CALLCONV tzgnCore_cleanup(void)
{
    if (gTZGNCoreCache != NULL) {
        uhash_close(gTZGNCoreCache);
        gTZGNCoreCache = NULL;
    }
    gTZGNCoreCacheInitialized = FALSE;
    return TRUE;
}




static void U_CALLCONV
deleteTZGNCoreRef(void *obj) {
    icu::TZGNCoreRef *entry = (icu::TZGNCoreRef*)obj;
    delete (icu::TZGNCore*) entry->obj;
    uprv_free(entry);
}
U_CDECL_END






static void sweepCache() {
    int32_t pos = UHASH_FIRST;
    const UHashElement* elem;
    double now = (double)uprv_getUTCtime();

    while ((elem = uhash_nextElement(gTZGNCoreCache, &pos))) {
        TZGNCoreRef *entry = (TZGNCoreRef *)elem->value.pointer;
        if (entry->refCount <= 0 && (now - entry->lastAccess) > CACHE_EXPIRATION) {
            
            uhash_removeElement(gTZGNCoreCache, elem);
        }
    }
}

TimeZoneGenericNames::TimeZoneGenericNames()
: fRef(0) {
}

TimeZoneGenericNames::~TimeZoneGenericNames() {
    umtx_lock(&gTZGNLock);
    {
        U_ASSERT(fRef->refCount > 0);
        
        fRef->refCount--;
    }
    umtx_unlock(&gTZGNLock);
}

TimeZoneGenericNames*
TimeZoneGenericNames::createInstance(const Locale& locale, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    TimeZoneGenericNames* instance = new TimeZoneGenericNames();
    if (instance == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    TZGNCoreRef *cacheEntry = NULL;
    {
        Mutex lock(&gTZGNLock);

        if (!gTZGNCoreCacheInitialized) {
            
            gTZGNCoreCache = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
            if (U_SUCCESS(status)) {
                uhash_setKeyDeleter(gTZGNCoreCache, uprv_free);
                uhash_setValueDeleter(gTZGNCoreCache, deleteTZGNCoreRef);
                gTZGNCoreCacheInitialized = TRUE;
                ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONEGENERICNAMES, tzgnCore_cleanup);
            }
        }
        if (U_FAILURE(status)) {
            return NULL;
        }

        
        const char *key = locale.getName();
        cacheEntry = (TZGNCoreRef *)uhash_get(gTZGNCoreCache, key);
        if (cacheEntry == NULL) {
            TZGNCore *tzgnCore = NULL;
            char *newKey = NULL;

            tzgnCore = new TZGNCore(locale, status);
            if (tzgnCore == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
            }
            if (U_SUCCESS(status)) {
                newKey = (char *)uprv_malloc(uprv_strlen(key) + 1);
                if (newKey == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                } else {
                    uprv_strcpy(newKey, key);
                }
            }
            if (U_SUCCESS(status)) {
                cacheEntry = (TZGNCoreRef *)uprv_malloc(sizeof(TZGNCoreRef));
                if (cacheEntry == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                } else {
                    cacheEntry->obj = tzgnCore;
                    cacheEntry->refCount = 1;
                    cacheEntry->lastAccess = (double)uprv_getUTCtime();

                    uhash_put(gTZGNCoreCache, newKey, cacheEntry, &status);
                }
            }
            if (U_FAILURE(status)) {
                if (tzgnCore != NULL) {
                    delete tzgnCore;
                }
                if (newKey != NULL) {
                    uprv_free(newKey);
                }
                if (cacheEntry != NULL) {
                    uprv_free(cacheEntry);
                }
                cacheEntry = NULL;
            }
        } else {
            
            cacheEntry->refCount++;
            cacheEntry->lastAccess = (double)uprv_getUTCtime();
        }
        gAccessCount++;
        if (gAccessCount >= SWEEP_INTERVAL) {
            
            sweepCache();
            gAccessCount = 0;
        }
    }  

    if (cacheEntry == NULL) {
        delete instance;
        return NULL;
    }

    instance->fRef = cacheEntry;
    return instance;
}

UBool
TimeZoneGenericNames::operator==(const TimeZoneGenericNames& other) const {
    
    
    return fRef == other.fRef;
}

TimeZoneGenericNames*
TimeZoneGenericNames::clone() const {
    TimeZoneGenericNames* other = new TimeZoneGenericNames();
    if (other) {
        umtx_lock(&gTZGNLock);
        {
            
            fRef->refCount++;
            other->fRef = fRef;
        }
        umtx_unlock(&gTZGNLock);
    }
    return other;
}

UnicodeString&
TimeZoneGenericNames::getDisplayName(const TimeZone& tz, UTimeZoneGenericNameType type,
                        UDate date, UnicodeString& name) const {
    return fRef->obj->getDisplayName(tz, type, date, name);
}

UnicodeString&
TimeZoneGenericNames::getGenericLocationName(const UnicodeString& tzCanonicalID, UnicodeString& name) const {
    return fRef->obj->getGenericLocationName(tzCanonicalID, name);
}

int32_t
TimeZoneGenericNames::findBestMatch(const UnicodeString& text, int32_t start, uint32_t types,
        UnicodeString& tzID, UTimeZoneFormatTimeType& timeType, UErrorCode& status) const {
    return fRef->obj->findBestMatch(text, start, types, tzID, timeType, status);
}

U_NAMESPACE_END
#endif
