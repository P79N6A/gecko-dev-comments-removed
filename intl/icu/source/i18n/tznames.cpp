






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/locid.h"
#include "unicode/tznames.h"
#include "unicode/uenum.h"
#include "cmemory.h"
#include "cstring.h"
#include "mutex.h"
#include "putilimp.h"
#include "tznames_impl.h"
#include "uassert.h"
#include "ucln_in.h"
#include "uhash.h"
#include "umutex.h"
#include "uvector.h"


U_NAMESPACE_BEGIN


static UMutex gTimeZoneNamesLock = U_MUTEX_INITIALIZER;
static UHashtable *gTimeZoneNamesCache = NULL;
static UBool gTimeZoneNamesCacheInitialized = FALSE;



static int32_t gAccessCount = 0;


#define SWEEP_INTERVAL 100





#define CACHE_EXPIRATION 180000.0

typedef struct TimeZoneNamesCacheEntry {
    TimeZoneNames*  names;
    int32_t         refCount;
    double          lastAccess;
} TimeZoneNamesCacheEntry;

U_CDECL_BEGIN



static UBool U_CALLCONV timeZoneNames_cleanup(void)
{
    if (gTimeZoneNamesCache != NULL) {
        uhash_close(gTimeZoneNamesCache);
        gTimeZoneNamesCache = NULL;
    }
    gTimeZoneNamesCacheInitialized = FALSE;
    return TRUE;
}




static void U_CALLCONV
deleteTimeZoneNamesCacheEntry(void *obj) {
    icu::TimeZoneNamesCacheEntry *entry = (icu::TimeZoneNamesCacheEntry*)obj;
    delete (icu::TimeZoneNamesImpl*) entry->names;
    uprv_free(entry);
}
U_CDECL_END






static void sweepCache() {
    int32_t pos = UHASH_FIRST;
    const UHashElement* elem;
    double now = (double)uprv_getUTCtime();

    while ((elem = uhash_nextElement(gTimeZoneNamesCache, &pos))) {
        TimeZoneNamesCacheEntry *entry = (TimeZoneNamesCacheEntry *)elem->value.pointer;
        if (entry->refCount <= 0 && (now - entry->lastAccess) > CACHE_EXPIRATION) {
            
            uhash_removeElement(gTimeZoneNamesCache, elem);
        }
    }
}




class TimeZoneNamesDelegate : public TimeZoneNames {
public:
    TimeZoneNamesDelegate(const Locale& locale, UErrorCode& status);
    virtual ~TimeZoneNamesDelegate();

    virtual UBool operator==(const TimeZoneNames& other) const;
    virtual UBool operator!=(const TimeZoneNames& other) const {return !operator==(other);};
    virtual TimeZoneNames* clone() const;

    StringEnumeration* getAvailableMetaZoneIDs(UErrorCode& status) const;
    StringEnumeration* getAvailableMetaZoneIDs(const UnicodeString& tzID, UErrorCode& status) const;
    UnicodeString& getMetaZoneID(const UnicodeString& tzID, UDate date, UnicodeString& mzID) const;
    UnicodeString& getReferenceZoneID(const UnicodeString& mzID, const char* region, UnicodeString& tzID) const;

    UnicodeString& getMetaZoneDisplayName(const UnicodeString& mzID, UTimeZoneNameType type, UnicodeString& name) const;
    UnicodeString& getTimeZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UnicodeString& name) const;

    UnicodeString& getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const;

    MatchInfoCollection* find(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const;
private:
    TimeZoneNamesDelegate();
    TimeZoneNamesCacheEntry*    fTZnamesCacheEntry;
};

TimeZoneNamesDelegate::TimeZoneNamesDelegate()
: fTZnamesCacheEntry(0) {
}

TimeZoneNamesDelegate::TimeZoneNamesDelegate(const Locale& locale, UErrorCode& status) {
    Mutex lock(&gTimeZoneNamesLock);
    if (!gTimeZoneNamesCacheInitialized) {
        
        gTimeZoneNamesCache = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
        if (U_SUCCESS(status)) {
            uhash_setKeyDeleter(gTimeZoneNamesCache, uprv_free);
            uhash_setValueDeleter(gTimeZoneNamesCache, deleteTimeZoneNamesCacheEntry);
            gTimeZoneNamesCacheInitialized = TRUE;
            ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONENAMES, timeZoneNames_cleanup);
        }
    }

    if (U_FAILURE(status)) {
        return;
    }

    
    TimeZoneNamesCacheEntry *cacheEntry = NULL;

    const char *key = locale.getName();
    cacheEntry = (TimeZoneNamesCacheEntry *)uhash_get(gTimeZoneNamesCache, key);
    if (cacheEntry == NULL) {
        TimeZoneNames *tznames = NULL;
        char *newKey = NULL;

        tznames = new TimeZoneNamesImpl(locale, status);
        if (tznames == NULL) {
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
            cacheEntry = (TimeZoneNamesCacheEntry *)uprv_malloc(sizeof(TimeZoneNamesCacheEntry));
            if (cacheEntry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                cacheEntry->names = tznames;
                cacheEntry->refCount = 1;
                cacheEntry->lastAccess = (double)uprv_getUTCtime();

                uhash_put(gTimeZoneNamesCache, newKey, cacheEntry, &status);
            }
        }
        if (U_FAILURE(status)) {
            if (tznames != NULL) {
                delete tznames;
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
    fTZnamesCacheEntry = cacheEntry;
}

TimeZoneNamesDelegate::~TimeZoneNamesDelegate() {
    umtx_lock(&gTimeZoneNamesLock);
    {
        if (fTZnamesCacheEntry) {
            U_ASSERT(fTZnamesCacheEntry->refCount > 0);
            
            fTZnamesCacheEntry->refCount--;
        }
    }
    umtx_unlock(&gTimeZoneNamesLock);
}

UBool
TimeZoneNamesDelegate::operator==(const TimeZoneNames& other) const {
    if (this == &other) {
        return TRUE;
    }
    
    
    const TimeZoneNamesDelegate* rhs = dynamic_cast<const TimeZoneNamesDelegate*>(&other);
    if (rhs) {
        return fTZnamesCacheEntry == rhs->fTZnamesCacheEntry;
    }
    return FALSE;
}

TimeZoneNames*
TimeZoneNamesDelegate::clone() const {
    TimeZoneNamesDelegate* other = new TimeZoneNamesDelegate();
    if (other != NULL) {
        umtx_lock(&gTimeZoneNamesLock);
        {
            
            fTZnamesCacheEntry->refCount++;
            other->fTZnamesCacheEntry = fTZnamesCacheEntry;
        }
        umtx_unlock(&gTimeZoneNamesLock);
    }
    return other;
}

StringEnumeration*
TimeZoneNamesDelegate::getAvailableMetaZoneIDs(UErrorCode& status) const {
    return fTZnamesCacheEntry->names->getAvailableMetaZoneIDs(status);
}

StringEnumeration*
TimeZoneNamesDelegate::getAvailableMetaZoneIDs(const UnicodeString& tzID, UErrorCode& status) const {
    return fTZnamesCacheEntry->names->getAvailableMetaZoneIDs(tzID, status);
}

UnicodeString&
TimeZoneNamesDelegate::getMetaZoneID(const UnicodeString& tzID, UDate date, UnicodeString& mzID) const {
    return fTZnamesCacheEntry->names->getMetaZoneID(tzID, date, mzID);
}

UnicodeString&
TimeZoneNamesDelegate::getReferenceZoneID(const UnicodeString& mzID, const char* region, UnicodeString& tzID) const {
    return fTZnamesCacheEntry->names->getReferenceZoneID(mzID, region, tzID);
}

UnicodeString&
TimeZoneNamesDelegate::getMetaZoneDisplayName(const UnicodeString& mzID, UTimeZoneNameType type, UnicodeString& name) const {
    return fTZnamesCacheEntry->names->getMetaZoneDisplayName(mzID, type, name);
}

UnicodeString&
TimeZoneNamesDelegate::getTimeZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UnicodeString& name) const {
    return fTZnamesCacheEntry->names->getTimeZoneDisplayName(tzID, type, name);
}

UnicodeString&
TimeZoneNamesDelegate::getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const {
    return fTZnamesCacheEntry->names->getExemplarLocationName(tzID, name);
}

TimeZoneNames::MatchInfoCollection*
TimeZoneNamesDelegate::find(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const {
    return fTZnamesCacheEntry->names->find(text, start, types, status);
}




TimeZoneNames::~TimeZoneNames() {
}

TimeZoneNames*
TimeZoneNames::createInstance(const Locale& locale, UErrorCode& status) {
    TimeZoneNames *instance = NULL;
    if (U_SUCCESS(status)) {
        instance = new TimeZoneNamesDelegate(locale, status);
        if (instance == NULL && U_SUCCESS(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return instance;
}

TimeZoneNames*
TimeZoneNames::createTZDBInstance(const Locale& locale, UErrorCode& status) {
    TimeZoneNames *instance = NULL;
    if (U_SUCCESS(status)) {
        instance = new TZDBTimeZoneNames(locale);
        if (instance == NULL && U_SUCCESS(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return instance;
}

UnicodeString&
TimeZoneNames::getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const {
    return TimeZoneNamesImpl::getDefaultExemplarLocationName(tzID, name);
}

UnicodeString&
TimeZoneNames::getDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UDate date, UnicodeString& name) const {
    getTimeZoneDisplayName(tzID, type, name);
    if (name.isEmpty()) {
        UChar mzIDBuf[32];
        UnicodeString mzID(mzIDBuf, 0, UPRV_LENGTHOF(mzIDBuf));
        getMetaZoneID(tzID, date, mzID);
        getMetaZoneDisplayName(mzID, type, name);
    }
    return name;
}


struct MatchInfo : UMemory {
    UTimeZoneNameType nameType;
    UnicodeString id;
    int32_t matchLength;
    UBool isTZID;

    MatchInfo(UTimeZoneNameType nameType, int32_t matchLength, const UnicodeString* tzID, const UnicodeString* mzID) {
        this->nameType = nameType;
        this->matchLength = matchLength;
        if (tzID != NULL) {
            this->id.setTo(*tzID);
            this->isTZID = TRUE;
        } else {
            this->id.setTo(*mzID);
            this->isTZID = FALSE;
        }
    }
};

U_CDECL_BEGIN
static void U_CALLCONV
deleteMatchInfo(void *obj) {
    delete static_cast<MatchInfo *>(obj);
}
U_CDECL_END




TimeZoneNames::MatchInfoCollection::MatchInfoCollection()
: fMatches(NULL) {
}

TimeZoneNames::MatchInfoCollection::~MatchInfoCollection() {
    if (fMatches != NULL) {
        delete fMatches;
    }
}

void
TimeZoneNames::MatchInfoCollection::addZone(UTimeZoneNameType nameType, int32_t matchLength,
            const UnicodeString& tzID, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    MatchInfo* matchInfo = new MatchInfo(nameType, matchLength, &tzID, NULL);
    if (matchInfo == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    matches(status)->addElement(matchInfo, status);
    if (U_FAILURE(status)) {
        delete matchInfo;
    }
}

void
TimeZoneNames::MatchInfoCollection::addMetaZone(UTimeZoneNameType nameType, int32_t matchLength,
            const UnicodeString& mzID, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    MatchInfo* matchInfo = new MatchInfo(nameType, matchLength, NULL, &mzID);
    if (matchInfo == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    matches(status)->addElement(matchInfo, status);
    if (U_FAILURE(status)) {
        delete matchInfo;
    }
}

int32_t
TimeZoneNames::MatchInfoCollection::size() const {
    if (fMatches == NULL) {
        return 0;
    }
    return fMatches->size();
}

UTimeZoneNameType
TimeZoneNames::MatchInfoCollection::getNameTypeAt(int32_t idx) const {
    const MatchInfo* match = (const MatchInfo*)fMatches->elementAt(idx);
    if (match) {
        return match->nameType;
    }
    return UTZNM_UNKNOWN;
}

int32_t
TimeZoneNames::MatchInfoCollection::getMatchLengthAt(int32_t idx) const {
    const MatchInfo* match = (const MatchInfo*)fMatches->elementAt(idx);
    if (match) {
        return match->matchLength;
    }
    return 0;
}

UBool
TimeZoneNames::MatchInfoCollection::getTimeZoneIDAt(int32_t idx, UnicodeString& tzID) const {
    tzID.remove();
    const MatchInfo* match = (const MatchInfo*)fMatches->elementAt(idx);
    if (match && match->isTZID) {
        tzID.setTo(match->id);
        return TRUE;
    }
    return FALSE;
}

UBool
TimeZoneNames::MatchInfoCollection::getMetaZoneIDAt(int32_t idx, UnicodeString& mzID) const {
    mzID.remove();
    const MatchInfo* match = (const MatchInfo*)fMatches->elementAt(idx);
    if (match && !match->isTZID) {
        mzID.setTo(match->id);
        return TRUE;
    }
    return FALSE;
}

UVector*
TimeZoneNames::MatchInfoCollection::matches(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (fMatches != NULL) {
        return fMatches;
    }
    fMatches = new UVector(deleteMatchInfo, NULL, status);
    if (fMatches == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else if (U_FAILURE(status)) {
        delete fMatches;
        fMatches = NULL;
    }
    return fMatches;
}


U_NAMESPACE_END
#endif
