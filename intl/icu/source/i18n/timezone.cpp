




































#include "utypeinfo.h"  

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "ustr_imp.h"

#ifdef U_DEBUG_TZ
# include <stdio.h>
# include "uresimp.h" 

static void debug_tz_loc(const char *f, int32_t l)
{
  fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_tz_msg(const char *pat, ...)
{
  va_list ap;
  va_start(ap, pat);
  vfprintf(stderr, pat, ap);
  fflush(stderr);
}
static char gStrBuf[256];
#define U_DEBUG_TZ_STR(x) u_austrncpy(gStrBuf,x,sizeof(gStrBuf)-1)

#define U_DEBUG_TZ_MSG(x) {debug_tz_loc(__FILE__,__LINE__);debug_tz_msg x;}
#else
#define U_DEBUG_TZ_MSG(x)
#endif

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/ures.h"
#include "unicode/tzfmt.h"
#include "unicode/numfmt.h"
#include "gregoimp.h"
#include "uresimp.h" 
#include "olsontz.h"
#include "mutex.h"
#include "unicode/udata.h"
#include "ucln_in.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/strenum.h"
#include "uassert.h"
#include "zonemeta.h"

#define kZONEINFO "zoneinfo64"
#define kREGIONS  "Regions"
#define kZONES    "Zones"
#define kRULES    "Rules"
#define kNAMES    "Names"
#define kTZVERSION  "TZVersion"
#define kLINKS    "links"
#define kMAX_CUSTOM_HOUR    23
#define kMAX_CUSTOM_MIN     59
#define kMAX_CUSTOM_SEC     59
#define MINUS 0x002D
#define PLUS 0x002B
#define ZERO_DIGIT 0x0030
#define COLON 0x003A



static const UChar         WORLD[] = {0x30, 0x30, 0x31, 0x00}; 

static const UChar         GMT_ID[] = {0x47, 0x4D, 0x54, 0x00}; 
static const UChar         UNKNOWN_ZONE_ID[] = {0x45, 0x74, 0x63, 0x2F, 0x55, 0x6E, 0x6B, 0x6E, 0x6F, 0x77, 0x6E, 0x00}; 
static const int32_t       GMT_ID_LENGTH = 3;
static const int32_t       UNKNOWN_ZONE_ID_LENGTH = 11;

static UMutex LOCK = U_MUTEX_INITIALIZER;
static UMutex TZSET_LOCK = U_MUTEX_INITIALIZER;
static icu::TimeZone* DEFAULT_ZONE = NULL;
static icu::TimeZone* _GMT = NULL;
static icu::TimeZone* _UNKNOWN_ZONE = NULL;

static char TZDATA_VERSION[16];
static UBool TZDataVersionInitialized = FALSE;

static int32_t* MAP_SYSTEM_ZONES = NULL;
static int32_t* MAP_CANONICAL_SYSTEM_ZONES = NULL;
static int32_t* MAP_CANONICAL_SYSTEM_LOCATION_ZONES = NULL;

static int32_t LEN_SYSTEM_ZONES = 0;
static int32_t LEN_CANONICAL_SYSTEM_ZONES = 0;
static int32_t LEN_CANONICAL_SYSTEM_LOCATION_ZONES = 0;

U_CDECL_BEGIN
static UBool U_CALLCONV timeZone_cleanup(void)
{
    delete DEFAULT_ZONE;
    DEFAULT_ZONE = NULL;

    delete _GMT;
    _GMT = NULL;

    delete _UNKNOWN_ZONE;
    _UNKNOWN_ZONE = NULL;

    uprv_memset(TZDATA_VERSION, 0, sizeof(TZDATA_VERSION));
    TZDataVersionInitialized = FALSE;

    LEN_SYSTEM_ZONES = 0;
    uprv_free(MAP_SYSTEM_ZONES);
    MAP_SYSTEM_ZONES = 0;

    LEN_CANONICAL_SYSTEM_ZONES = 0;
    uprv_free(MAP_CANONICAL_SYSTEM_ZONES);
    MAP_CANONICAL_SYSTEM_ZONES = 0;

    LEN_CANONICAL_SYSTEM_LOCATION_ZONES = 0;
    uprv_free(MAP_CANONICAL_SYSTEM_LOCATION_ZONES);
    MAP_CANONICAL_SYSTEM_LOCATION_ZONES = 0;

    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

static int32_t findInStringArray(UResourceBundle* array, const UnicodeString& id, UErrorCode &status)
{
    UnicodeString copy;
    const UChar *u;
    int32_t len;

    int32_t start = 0;
    int32_t limit = ures_getSize(array);
    int32_t mid;
    int32_t lastMid = INT32_MAX;
    if(U_FAILURE(status) || (limit < 1)) {
        return -1;
    }
    U_DEBUG_TZ_MSG(("fisa: Looking for %s, between %d and %d\n", U_DEBUG_TZ_STR(UnicodeString(id).getTerminatedBuffer()), start, limit));

    for (;;) {
        mid = (int32_t)((start + limit) / 2);
        if (lastMid == mid) {   
            break;  
        }
        lastMid = mid;
        u = ures_getStringByIndex(array, mid, &len, &status);
        if (U_FAILURE(status)) {
            break;
        }
        U_DEBUG_TZ_MSG(("tz: compare to %s, %d .. [%d] .. %d\n", U_DEBUG_TZ_STR(u), start, mid, limit));
        copy.setTo(TRUE, u, len);
        int r = id.compare(copy);
        if(r==0) {
            U_DEBUG_TZ_MSG(("fisa: found at %d\n", mid));
            return mid;
        } else if(r<0) {
            limit = mid;
        } else {
            start = mid;
        }
    }
    U_DEBUG_TZ_MSG(("fisa: not found\n"));
    return -1;
}








static UResourceBundle* getZoneByName(const UResourceBundle* top, const UnicodeString& id, UResourceBundle *oldbundle, UErrorCode& status) {
    
    UResourceBundle *tmp = ures_getByKey(top, kNAMES, NULL, &status);

    
    int32_t idx = findInStringArray(tmp, id, status);

    if((idx == -1) && U_SUCCESS(status)) {
        
        status = U_MISSING_RESOURCE_ERROR;
        
        
    } else {
        U_DEBUG_TZ_MSG(("gzbn: oldbundle= size %d, type %d, %s\n", ures_getSize(tmp), ures_getType(tmp), u_errorName(status)));
        tmp = ures_getByKey(top, kZONES, tmp, &status); 
        U_DEBUG_TZ_MSG(("gzbn: loaded ZONES, size %d, type %d, path %s %s\n", ures_getSize(tmp), ures_getType(tmp), ures_getPath(tmp), u_errorName(status)));
        oldbundle = ures_getByIndex(tmp, idx, oldbundle, &status); 
        U_DEBUG_TZ_MSG(("gzbn: loaded z#%d, size %d, type %d, path %s, %s\n", idx, ures_getSize(oldbundle), ures_getType(oldbundle), ures_getPath(oldbundle),  u_errorName(status)));
    }
    ures_close(tmp);
    if(U_FAILURE(status)) {
        
        return NULL;
    } else {
        return oldbundle;
    }
}


UResourceBundle* TimeZone::loadRule(const UResourceBundle* top, const UnicodeString& ruleid, UResourceBundle* oldbundle, UErrorCode& status) {
    char key[64];
    ruleid.extract(0, sizeof(key)-1, key, (int32_t)sizeof(key)-1, US_INV);
    U_DEBUG_TZ_MSG(("loadRule(%s)\n", key));
    UResourceBundle *r = ures_getByKey(top, kRULES, oldbundle, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> kRULES [%s]\n", key, u_errorName(status)));
    r = ures_getByKey(r, key, r, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> item [%s]\n", key, u_errorName(status)));
    return r;
}









static UResourceBundle* openOlsonResource(const UnicodeString& id,
                                          UResourceBundle& res,
                                          UErrorCode& ec)
{
#if U_DEBUG_TZ
    char buf[128];
    id.extract(0, sizeof(buf)-1, buf, sizeof(buf), "");
#endif
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    U_DEBUG_TZ_MSG(("pre: res sz=%d\n", ures_getSize(&res)));
     getZoneByName(top, id, &res, ec);
    
    
    U_DEBUG_TZ_MSG(("Loading zone '%s' (%s, size %d) - %s\n", buf, ures_getKey((UResourceBundle*)&res), ures_getSize(&res), u_errorName(ec)));
    if (ures_getType(&res) == URES_INT) {
        int32_t deref = ures_getInt(&res, &ec) + 0;
        U_DEBUG_TZ_MSG(("getInt: %s - type is %d\n", u_errorName(ec), ures_getType(&res)));
        UResourceBundle *ares = ures_getByKey(top, kZONES, NULL, &ec); 
        ures_getByIndex(ares, deref, &res, &ec);
        ures_close(ares);
        U_DEBUG_TZ_MSG(("alias to #%d (%s) - %s\n", deref, "??", u_errorName(ec)));
    } else {
        U_DEBUG_TZ_MSG(("not an alias - size %d\n", ures_getSize(&res)));
    }
    U_DEBUG_TZ_MSG(("%s - final status is %s\n", buf, u_errorName(ec)));
    return top;
}



namespace {

void
ensureStaticTimeZones() {
    UBool needsInit;
    UMTX_CHECK(&LOCK, (_GMT == NULL), needsInit);   

    
    
    if (needsInit) {
        SimpleTimeZone *tmpUnknown =
            new SimpleTimeZone(0, UnicodeString(TRUE, UNKNOWN_ZONE_ID, UNKNOWN_ZONE_ID_LENGTH));
        SimpleTimeZone *tmpGMT = new SimpleTimeZone(0, UnicodeString(TRUE, GMT_ID, GMT_ID_LENGTH));
        umtx_lock(&LOCK);
        if (_UNKNOWN_ZONE == 0) {
            _UNKNOWN_ZONE = tmpUnknown;
            tmpUnknown = NULL;
        }
        if (_GMT == 0) {
            _GMT = tmpGMT;
            tmpGMT = NULL;
        }
        umtx_unlock(&LOCK);
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        delete tmpUnknown;
        delete tmpGMT;
    }
}

}  

const TimeZone& U_EXPORT2
TimeZone::getUnknown()
{
    ensureStaticTimeZones();
    return *_UNKNOWN_ZONE;
}

const TimeZone* U_EXPORT2
TimeZone::getGMT(void)
{
    ensureStaticTimeZones();
    return _GMT;
}





UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(TimeZone)

TimeZone::TimeZone()
    :   UObject(), fID()
{
}



TimeZone::TimeZone(const UnicodeString &id)
    :   UObject(), fID(id)
{
}



TimeZone::~TimeZone()
{
}



TimeZone::TimeZone(const TimeZone &source)
    :   UObject(source), fID(source.fID)
{
}



TimeZone &
TimeZone::operator=(const TimeZone &right)
{
    if (this != &right) fID = right.fID;
    return *this;
}



UBool
TimeZone::operator==(const TimeZone& that) const
{
    return typeid(*this) == typeid(that) &&
        fID == that.fID;
}



TimeZone* U_EXPORT2
TimeZone::createTimeZone(const UnicodeString& ID)
{
    







    TimeZone* result = createSystemTimeZone(ID);

    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load system time zone with id - falling to custom"));
        result = createCustomTimeZone(ID);
    }
    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load time zone with id - falling to Etc/Unknown(GMT)"));
        result = getUnknown().clone();
    }
    return result;
}






TimeZone*
TimeZone::createSystemTimeZone(const UnicodeString& id) {
    UErrorCode ec = U_ZERO_ERROR;
    return createSystemTimeZone(id, ec);
}

TimeZone*
TimeZone::createSystemTimeZone(const UnicodeString& id, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return NULL;
    }
    TimeZone* z = 0;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("pre-err=%s\n", u_errorName(ec)));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    U_DEBUG_TZ_MSG(("post-err=%s\n", u_errorName(ec)));
    if (U_SUCCESS(ec)) {
        z = new OlsonTimeZone(top, &res, id, ec);
        if (z == NULL) {
          U_DEBUG_TZ_MSG(("cstz: olson time zone failed to initialize - err %s\n", u_errorName(ec)));
        }
    }
    ures_close(&res);
    ures_close(top);
    if (U_FAILURE(ec)) {
        U_DEBUG_TZ_MSG(("cstz: failed to create, err %s\n", u_errorName(ec)));
        delete z;
        z = 0;
    }
    return z;
}











void
TimeZone::initDefault()
{
    
    
    int32_t rawOffset = 0;
    const char *hostID;

    
    
    {
        
        
        
        
        
        
        
        
        
        
        Mutex lock(&TZSET_LOCK);

        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        uprv_tzset(); 

        
        
        
        
        hostID = uprv_tzname(0);

        
        rawOffset = uprv_timezone() * -U_MILLIS_PER_SECOND;
    }

    UBool initialized;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE != NULL), initialized);
    if (initialized) {
        
        return;
    }

    TimeZone* default_zone = NULL;

    
    UnicodeString hostStrID(hostID, -1, US_INV);
    hostStrID.append((UChar)0);
    hostStrID.truncate(hostStrID.length()-1);
    default_zone = createSystemTimeZone(hostStrID);

#if U_PLATFORM_USES_ONLY_WIN32_API
    
    uprv_free(const_cast<char *>(hostID));
#endif

    int32_t hostIDLen = hostStrID.length();
    if (default_zone != NULL && rawOffset != default_zone->getRawOffset()
        && (3 <= hostIDLen && hostIDLen <= 4))
    {
        
        
        delete default_zone;
        default_zone = NULL;
    }

    
    
    if (default_zone == NULL) {
        default_zone = new SimpleTimeZone(rawOffset, hostStrID);
    }

    
    if (default_zone == NULL) {
        const TimeZone* temptz = getGMT();
        
        if (temptz == NULL) {
            return;
        }
        default_zone = temptz->clone();
    }

    
    umtx_lock(&LOCK);
    if (DEFAULT_ZONE == NULL) {
        DEFAULT_ZONE = default_zone;
        default_zone = NULL;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
    umtx_unlock(&LOCK);

    delete default_zone;
}



TimeZone* U_EXPORT2
TimeZone::createDefault()
{
    
    UBool needsInit;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE == NULL), needsInit);
    if (needsInit) {
        initDefault();
    }

    Mutex lock(&LOCK); 
    return (DEFAULT_ZONE != NULL) ? DEFAULT_ZONE->clone() : NULL;
}



void U_EXPORT2
TimeZone::adoptDefault(TimeZone* zone)
{
    if (zone != NULL)
    {
        TimeZone* old = NULL;

        umtx_lock(&LOCK);
        old = DEFAULT_ZONE;
        DEFAULT_ZONE = zone;
        umtx_unlock(&LOCK);

        delete old;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
}


void U_EXPORT2
TimeZone::setDefault(const TimeZone& zone)
{
    adoptDefault(zone.clone());
}









void TimeZone::getOffset(UDate date, UBool local, int32_t& rawOffset,
                         int32_t& dstOffset, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }

    rawOffset = getRawOffset();
    if (!local) {
        date += rawOffset; 
    }

    
    
    
    
    
    
    
    
    
    
    
    
    for (int32_t pass=0; ; ++pass) {
        int32_t year, month, dom, dow;
        double day = uprv_floor(date / U_MILLIS_PER_DAY);
        int32_t millis = (int32_t) (date - day * U_MILLIS_PER_DAY);

        Grego::dayToFields(day, year, month, dom, dow);

        dstOffset = getOffset(GregorianCalendar::AD, year, month, dom,
                              (uint8_t) dow, millis,
                              Grego::monthLength(year, month),
                              ec) - rawOffset;

        
        if (pass!=0 || !local || dstOffset == 0) {
            break;
        }
        
        date -= dstOffset;
    }
}





class TZEnumeration : public StringEnumeration {
private:

    
    
    
    
    int32_t* map;
    int32_t* localMap;
    int32_t  len;
    int32_t  pos;

    TZEnumeration(int32_t* mapData, int32_t mapLen, UBool adoptMapData) : pos(0) {
        map = mapData;
        localMap = adoptMapData ? mapData : NULL;
        len = mapLen;
    }

    UBool getID(int32_t i) {
        UErrorCode ec = U_ZERO_ERROR;
        int32_t idLen = 0;
        const UChar* id = NULL;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        top = ures_getByKey(top, kNAMES, top, &ec); 
        id = ures_getStringByIndex(top, i, &idLen, &ec);
        if(U_FAILURE(ec)) {
            unistr.truncate(0);
        }
        else {
            unistr.fastCopyFrom(UnicodeString(TRUE, id, idLen));
        }
        ures_close(top);
        return U_SUCCESS(ec);
    }

    static int32_t* getMap(USystemTimeZoneType type, int32_t& len, UErrorCode& ec) {
        len = 0;
        if (U_FAILURE(ec)) {
            return NULL;
        }
        int32_t* m = NULL;
        switch (type) {
        case UCAL_ZONE_TYPE_ANY:
            m = MAP_SYSTEM_ZONES;
            len = LEN_SYSTEM_ZONES;
            break;
        case UCAL_ZONE_TYPE_CANONICAL:
            m = MAP_CANONICAL_SYSTEM_ZONES;
            len = LEN_CANONICAL_SYSTEM_ZONES;
            break;
        case UCAL_ZONE_TYPE_CANONICAL_LOCATION:
            m = MAP_CANONICAL_SYSTEM_LOCATION_ZONES;
            len = LEN_CANONICAL_SYSTEM_LOCATION_ZONES;
            break;
        }
        UBool needsInit = FALSE;
        UMTX_CHECK(&LOCK, (len == 0), needsInit);
        if (needsInit) {
            m = initMap(type, len, ec);
        }
        return m;
    }

    static int32_t* initMap(USystemTimeZoneType type, int32_t& len, UErrorCode& ec) {
        len = 0;
        if (U_FAILURE(ec)) {
            return NULL;
        }

        int32_t *result = NULL;

        UResourceBundle *res = ures_openDirect(0, kZONEINFO, &ec);
        res = ures_getByKey(res, kNAMES, res, &ec); 
        if (U_SUCCESS(ec)) {
            int32_t size = ures_getSize(res);
            int32_t *m = (int32_t *)uprv_malloc(size * sizeof(int32_t));
            if (m == NULL) {
                ec = U_MEMORY_ALLOCATION_ERROR;
            } else {
                int32_t numEntries = 0;
                for (int32_t i = 0; i < size; i++) {
                    UnicodeString id = ures_getUnicodeStringByIndex(res, i, &ec);
                    if (U_FAILURE(ec)) {
                        break;
                    }
                    if (0 == id.compare(UNKNOWN_ZONE_ID, UNKNOWN_ZONE_ID_LENGTH)) {
                        
                        continue;
                    }
                    if (type == UCAL_ZONE_TYPE_CANONICAL || type == UCAL_ZONE_TYPE_CANONICAL_LOCATION) {
                        UnicodeString canonicalID;
                        ZoneMeta::getCanonicalCLDRID(id, canonicalID, ec);
                        if (U_FAILURE(ec)) {
                            break;
                        }
                        if (canonicalID != id) {
                            
                            continue;
                        }
                    }
                    if (type == UCAL_ZONE_TYPE_CANONICAL_LOCATION) {
                        const UChar *region = TimeZone::getRegion(id, ec);
                        if (U_FAILURE(ec)) {
                            break;
                        }
                        if (u_strcmp(region, WORLD) == 0) {
                           
                            continue;
                        }
                    }
                    m[numEntries++] = i;
                }
                if (U_SUCCESS(ec)) {
                    int32_t *tmp = m;
                    m = (int32_t *)uprv_realloc(tmp, numEntries * sizeof(int32_t));
                    if (m == NULL) {
                        
                        
                        m = tmp;
                    }

                    umtx_lock(&LOCK);
                    {
                        switch(type) {
                        case UCAL_ZONE_TYPE_ANY:
                            if (MAP_SYSTEM_ZONES == NULL) {
                                MAP_SYSTEM_ZONES = m;
                                LEN_SYSTEM_ZONES = numEntries;
                                m = NULL;
                                ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
                            }
                            result = MAP_SYSTEM_ZONES;
                            len = LEN_SYSTEM_ZONES;
                            break;
                        case UCAL_ZONE_TYPE_CANONICAL:
                            if (MAP_CANONICAL_SYSTEM_ZONES == NULL) {
                                MAP_CANONICAL_SYSTEM_ZONES = m;
                                LEN_CANONICAL_SYSTEM_ZONES = numEntries;
                                m = NULL;
                                ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
                            }
                            result = MAP_CANONICAL_SYSTEM_ZONES;
                            len = LEN_CANONICAL_SYSTEM_ZONES;
                            break;
                        case UCAL_ZONE_TYPE_CANONICAL_LOCATION:
                            if (MAP_CANONICAL_SYSTEM_LOCATION_ZONES == NULL) {
                                MAP_CANONICAL_SYSTEM_LOCATION_ZONES = m;
                                LEN_CANONICAL_SYSTEM_LOCATION_ZONES = numEntries;
                                m = NULL;
                                ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
                            }
                            result = MAP_CANONICAL_SYSTEM_LOCATION_ZONES;
                            len = LEN_CANONICAL_SYSTEM_LOCATION_ZONES;
                            break;
                        }
                    }
                    umtx_unlock(&LOCK);
                }
                uprv_free(m);
            }
        }

        ures_close(res);
        return result;
    }

public:

#define DEFAULT_FILTERED_MAP_SIZE 8
#define MAP_INCREMENT_SIZE 8

    static TZEnumeration* create(USystemTimeZoneType type, const char* region, const int32_t* rawOffset, UErrorCode& ec) {
        if (U_FAILURE(ec)) {
            return NULL;
        }

        int32_t baseLen;
        int32_t *baseMap = getMap(type, baseLen, ec);

        if (U_FAILURE(ec)) {
            return NULL;
        }

        
        

        int32_t *filteredMap = NULL;
        int32_t numEntries = 0;

        if (region != NULL || rawOffset != NULL) {
            int32_t filteredMapSize = DEFAULT_FILTERED_MAP_SIZE;
            filteredMap = (int32_t *)uprv_malloc(filteredMapSize * sizeof(int32_t));
            if (filteredMap == NULL) {
                ec = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }

            
            UResourceBundle *res = ures_openDirect(0, kZONEINFO, &ec);
            res = ures_getByKey(res, kNAMES, res, &ec); 
            for (int32_t i = 0; i < baseLen; i++) {
                int32_t zidx = baseMap[i];
                UnicodeString id = ures_getUnicodeStringByIndex(res, zidx, &ec);
                if (U_FAILURE(ec)) {
                    break;
                }
                if (region != NULL) {
                    
                    char tzregion[4]; 
                    TimeZone::getRegion(id, tzregion, sizeof(tzregion), ec);
                    if (U_FAILURE(ec)) {
                        break;
                    }
                    if (uprv_stricmp(tzregion, region) != 0) {
                        
                        continue;
                    }
                }
                if (rawOffset != NULL) {
                    
                    
                    TimeZone *z = TimeZone::createSystemTimeZone(id, ec);
                    if (U_FAILURE(ec)) {
                        break;
                    }
                    int32_t tzoffset = z->getRawOffset();
                    delete z;

                    if (tzoffset != *rawOffset) {
                        continue;
                    }
                }

                if (filteredMapSize <= numEntries) {
                    filteredMapSize += MAP_INCREMENT_SIZE;
                    int32_t *tmp = (int32_t *)uprv_realloc(filteredMap, filteredMapSize * sizeof(int32_t));
                    if (tmp == NULL) {
                        ec = U_MEMORY_ALLOCATION_ERROR;
                        break;
                    } else {
                        filteredMap = tmp;
                    }
                }

                filteredMap[numEntries++] = zidx;
            }

            if (U_FAILURE(ec)) {
                uprv_free(filteredMap);
                filteredMap = NULL;
            }

            ures_close(res);
        }

        TZEnumeration *result = NULL;
        if (U_SUCCESS(ec)) {
            
            if (filteredMap == NULL) {
                result = new TZEnumeration(baseMap, baseLen, FALSE);
            } else {
                result = new TZEnumeration(filteredMap, numEntries, TRUE);
                filteredMap = NULL;
            }
            if (result == NULL) {
                ec = U_MEMORY_ALLOCATION_ERROR;
            }
        }

        if (filteredMap != NULL) {
            uprv_free(filteredMap);
        }

        return result;
    }

    TZEnumeration(const TZEnumeration &other) : StringEnumeration(), map(NULL), localMap(NULL), len(0), pos(0) {
        if (other.localMap != NULL) {
            localMap = (int32_t *)uprv_malloc(other.len * sizeof(int32_t));
            if (localMap != NULL) {
                len = other.len;
                uprv_memcpy(localMap, other.localMap, len * sizeof(int32_t));
                pos = other.pos;
                map = localMap;
            } else {
                len = 0;
                pos = 0;
                map = NULL;
            }
        } else {
            map = other.map;
            localMap = NULL;
            len = other.len;
            pos = other.pos;
        }
    }

    virtual ~TZEnumeration();

    virtual StringEnumeration *clone() const {
        return new TZEnumeration(*this);
    }

    virtual int32_t count(UErrorCode& status) const {
        return U_FAILURE(status) ? 0 : len;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        if (U_SUCCESS(status) && map != NULL && pos < len) {
            getID(map[pos]);
            ++pos;
            return &unistr;
        }
        return 0;
    }

    virtual void reset(UErrorCode& ) {
        pos = 0;
    }

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

TZEnumeration::~TZEnumeration() {
    if (localMap != NULL) {
        uprv_free(localMap);
    }
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TZEnumeration)

StringEnumeration* U_EXPORT2
TimeZone::createTimeZoneIDEnumeration(
            USystemTimeZoneType zoneType,
            const char* region,
            const int32_t* rawOffset,
            UErrorCode& ec) {
    return TZEnumeration::create(zoneType, region, rawOffset, ec);
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration() {
    UErrorCode ec = U_ZERO_ERROR;
    return TZEnumeration::create(UCAL_ZONE_TYPE_ANY, NULL, NULL, ec);
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(int32_t rawOffset) {
    UErrorCode ec = U_ZERO_ERROR;
    return TZEnumeration::create(UCAL_ZONE_TYPE_ANY, NULL, &rawOffset, ec);
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(const char* country) {
    UErrorCode ec = U_ZERO_ERROR;
    return TZEnumeration::create(UCAL_ZONE_TYPE_ANY, country, NULL, ec);
}



int32_t U_EXPORT2
TimeZone::countEquivalentIDs(const UnicodeString& id) {
    int32_t result = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("countEquivalentIDs..\n"));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    if (U_SUCCESS(ec)) {
        UResourceBundle r;
        ures_initStackObject(&r);
        ures_getByKey(&res, kLINKS, &r, &ec);
        ures_getIntVector(&r, &result, &ec);
        ures_close(&r);
    }
    ures_close(&res);
    ures_close(top);
    return result;
}



const UnicodeString U_EXPORT2
TimeZone::getEquivalentID(const UnicodeString& id, int32_t index) {
    U_DEBUG_TZ_MSG(("gEI(%d)\n", index));
    UnicodeString result;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    UResourceBundle *top = openOlsonResource(id, res, ec);
    int32_t zone = -1;
    if (U_SUCCESS(ec)) {
        UResourceBundle r;
        ures_initStackObject(&r);
        int32_t size;
        ures_getByKey(&res, kLINKS, &r, &ec);
        const int32_t* v = ures_getIntVector(&r, &size, &ec);
        if (U_SUCCESS(ec)) {
            if (index >= 0 && index < size) {
                zone = v[index];
            }
        }
        ures_close(&r);
    }
    ures_close(&res);
    if (zone >= 0) {
        UResourceBundle *ares = ures_getByKey(top, kNAMES, NULL, &ec); 
        if (U_SUCCESS(ec)) {
            int32_t idLen = 0;
            const UChar* id = ures_getStringByIndex(ares, zone, &idLen, &ec);
            result.fastCopyFrom(UnicodeString(TRUE, id, idLen));
            U_DEBUG_TZ_MSG(("gei(%d) -> %d, len%d, %s\n", index, zone, result.length(), u_errorName(ec)));
        }
        ures_close(ares);
    }
    ures_close(top);
#if defined(U_DEBUG_TZ)
    if(result.length() ==0) {
      U_DEBUG_TZ_MSG(("equiv [__, #%d] -> 0 (%s)\n", index, u_errorName(ec)));
    }
#endif
    return result;
}





const UChar*
TimeZone::findID(const UnicodeString& id) {
    const UChar *result = NULL;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle *rb = ures_openDirect(NULL, kZONEINFO, &ec);

    
    UResourceBundle *names = ures_getByKey(rb, kNAMES, NULL, &ec);
    int32_t idx = findInStringArray(names, id, ec);
    result = ures_getStringByIndex(names, idx, NULL, &ec);
    if (U_FAILURE(ec)) {
        result = NULL;
    }
    ures_close(names);
    ures_close(rb);
    return result;
}


const UChar*
TimeZone::dereferOlsonLink(const UnicodeString& id) {
    const UChar *result = NULL;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle *rb = ures_openDirect(NULL, kZONEINFO, &ec);

    
    UResourceBundle *names = ures_getByKey(rb, kNAMES, NULL, &ec);
    int32_t idx = findInStringArray(names, id, ec);
    result = ures_getStringByIndex(names, idx, NULL, &ec);

    
    ures_getByKey(rb, kZONES, rb, &ec);
    ures_getByIndex(rb, idx, rb, &ec); 

    if (U_SUCCESS(ec)) {
        if (ures_getType(rb) == URES_INT) {
            
            int32_t deref = ures_getInt(rb, &ec);
            const UChar* tmp = ures_getStringByIndex(names, deref, NULL, &ec);
            if (U_SUCCESS(ec)) {
                result = tmp;
            }
        }
    }

    ures_close(names);
    ures_close(rb);

    return result;
}

const UChar*
TimeZone::getRegion(const UnicodeString& id) {
    UErrorCode status = U_ZERO_ERROR;
    return getRegion(id, status);
}

const UChar*
TimeZone::getRegion(const UnicodeString& id, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    const UChar *result = NULL;
    UResourceBundle *rb = ures_openDirect(NULL, kZONEINFO, &status);

    
    UResourceBundle *res = ures_getByKey(rb, kNAMES, NULL, &status);
    int32_t idx = findInStringArray(res, id, status);

    
    ures_getByKey(rb, kREGIONS, res, &status);
    const UChar *tmp = ures_getStringByIndex(res, idx, NULL, &status);
    if (U_SUCCESS(status)) {
        result = tmp;
    }

    ures_close(res);
    ures_close(rb);

    return result;
}



int32_t
TimeZone::getRegion(const UnicodeString& id, char *region, int32_t capacity, UErrorCode& status)
{
    int32_t resultLen = 0;
    *region = 0;
    if (U_FAILURE(status)) {
        return 0;
    }

    const UChar *uregion = NULL;
    
    
    if (id.compare(UNKNOWN_ZONE_ID, UNKNOWN_ZONE_ID_LENGTH) != 0) {
        uregion = getRegion(id);
    }
    if (uregion == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    resultLen = u_strlen(uregion);
    
    u_UCharsToChars(uregion, region, uprv_min(resultLen, capacity));

    if (capacity < resultLen) {
        status = U_BUFFER_OVERFLOW_ERROR;
        return resultLen;
    }

    return u_terminateChars(region, capacity, resultLen, &status);
}




UnicodeString&
TimeZone::getDisplayName(UnicodeString& result) const
{
    return getDisplayName(FALSE,LONG,Locale::getDefault(), result);
}

UnicodeString&
TimeZone::getDisplayName(const Locale& locale, UnicodeString& result) const
{
    return getDisplayName(FALSE, LONG, locale, result);
}

UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, UnicodeString& result)  const
{
    return getDisplayName(daylight,style, Locale::getDefault(), result);
}

int32_t
TimeZone::getDSTSavings()const {
    if (useDaylightTime()) {
        return 3600000;
    }
    return 0;
}

UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, const Locale& locale, UnicodeString& result) const
{
    UErrorCode status = U_ZERO_ERROR;
    UDate date = Calendar::getNow();
    UTimeZoneFormatTimeType timeType;
    int32_t offset;

    if (style == GENERIC_LOCATION || style == LONG_GENERIC || style == SHORT_GENERIC) {
        LocalPointer<TimeZoneFormat> tzfmt(TimeZoneFormat::createInstance(locale, status));
        if (U_FAILURE(status)) {
            result.remove();
            return result;
        }
        
        switch (style) {
        case GENERIC_LOCATION:
            tzfmt->format(UTZFMT_STYLE_GENERIC_LOCATION, *this, date, result, &timeType);
            break;
        case LONG_GENERIC:
            tzfmt->format(UTZFMT_STYLE_GENERIC_LONG, *this, date, result, &timeType);
            break;
        case SHORT_GENERIC:
            tzfmt->format(UTZFMT_STYLE_GENERIC_SHORT, *this, date, result, &timeType);
            break;
        default:
            U_ASSERT(FALSE);
        }
        
        
        
        if ((daylight && timeType == UTZFMT_TIME_TYPE_STANDARD) || (!daylight && timeType == UTZFMT_TIME_TYPE_DAYLIGHT)) {
            offset = daylight ? getRawOffset() + getDSTSavings() : getRawOffset();
            tzfmt->formatOffsetLocalizedGMT(offset, result, status);
        }
    } else if (style == LONG_GMT || style == SHORT_GMT) {
        LocalPointer<TimeZoneFormat> tzfmt(TimeZoneFormat::createInstance(locale, status));
        if (U_FAILURE(status)) {
            result.remove();
            return result;
        }
        offset = daylight && useDaylightTime() ? getRawOffset() + getDSTSavings() : getRawOffset();
        switch (style) {
        case LONG_GMT:
            tzfmt->formatOffsetLocalizedGMT(offset, result, status);
            break;
        case SHORT_GMT:
            tzfmt->formatOffsetRFC822(offset, result, status);
            break;
        default:
            U_ASSERT(FALSE);
        }

    } else {
        U_ASSERT(style == LONG || style == SHORT || style == SHORT_COMMONLY_USED);
        UTimeZoneNameType nameType = UTZNM_UNKNOWN;
        switch (style) {
        case LONG:
            nameType = daylight ? UTZNM_LONG_DAYLIGHT : UTZNM_LONG_STANDARD;
            break;
        case SHORT:
        case SHORT_COMMONLY_USED:
            nameType = daylight ? UTZNM_SHORT_DAYLIGHT : UTZNM_SHORT_STANDARD;
            break;
        default:
            U_ASSERT(FALSE);
        }
        LocalPointer<TimeZoneNames> tznames(TimeZoneNames::createInstance(locale, status));
        if (U_FAILURE(status)) {
            result.remove();
            return result;
        }
        UnicodeString canonicalID(ZoneMeta::getCanonicalCLDRID(*this));
        tznames->getDisplayName(canonicalID, nameType, date, result);
        if (result.isEmpty()) {
            
            LocalPointer<TimeZoneFormat> tzfmt(TimeZoneFormat::createInstance(locale, status));
            offset = daylight && useDaylightTime() ? getRawOffset() + getDSTSavings() : getRawOffset();
            tzfmt->formatOffsetLocalizedGMT(offset, result, status);
        }
    }
    if (U_FAILURE(status)) {
        result.remove();
    }
    return  result;
}








TimeZone*
TimeZone::createCustomTimeZone(const UnicodeString& id)
{
    int32_t sign, hour, min, sec;
    if (parseCustomID(id, sign, hour, min, sec)) {
        UnicodeString customID;
        formatCustomID(hour, min, sec, (sign < 0), customID);
        int32_t offset = sign * ((hour * 60 + min) * 60 + sec) * 1000;
        return new SimpleTimeZone(offset, customID);
    }
    return NULL;
}

UnicodeString&
TimeZone::getCustomID(const UnicodeString& id, UnicodeString& normalized, UErrorCode& status) {
    normalized.remove();
    if (U_FAILURE(status)) {
        return normalized;
    }
    int32_t sign, hour, min, sec;
    if (parseCustomID(id, sign, hour, min, sec)) {
        formatCustomID(hour, min, sec, (sign < 0), normalized);
    }
    return normalized;
}

UBool
TimeZone::parseCustomID(const UnicodeString& id, int32_t& sign,
                        int32_t& hour, int32_t& min, int32_t& sec) {
    static const int32_t         kParseFailed = -99999;

    NumberFormat* numberFormat = 0;
    UnicodeString idUppercase = id;
    idUppercase.toUpper("");

    if (id.length() > GMT_ID_LENGTH &&
        idUppercase.startsWith(GMT_ID, GMT_ID_LENGTH))
    {
        ParsePosition pos(GMT_ID_LENGTH);
        sign = 1;
        hour = 0;
        min = 0;
        sec = 0;

        if (id[pos.getIndex()] == MINUS ) {
            sign = -1;
        } else if (id[pos.getIndex()] != PLUS ) {
            return FALSE;
        }
        pos.setIndex(pos.getIndex() + 1);

        UErrorCode success = U_ZERO_ERROR;
        numberFormat = NumberFormat::createInstance(success);
        if(U_FAILURE(success)){
            return FALSE;
        }
        numberFormat->setParseIntegerOnly(TRUE);
        

        
        int32_t start = pos.getIndex();
        Formattable n(kParseFailed);
        numberFormat->parse(id, n, pos);
        if (pos.getIndex() == start) {
            delete numberFormat;
            return FALSE;
        }
        hour = n.getLong();

        if (pos.getIndex() < id.length()) {
            if (pos.getIndex() - start > 2
                || id[pos.getIndex()] != COLON) {
                delete numberFormat;
                return FALSE;
            }
            
            pos.setIndex(pos.getIndex() + 1);
            int32_t oldPos = pos.getIndex();
            n.setLong(kParseFailed);
            numberFormat->parse(id, n, pos);
            if ((pos.getIndex() - oldPos) != 2) {
                
                delete numberFormat;
                return FALSE;
            }
            min = n.getLong();
            if (pos.getIndex() < id.length()) {
                if (id[pos.getIndex()] != COLON) {
                    delete numberFormat;
                    return FALSE;
                }
                
                pos.setIndex(pos.getIndex() + 1);
                oldPos = pos.getIndex();
                n.setLong(kParseFailed);
                numberFormat->parse(id, n, pos);
                if (pos.getIndex() != id.length()
                        || (pos.getIndex() - oldPos) != 2) {
                    delete numberFormat;
                    return FALSE;
                }
                sec = n.getLong();
            }
        } else {
            
            
            
            
            
            
            
            

            int32_t length = pos.getIndex() - start;
            if (length <= 0 || 6 < length) {
                
                delete numberFormat;
                return FALSE;
            }
            switch (length) {
                case 1:
                case 2:
                    
                    break;
                case 3:
                case 4:
                    min = hour % 100;
                    hour /= 100;
                    break;
                case 5:
                case 6:
                    sec = hour % 100;
                    min = (hour/100) % 100;
                    hour /= 10000;
                    break;
            }
        }

        delete numberFormat;

        if (hour > kMAX_CUSTOM_HOUR || min > kMAX_CUSTOM_MIN || sec > kMAX_CUSTOM_SEC) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

UnicodeString&
TimeZone::formatCustomID(int32_t hour, int32_t min, int32_t sec,
                         UBool negative, UnicodeString& id) {
    
    id.setTo(GMT_ID, GMT_ID_LENGTH);
    if (hour | min | sec) {
        if (negative) {
            id += (UChar)MINUS;
        } else {
            id += (UChar)PLUS;
        }

        if (hour < 10) {
            id += (UChar)ZERO_DIGIT;
        } else {
            id += (UChar)(ZERO_DIGIT + hour/10);
        }
        id += (UChar)(ZERO_DIGIT + hour%10);
        id += (UChar)COLON;
        if (min < 10) {
            id += (UChar)ZERO_DIGIT;
        } else {
            id += (UChar)(ZERO_DIGIT + min/10);
        }
        id += (UChar)(ZERO_DIGIT + min%10);

        if (sec) {
            id += (UChar)COLON;
            if (sec < 10) {
                id += (UChar)ZERO_DIGIT;
            } else {
                id += (UChar)(ZERO_DIGIT + sec/10);
            }
            id += (UChar)(ZERO_DIGIT + sec%10);
        }
    }
    return id;
}


UBool
TimeZone::hasSameRules(const TimeZone& other) const
{
    return (getRawOffset() == other.getRawOffset() &&
            useDaylightTime() == other.useDaylightTime());
}

const char*
TimeZone::getTZDataVersion(UErrorCode& status)
{
    
    UBool needsInit;
    UMTX_CHECK(&LOCK, !TZDataVersionInitialized, needsInit);
    if (needsInit) {
        int32_t len = 0;
        UResourceBundle *bundle = ures_openDirect(NULL, kZONEINFO, &status);
        const UChar *tzver = ures_getStringByKey(bundle, kTZVERSION,
            &len, &status);

        if (U_SUCCESS(status)) {
            if (len >= (int32_t)sizeof(TZDATA_VERSION)) {
                
                len = sizeof(TZDATA_VERSION) - 1;
            }
            umtx_lock(&LOCK);
            if (!TZDataVersionInitialized) {
                u_UCharsToChars(tzver, TZDATA_VERSION, len);
                TZDataVersionInitialized = TRUE;
            }
            umtx_unlock(&LOCK);
            ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        }

        ures_close(bundle);
    }
    if (U_FAILURE(status)) {
        return NULL;
    }
    return (const char*)TZDATA_VERSION;
}

UnicodeString&
TimeZone::getCanonicalID(const UnicodeString& id, UnicodeString& canonicalID, UErrorCode& status)
{
    UBool isSystemID = FALSE;
    return getCanonicalID(id, canonicalID, isSystemID, status);
}

UnicodeString&
TimeZone::getCanonicalID(const UnicodeString& id, UnicodeString& canonicalID, UBool& isSystemID,
                         UErrorCode& status)
{
    canonicalID.remove();
    isSystemID = FALSE;
    if (U_FAILURE(status)) {
        return canonicalID;
    }
    if (id.compare(UNKNOWN_ZONE_ID, UNKNOWN_ZONE_ID_LENGTH) == 0) {
        
        canonicalID.fastCopyFrom(id);
        isSystemID = FALSE;
    } else {
        ZoneMeta::getCanonicalCLDRID(id, canonicalID, status);
        if (U_SUCCESS(status)) {
            isSystemID = TRUE;
        } else {
            
            status = U_ZERO_ERROR;
            getCustomID(id, canonicalID, status);
        }
    }
    return canonicalID;
}

U_NAMESPACE_END

#endif 


