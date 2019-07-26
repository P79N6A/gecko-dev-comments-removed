

























#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/gregocal.h"
#include "unicode/basictz.h"
#include "unicode/simpletz.h"
#include "unicode/rbtz.h"
#include "unicode/vtzone.h"
#include "gregoimp.h"
#include "buddhcal.h"
#include "taiwncal.h"
#include "japancal.h"
#include "islamcal.h"
#include "hebrwcal.h"
#include "persncal.h"
#include "indiancal.h"
#include "chnsecal.h"
#include "coptccal.h"
#include "ethpccal.h"
#include "unicode/calendar.h"
#include "cpputils.h"
#include "servloc.h"
#include "ucln_in.h"
#include "cstring.h"
#include "locbased.h"
#include "uresimp.h"
#include "ustrenum.h"
#include "uassert.h"
#include "olsontz.h"

#if !UCONFIG_NO_SERVICE
static icu::ICULocaleService* gService = NULL;
#endif



U_CDECL_BEGIN
static UBool calendar_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    if (gService) {
        delete gService;
        gService = NULL;
    }
#endif
    return TRUE;
}
U_CDECL_END









#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)








#include "udbgutil.h"
#include <stdio.h>








const char* fldName(UCalendarDateFields f) {
	return udbg_enumName(UDBG_UCalendarDateFields, (int32_t)f);
}

#if UCAL_DEBUG_DUMP

void ucal_dump(const Calendar &cal) {
    cal.dump();
}

void Calendar::dump() const {
    int i;
    fprintf(stderr, "@calendar=%s, timeset=%c, fieldset=%c, allfields=%c, virtualset=%c, t=%.2f",
        getType(), fIsTimeSet?'y':'n',  fAreFieldsSet?'y':'n',  fAreAllFieldsSet?'y':'n',  
        fAreFieldsVirtuallySet?'y':'n',
        fTime);

    
    fprintf(stderr, "\n");
    for(i = 0;i<UCAL_FIELD_COUNT;i++) {
        int n;
        const char *f = fldName((UCalendarDateFields)i);
        fprintf(stderr, "  %25s: %-11ld", f, fFields[i]);
        if(fStamp[i] == kUnset) {
            fprintf(stderr, " (unset) ");
        } else if(fStamp[i] == kInternallySet) { 
            fprintf(stderr, " (internally set) ");
            
            
        } else {
            fprintf(stderr, " %%%d ", fStamp[i]);
        }
        fprintf(stderr, "\n");

    }
}

U_CFUNC void ucal_dump(UCalendar* cal) {
    ucal_dump( *((Calendar*)cal)  );
}
#endif

#endif


#define STAMP_MAX 10000

static const char * const gCalTypes[] = {
    "gregorian",
    "japanese",
    "buddhist",
    "roc",
    "persian",
    "islamic-civil",
    "islamic",
    "hebrew",
    "chinese",
    "indian",
    "coptic",
    "ethiopic",
    "ethiopic-amete-alem",
    "iso8601",
    NULL
};


typedef enum ECalType {
    CALTYPE_UNKNOWN = -1,
    CALTYPE_GREGORIAN = 0,
    CALTYPE_JAPANESE,
    CALTYPE_BUDDHIST,
    CALTYPE_ROC,
    CALTYPE_PERSIAN,
    CALTYPE_ISLAMIC_CIVIL,
    CALTYPE_ISLAMIC,
    CALTYPE_HEBREW,
    CALTYPE_CHINESE,
    CALTYPE_INDIAN,
    CALTYPE_COPTIC,
    CALTYPE_ETHIOPIC,
    CALTYPE_ETHIOPIC_AMETE_ALEM,
    CALTYPE_ISO8601
} ECalType;

U_NAMESPACE_BEGIN

static ECalType getCalendarType(const char *s) {
    for (int i = 0; gCalTypes[i] != NULL; i++) {
        if (uprv_stricmp(s, gCalTypes[i]) == 0) {
            return (ECalType)i;
        }
    }
    return CALTYPE_UNKNOWN;
}

static UBool isStandardSupportedKeyword(const char *keyword, UErrorCode& status) { 
    if(U_FAILURE(status)) {
        return FALSE;
    }
    ECalType calType = getCalendarType(keyword);
    return (calType != CALTYPE_UNKNOWN);
}

static void getCalendarKeyword(const UnicodeString &id, char *targetBuffer, int32_t targetBufferSize) {
    UnicodeString calendarKeyword = UNICODE_STRING_SIMPLE("calendar=");
    int32_t calKeyLen = calendarKeyword.length();
    int32_t keyLen = 0;

    int32_t keywordIdx = id.indexOf((UChar)0x003D); 
    if (id[0] == 0x40
        && id.compareBetween(1, keywordIdx+1, calendarKeyword, 0, calKeyLen) == 0)
    {
        keyLen = id.extract(keywordIdx+1, id.length(), targetBuffer, targetBufferSize, US_INV);
    }
    targetBuffer[keyLen] = 0;
}

static ECalType getCalendarTypeForLocale(const char *locid) {
    UErrorCode status = U_ZERO_ERROR;
    ECalType calType = CALTYPE_UNKNOWN;

    
    char canonicalName[256];

    
    
    int32_t canonicalLen = uloc_canonicalize(locid, canonicalName, sizeof(canonicalName) - 1, &status);
    if (U_FAILURE(status)) {
        return CALTYPE_GREGORIAN;
    }
    canonicalName[canonicalLen] = 0;    

    char calTypeBuf[32];
    int32_t calTypeBufLen;

    calTypeBufLen = uloc_getKeywordValue(canonicalName, "calendar", calTypeBuf, sizeof(calTypeBuf) - 1, &status);
    if (U_SUCCESS(status)) {
        calTypeBuf[calTypeBufLen] = 0;
        calType = getCalendarType(calTypeBuf);
        if (calType != CALTYPE_UNKNOWN) {
            return calType;
        }
    }
    status = U_ZERO_ERROR;

    
    
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLen = 0;
    regionLen = uloc_getCountry(canonicalName, region, sizeof(region) - 1, &status);
    if (regionLen == 0) {
        char fullLoc[256];
        uloc_addLikelySubtags(locid, fullLoc, sizeof(fullLoc) - 1, &status);
        regionLen = uloc_getCountry(fullLoc, region, sizeof(region) - 1, &status);
    }
    if (U_FAILURE(status)) {
        return CALTYPE_GREGORIAN;
    }
    region[regionLen] = 0;
    
    
    UResourceBundle *rb = ures_openDirect(NULL, "supplementalData", &status);
    ures_getByKey(rb, "calendarPreferenceData", rb, &status);
    UResourceBundle *order = ures_getByKey(rb, region, NULL, &status);
    if (status == U_MISSING_RESOURCE_ERROR && rb != NULL) {
        status = U_ZERO_ERROR;
        order = ures_getByKey(rb, "001", NULL, &status);
    }

    calTypeBuf[0] = 0;
    if (U_SUCCESS(status) && order != NULL) {
        
        int32_t len = 0;
        const UChar *uCalType = ures_getStringByIndex(order, 0, &len, &status);
        if (len < (int32_t)sizeof(calTypeBuf)) {
            u_UCharsToChars(uCalType, calTypeBuf, len);
            *(calTypeBuf + len) = 0; 
            calType = getCalendarType(calTypeBuf);
        }
    }

    ures_close(order);
    ures_close(rb);

    if (calType == CALTYPE_UNKNOWN) {
        
        calType = CALTYPE_GREGORIAN;
    }
    return calType;
}

static Calendar *createStandardCalendar(ECalType calType, const Locale &loc, UErrorCode& status) {
    Calendar *cal = NULL;

    switch (calType) {
        case CALTYPE_GREGORIAN:
            cal = new GregorianCalendar(loc, status);
            break;
        case CALTYPE_JAPANESE:
            cal = new JapaneseCalendar(loc, status);
            break;
        case CALTYPE_BUDDHIST:
            cal = new BuddhistCalendar(loc, status);
            break;
        case CALTYPE_ROC:
            cal = new TaiwanCalendar(loc, status);
            break;
        case CALTYPE_PERSIAN:
            cal = new PersianCalendar(loc, status);
            break;
        case CALTYPE_ISLAMIC_CIVIL:
            cal = new IslamicCalendar(loc, status, IslamicCalendar::CIVIL);
            break;
        case CALTYPE_ISLAMIC:
            cal = new IslamicCalendar(loc, status, IslamicCalendar::ASTRONOMICAL);
            break;
        case CALTYPE_HEBREW:
            cal = new HebrewCalendar(loc, status);
            break;
        case CALTYPE_CHINESE:
            cal = new ChineseCalendar(loc, status);
            break;
        case CALTYPE_INDIAN:
            cal = new IndianCalendar(loc, status);
            break;
        case CALTYPE_COPTIC:
            cal = new CopticCalendar(loc, status);
            break;
        case CALTYPE_ETHIOPIC:
            cal = new EthiopicCalendar(loc, status, EthiopicCalendar::AMETE_MIHRET_ERA);
            break;
        case CALTYPE_ETHIOPIC_AMETE_ALEM:
            cal = new EthiopicCalendar(loc, status, EthiopicCalendar::AMETE_ALEM_ERA);
            break;
        case CALTYPE_ISO8601:
            cal = new GregorianCalendar(loc, status);
            cal->setFirstDayOfWeek(UCAL_MONDAY);
            cal->setMinimalDaysInFirstWeek(4);
            break;
        default:
            status = U_UNSUPPORTED_ERROR;
    }
    return cal;
}


#if !UCONFIG_NO_SERVICE







class BasicCalendarFactory : public LocaleKeyFactory {
public:
    


    BasicCalendarFactory()
        : LocaleKeyFactory(LocaleKeyFactory::INVISIBLE) { }

    virtual ~BasicCalendarFactory();

protected:
    
    
    
    
    
    
    
    

    virtual void updateVisibleIDs(Hashtable& result, UErrorCode& status) const
    {
        if (U_SUCCESS(status)) {
            for(int32_t i=0;gCalTypes[i] != NULL;i++) {
                UnicodeString id((UChar)0x40); 
                id.append(UNICODE_STRING_SIMPLE("calendar="));
                id.append(UnicodeString(gCalTypes[i], -1, US_INV));
                result.put(id, (void*)this, status);
            }
        }
    }

    virtual UObject* create(const ICUServiceKey& key, const ICUService* , UErrorCode& status) const {
#ifdef U_DEBUG_CALSVC
        if(dynamic_cast<const LocaleKey*>(&key) == NULL) {
            fprintf(stderr, "::create - not a LocaleKey!\n");
        }
#endif
        const LocaleKey& lkey = (LocaleKey&)key;
        Locale curLoc;  
        Locale canLoc;  

        lkey.currentLocale(curLoc);
        lkey.canonicalLocale(canLoc);

        char keyword[ULOC_FULLNAME_CAPACITY];
        UnicodeString str;

        key.currentID(str);
        getCalendarKeyword(str, keyword, (int32_t) sizeof(keyword));

#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "BasicCalendarFactory::create() - cur %s, can %s\n", (const char*)curLoc.getName(), (const char*)canLoc.getName());
#endif

        if(!isStandardSupportedKeyword(keyword,status)) {  
#ifdef U_DEBUG_CALSVC

            fprintf(stderr, "BasicCalendarFactory - not handling %s.[%s]\n", (const char*) curLoc.getName(), tmp );
#endif
            return NULL;
        }

        return createStandardCalendar(getCalendarType(keyword), canLoc, status);
    }
};

BasicCalendarFactory::~BasicCalendarFactory() {}





class DefaultCalendarFactory : public ICUResourceBundleFactory {
public:
    DefaultCalendarFactory() : ICUResourceBundleFactory() { }
    virtual ~DefaultCalendarFactory();
protected:
    virtual UObject* create(const ICUServiceKey& key, const ICUService* , UErrorCode& status) const  {

        LocaleKey &lkey = (LocaleKey&)key;
        Locale loc;
        lkey.currentLocale(loc);

        UnicodeString *ret = new UnicodeString();
        if (ret == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            ret->append((UChar)0x40); 
            ret->append(UNICODE_STRING("calendar=", 9));
            ret->append(UnicodeString(gCalTypes[getCalendarTypeForLocale(loc.getName())], -1, US_INV));
        }
        return ret;
    }
};

DefaultCalendarFactory::~DefaultCalendarFactory() {}


class CalendarService : public ICULocaleService {
public:
    CalendarService()
        : ICULocaleService(UNICODE_STRING_SIMPLE("Calendar"))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new DefaultCalendarFactory(), status);
    }

    virtual ~CalendarService();

    virtual UObject* cloneInstance(UObject* instance) const {
        UnicodeString *s = dynamic_cast<UnicodeString *>(instance);
        if(s != NULL) {
            return s->clone(); 
        } else {
#ifdef U_DEBUG_CALSVC_F
            UErrorCode status2 = U_ZERO_ERROR;
            fprintf(stderr, "Cloning a %s calendar with tz=%ld\n", ((Calendar*)instance)->getType(), ((Calendar*)instance)->get(UCAL_ZONE_OFFSET, status2));
#endif
            return ((Calendar*)instance)->clone();
        }
    }

    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* , UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        

        Locale loc;
        lkey.canonicalLocale(loc);

#ifdef U_DEBUG_CALSVC
        Locale loc2;
        lkey.currentLocale(loc2);
        fprintf(stderr, "CalSvc:handleDefault for currentLoc %s, canloc %s\n", (const char*)loc.getName(),  (const char*)loc2.getName());
#endif
        Calendar *nc =  new GregorianCalendar(loc, status);

#ifdef U_DEBUG_CALSVC
        UErrorCode status2 = U_ZERO_ERROR;
        fprintf(stderr, "New default calendar has tz=%d\n", ((Calendar*)nc)->get(UCAL_ZONE_OFFSET, status2));
#endif
        return nc;
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

CalendarService::~CalendarService() {}



static inline UBool
isCalendarServiceUsed() {
    UBool retVal;
    UMTX_CHECK(NULL, gService != NULL, retVal);
    return retVal;
}



static ICULocaleService* 
getCalendarService(UErrorCode &status)
{
    UBool needInit;
    UMTX_CHECK(NULL, (UBool)(gService == NULL), needInit);
    if (needInit) {
#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "Spinning up Calendar Service\n");
#endif
        ICULocaleService * newservice = new CalendarService();
        if (newservice == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return newservice;
        }
#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "Registering classes..\n");
#endif

        
        newservice->registerFactory(new BasicCalendarFactory(),status);

#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "Done..\n");
#endif

        if(U_FAILURE(status)) {
#ifdef U_DEBUG_CALSVC
            fprintf(stderr, "err (%s) registering classes, deleting service.....\n", u_errorName(status));
#endif
            delete newservice;
            newservice = NULL;
        }

        if (newservice) {
            umtx_lock(NULL);
            if (gService == NULL) {
                gService = newservice;
                newservice = NULL;
            }
            umtx_unlock(NULL);
        }
        if (newservice) {
            delete newservice;
        } else {
            
            ucln_i18n_registerCleanup(UCLN_I18N_CALENDAR, calendar_cleanup);
        }
    }
    return gService;
}

URegistryKey Calendar::registerFactory(ICUServiceFactory* toAdopt, UErrorCode& status)
{
    return getCalendarService(status)->registerFactory(toAdopt, status);
}

UBool Calendar::unregister(URegistryKey key, UErrorCode& status) {
    return getCalendarService(status)->unregister(key, status);
}
#endif 



static const int32_t kCalendarLimits[UCAL_FIELD_COUNT][4] = {
    
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {-1,       -1,     -1,       -1}, 
    {           1,            1,             7,             7  }, 
    {-1,       -1,     -1,       -1}, 
    {           0,            0,             1,             1  }, 
    {           0,            0,            11,            11  }, 
    {           0,            0,            23,            23  }, 
    {           0,            0,            59,            59  }, 
    {           0,            0,            59,            59  }, 
    {           0,            0,           999,           999  }, 
    {-12*kOneHour, -12*kOneHour,   12*kOneHour,   15*kOneHour  }, 
    {           0,            0,    1*kOneHour,    1*kOneHour  }, 
    {-1,       -1,     -1,       -1}, 
    {           1,            1,             7,             7  }, 
    {-1,       -1,     -1,       -1}, 
    { -0x7F000000,  -0x7F000000,    0x7F000000,    0x7F000000  }, 
    {           0,            0, 24*kOneHour-1, 24*kOneHour-1  },  
    {           0,            0,             1,             1  },  
};


static const char gMonthNames[] = "monthNames";





















































Calendar::Calendar(UErrorCode& success)
:   UObject(),
fIsTimeSet(FALSE),
fAreFieldsSet(FALSE),
fAreAllFieldsSet(FALSE),
fAreFieldsVirtuallySet(FALSE),
fNextStamp((int32_t)kMinimumUserStamp),
fTime(0),
fLenient(TRUE),
fZone(0),
fRepeatedWallTime(UCAL_WALLTIME_LAST),
fSkippedWallTime(UCAL_WALLTIME_LAST)
{
    clear();
    fZone = TimeZone::createDefault();
    if (fZone == NULL) {
        success = U_MEMORY_ALLOCATION_ERROR;
    }
    setWeekData(Locale::getDefault(), NULL, success);
}



Calendar::Calendar(TimeZone* zone, const Locale& aLocale, UErrorCode& success)
:   UObject(),
fIsTimeSet(FALSE),
fAreFieldsSet(FALSE),
fAreAllFieldsSet(FALSE),
fAreFieldsVirtuallySet(FALSE),
fNextStamp((int32_t)kMinimumUserStamp),
fTime(0),
fLenient(TRUE),
fZone(0),
fRepeatedWallTime(UCAL_WALLTIME_LAST),
fSkippedWallTime(UCAL_WALLTIME_LAST)
{
    if(zone == 0) {
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d: ILLEGAL ARG because timezone cannot be 0\n",
            __FILE__, __LINE__);
#endif
        success = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    clear();    
    fZone = zone;

    setWeekData(aLocale, NULL, success);
}



Calendar::Calendar(const TimeZone& zone, const Locale& aLocale, UErrorCode& success)
:   UObject(),
fIsTimeSet(FALSE),
fAreFieldsSet(FALSE),
fAreAllFieldsSet(FALSE),
fAreFieldsVirtuallySet(FALSE),
fNextStamp((int32_t)kMinimumUserStamp),
fTime(0),
fLenient(TRUE),
fZone(0),
fRepeatedWallTime(UCAL_WALLTIME_LAST),
fSkippedWallTime(UCAL_WALLTIME_LAST)
{
    clear();
    fZone = zone.clone();
    if (fZone == NULL) {
    	success = U_MEMORY_ALLOCATION_ERROR;
    }
    setWeekData(aLocale, NULL, success);
}



Calendar::~Calendar()
{
    delete fZone;
}



Calendar::Calendar(const Calendar &source)
:   UObject(source)
{
    fZone = 0;
    *this = source;
}



Calendar &
Calendar::operator=(const Calendar &right)
{
    if (this != &right) {
        uprv_arrayCopy(right.fFields, fFields, UCAL_FIELD_COUNT);
        uprv_arrayCopy(right.fIsSet, fIsSet, UCAL_FIELD_COUNT);
        uprv_arrayCopy(right.fStamp, fStamp, UCAL_FIELD_COUNT);
        fTime                    = right.fTime;
        fIsTimeSet               = right.fIsTimeSet;
        fAreAllFieldsSet         = right.fAreAllFieldsSet;
        fAreFieldsSet            = right.fAreFieldsSet;
        fAreFieldsVirtuallySet   = right.fAreFieldsVirtuallySet;
        fLenient                 = right.fLenient;
        fRepeatedWallTime        = right.fRepeatedWallTime;
        fSkippedWallTime         = right.fSkippedWallTime;
        if (fZone != NULL) {
            delete fZone;
        }
        if (right.fZone != NULL) {
            fZone                = right.fZone->clone();
        }
        fFirstDayOfWeek          = right.fFirstDayOfWeek;
        fMinimalDaysInFirstWeek  = right.fMinimalDaysInFirstWeek;
        fWeekendOnset            = right.fWeekendOnset;
        fWeekendOnsetMillis      = right.fWeekendOnsetMillis;
        fWeekendCease            = right.fWeekendCease;
        fWeekendCeaseMillis      = right.fWeekendCeaseMillis;
        fNextStamp               = right.fNextStamp;
        uprv_strcpy(validLocale, right.validLocale);
        uprv_strcpy(actualLocale, right.actualLocale);
    }

    return *this;
}



Calendar* U_EXPORT2
Calendar::createInstance(UErrorCode& success)
{
    return createInstance(TimeZone::createDefault(), Locale::getDefault(), success);
}



Calendar* U_EXPORT2
Calendar::createInstance(const TimeZone& zone, UErrorCode& success)
{
    return createInstance(zone, Locale::getDefault(), success);
}



Calendar* U_EXPORT2
Calendar::createInstance(const Locale& aLocale, UErrorCode& success)
{
    return createInstance(TimeZone::createDefault(), aLocale, success);
}





Calendar* U_EXPORT2
Calendar::createInstance(TimeZone* zone, const Locale& aLocale, UErrorCode& success)
{
    if (U_FAILURE(success)) {
        return NULL;
    }

    Locale actualLoc;
    UObject* u = NULL;

#if !UCONFIG_NO_SERVICE
    if (isCalendarServiceUsed()) {
        u = getCalendarService(success)->get(aLocale, LocaleKey::KIND_ANY, &actualLoc, success);
    }
    else
#endif
    {
        u = createStandardCalendar(getCalendarTypeForLocale(aLocale.getName()), aLocale, success);
    }
    Calendar* c = NULL;

    if(U_FAILURE(success) || !u) {
        delete zone;
        if(U_SUCCESS(success)) { 
            success = U_INTERNAL_PROGRAM_ERROR;
        }
        return NULL;
    }

#if !UCONFIG_NO_SERVICE
    const UnicodeString* str = dynamic_cast<const UnicodeString*>(u);
    if(str != NULL) {
        
        
        Locale l("");
        LocaleUtility::initLocaleFromName(*str, l);

#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "Calendar::createInstance(%s), looking up [%s]\n", aLocale.getName(), l.getName());
#endif

        Locale actualLoc2;
        delete u;
        u = NULL;

        
        
        
        c = (Calendar*)getCalendarService(success)->get(l, LocaleKey::KIND_ANY, &actualLoc2, success);

        if(U_FAILURE(success) || !c) {
            delete zone;
            if(U_SUCCESS(success)) { 
                success = U_INTERNAL_PROGRAM_ERROR; 
            }
            return NULL;
        }

        str = dynamic_cast<const UnicodeString*>(c);
        if(str != NULL) {
            
            
#ifdef U_DEBUG_CALSVC
            char tmp[200];
            
            int32_t len = str->length();
            int32_t actLen = sizeof(tmp)-1;
            if(len > actLen) {
                len = actLen;
            }
            str->extract(0,len,tmp);
            tmp[len]=0;

            fprintf(stderr, "err - recursed, 2nd lookup was unistring %s\n", tmp);
#endif
            success = U_MISSING_RESOURCE_ERROR;  
            delete c;
            delete zone;
            return NULL;
        }
#ifdef U_DEBUG_CALSVC
        fprintf(stderr, "%p: setting week count data to locale %s, actual locale %s\n", c, (const char*)aLocale.getName(), (const char *)actualLoc.getName());
#endif
        c->setWeekData(aLocale, c->getType(), success);  

        char keyword[ULOC_FULLNAME_CAPACITY];
        UErrorCode tmpStatus = U_ZERO_ERROR;
        l.getKeywordValue("calendar", keyword, ULOC_FULLNAME_CAPACITY, tmpStatus);
        if (U_SUCCESS(tmpStatus) && uprv_strcmp(keyword, "iso8601") == 0) {
            c->setFirstDayOfWeek(UCAL_MONDAY);
            c->setMinimalDaysInFirstWeek(4);
        }
    }
    else
#endif 
    {
        
        c = (Calendar*)u;
    }

    
    c->adoptTimeZone(zone); 
    c->setTimeInMillis(getNow(), success); 

    return c;
}



Calendar* U_EXPORT2
Calendar::createInstance(const TimeZone& zone, const Locale& aLocale, UErrorCode& success)
{
    Calendar* c = createInstance(aLocale, success);
    if(U_SUCCESS(success) && c) {
        c->setTimeZone(zone);
    }
    return c; 
}



UBool
Calendar::operator==(const Calendar& that) const
{
    UErrorCode status = U_ZERO_ERROR;
    return isEquivalentTo(that) &&
        getTimeInMillis(status) == that.getTimeInMillis(status) &&
        U_SUCCESS(status);
}

UBool 
Calendar::isEquivalentTo(const Calendar& other) const
{
    return typeid(*this) == typeid(other) &&
        fLenient                == other.fLenient &&
        fRepeatedWallTime       == other.fRepeatedWallTime &&
        fSkippedWallTime        == other.fSkippedWallTime &&
        fFirstDayOfWeek         == other.fFirstDayOfWeek &&
        fMinimalDaysInFirstWeek == other.fMinimalDaysInFirstWeek &&
        fWeekendOnset           == other.fWeekendOnset &&
        fWeekendOnsetMillis     == other.fWeekendOnsetMillis &&
        fWeekendCease           == other.fWeekendCease &&
        fWeekendCeaseMillis     == other.fWeekendCeaseMillis &&
        *fZone                  == *other.fZone;
}



UBool
Calendar::equals(const Calendar& when, UErrorCode& status) const
{
    return (this == &when ||
        getTime(status) == when.getTime(status));
}



UBool
Calendar::before(const Calendar& when, UErrorCode& status) const
{
    return (this != &when &&
        getTimeInMillis(status) < when.getTimeInMillis(status));
}



UBool
Calendar::after(const Calendar& when, UErrorCode& status) const
{
    return (this != &when &&
        getTimeInMillis(status) > when.getTimeInMillis(status));
}




const Locale* U_EXPORT2
Calendar::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}



StringEnumeration* U_EXPORT2
Calendar::getKeywordValuesForLocale(const char* key,
                    const Locale& locale, UBool commonlyUsed, UErrorCode& status)
{
    
    UEnumeration *uenum = ucal_getKeywordValuesForLocale(key, locale.getName(),
                                                        commonlyUsed, &status);
    if (U_FAILURE(status)) {
        uenum_close(uenum);
        return NULL;
    }
    return new UStringEnumeration(uenum);
}



UDate U_EXPORT2
Calendar::getNow()
{
    return uprv_getUTCtime(); 
}







double 
Calendar::getTimeInMillis(UErrorCode& status) const
{
    if(U_FAILURE(status)) 
        return 0.0;

    if ( ! fIsTimeSet) 
        ((Calendar*)this)->updateTime(status);

    
    if(U_FAILURE(status)) {
        return 0.0;
    }
    return fTime;
}










void 
Calendar::setTimeInMillis( double millis, UErrorCode& status ) {
    if(U_FAILURE(status)) 
        return;

    if (millis > MAX_MILLIS) {
        if(isLenient()) {
            millis = MAX_MILLIS;
        } else {
		    status = U_ILLEGAL_ARGUMENT_ERROR;
		    return;
        }
    } else if (millis < MIN_MILLIS) {
        if(isLenient()) {
            millis = MIN_MILLIS;
        } else {
    		status = U_ILLEGAL_ARGUMENT_ERROR;
	    	return;
        }
    }

    fTime = millis;
    fAreFieldsSet = fAreAllFieldsSet = FALSE;
    fIsTimeSet = fAreFieldsVirtuallySet = TRUE;

    for (int32_t i=0; i<UCAL_FIELD_COUNT; ++i) {
        fFields[i]     = 0;
        fStamp[i]     = kUnset;
        fIsSet[i]     = FALSE;
    }
    

}



int32_t
Calendar::get(UCalendarDateFields field, UErrorCode& status) const
{
    
    
    
    if (U_SUCCESS(status)) ((Calendar*)this)->complete(status); 
    return U_SUCCESS(status) ? fFields[field] : 0;
}



void
Calendar::set(UCalendarDateFields field, int32_t value)
{
    if (fAreFieldsVirtuallySet) {
        UErrorCode ec = U_ZERO_ERROR;
        computeFields(ec);
    }
    fFields[field]     = value;
    
    if (fNextStamp == STAMP_MAX) {
        recalculateStamp();
    }
    fStamp[field]     = fNextStamp++;
    fIsSet[field]     = TRUE; 
    fIsTimeSet = fAreFieldsSet = fAreFieldsVirtuallySet = FALSE;
}



void
Calendar::set(int32_t year, int32_t month, int32_t date)
{
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
}



void
Calendar::set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute)
{
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
    set(UCAL_HOUR_OF_DAY, hour);
    set(UCAL_MINUTE, minute);
}



void
Calendar::set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second)
{
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
    set(UCAL_HOUR_OF_DAY, hour);
    set(UCAL_MINUTE, minute);
    set(UCAL_SECOND, second);
}



void
Calendar::clear()
{
    for (int32_t i=0; i<UCAL_FIELD_COUNT; ++i) {
        fFields[i]     = 0; 
        fStamp[i]     = kUnset;
        fIsSet[i]     = FALSE; 
    }
    fIsTimeSet = fAreFieldsSet = fAreAllFieldsSet = fAreFieldsVirtuallySet = FALSE;
    
}



void
Calendar::clear(UCalendarDateFields field)
{
    if (fAreFieldsVirtuallySet) {
        UErrorCode ec = U_ZERO_ERROR;
        computeFields(ec);
    }
    fFields[field]         = 0;
    fStamp[field]         = kUnset;
    fIsSet[field]         = FALSE; 
    fIsTimeSet = fAreFieldsSet = fAreAllFieldsSet = fAreFieldsVirtuallySet = FALSE;
}



UBool
Calendar::isSet(UCalendarDateFields field) const
{
    return fAreFieldsVirtuallySet || (fStamp[field] != kUnset);
}


int32_t Calendar::newestStamp(UCalendarDateFields first, UCalendarDateFields last, int32_t bestStampSoFar) const
{
    int32_t bestStamp = bestStampSoFar;
    for (int32_t i=(int32_t)first; i<=(int32_t)last; ++i) {
        if (fStamp[i] > bestStamp) {
            bestStamp = fStamp[i];
        }
    }
    return bestStamp;
}




void
Calendar::complete(UErrorCode& status)
{
    if (!fIsTimeSet) {
        updateTime(status);
        
        if(U_FAILURE(status)) {
            return;
        }
    }
    if (!fAreFieldsSet) {
        computeFields(status); 
        
        if(U_FAILURE(status)) {
            return;
        }
        fAreFieldsSet         = TRUE;
        fAreAllFieldsSet     = TRUE;
    }
}

































void Calendar::pinField(UCalendarDateFields field, UErrorCode& status) {
    int32_t max = getActualMaximum(field, status);
    int32_t min = getActualMinimum(field, status);

    if (fFields[field] > max) {
        set(field, max);
    } else if (fFields[field] < min) {
        set(field, min);
    }
}


void Calendar::computeFields(UErrorCode &ec)
{
  if (U_FAILURE(ec)) {
        return;
    }
    
    double localMillis = internalGetTime();
    int32_t rawOffset, dstOffset;
    getTimeZone().getOffset(localMillis, FALSE, rawOffset, dstOffset, ec);
    localMillis += (rawOffset + dstOffset); 

    
    uint32_t mask =   
        (1 << UCAL_ERA) |
        (1 << UCAL_YEAR) |
        (1 << UCAL_MONTH) |
        (1 << UCAL_DAY_OF_MONTH) | 
        (1 << UCAL_DAY_OF_YEAR) |
        (1 << UCAL_EXTENDED_YEAR);  

    for (int32_t i=0; i<UCAL_FIELD_COUNT; ++i) {
        if ((mask & 1) == 0) {
            fStamp[i] = kInternallySet;
            fIsSet[i] = TRUE; 
        } else {
            fStamp[i] = kUnset;
            fIsSet[i] = FALSE; 
        }
        mask >>= 1;
    }

    
    
    
    
    
    
    
    

    int32_t days =  (int32_t)ClockMath::floorDivide(localMillis, (double)kOneDay);

    internalSet(UCAL_JULIAN_DAY,days + kEpochStartAsJulianDay);

#if defined (U_DEBUG_CAL)
    
    
#endif  

    computeGregorianAndDOWFields(fFields[UCAL_JULIAN_DAY], ec);

    
    
    
    
    handleComputeFields(fFields[UCAL_JULIAN_DAY], ec);

    
    
    computeWeekFields(ec);

    
    
    
    int32_t millisInDay =  (int32_t) (localMillis - (days * kOneDay));
    fFields[UCAL_MILLISECONDS_IN_DAY] = millisInDay;
    fFields[UCAL_MILLISECOND] = millisInDay % 1000;
    millisInDay /= 1000;
    fFields[UCAL_SECOND] = millisInDay % 60;
    millisInDay /= 60;
    fFields[UCAL_MINUTE] = millisInDay % 60;
    millisInDay /= 60;
    fFields[UCAL_HOUR_OF_DAY] = millisInDay;
    fFields[UCAL_AM_PM] = millisInDay / 12; 
    fFields[UCAL_HOUR] = millisInDay % 12;
    fFields[UCAL_ZONE_OFFSET] = rawOffset;
    fFields[UCAL_DST_OFFSET] = dstOffset;
}

uint8_t Calendar::julianDayToDayOfWeek(double julian)
{
    
    
    int8_t dayOfWeek = (int8_t) uprv_fmod(julian + 1, 7);

    uint8_t result = (uint8_t)(dayOfWeek + ((dayOfWeek < 0) ? (7+UCAL_SUNDAY ) : UCAL_SUNDAY));
    return result;
}







void Calendar::computeGregorianAndDOWFields(int32_t julianDay, UErrorCode &ec)
{
    computeGregorianFields(julianDay, ec);

    
    int32_t dow = julianDayToDayOfWeek(julianDay);
    internalSet(UCAL_DAY_OF_WEEK,dow);

    
    int32_t dowLocal = dow - getFirstDayOfWeek() + 1;
    if (dowLocal < 1) {
        dowLocal += 7;
    }
    internalSet(UCAL_DOW_LOCAL,dowLocal);
    fFields[UCAL_DOW_LOCAL] = dowLocal;
}








void Calendar::computeGregorianFields(int32_t julianDay, UErrorCode & ) {
    int32_t gregorianDayOfWeekUnused;
    Grego::dayToFields(julianDay - kEpochStartAsJulianDay, fGregorianYear, fGregorianMonth, fGregorianDayOfMonth, gregorianDayOfWeekUnused, fGregorianDayOfYear);
}





















void Calendar::computeWeekFields(UErrorCode &ec) {
    if(U_FAILURE(ec)) { 
        return;
    }
    int32_t eyear = fFields[UCAL_EXTENDED_YEAR];
    int32_t dayOfWeek = fFields[UCAL_DAY_OF_WEEK];
    int32_t dayOfYear = fFields[UCAL_DAY_OF_YEAR];

    
    
    
    
    
    
    
    
    
    int32_t yearOfWeekOfYear = eyear;
    int32_t relDow = (dayOfWeek + 7 - getFirstDayOfWeek()) % 7; 
    int32_t relDowJan1 = (dayOfWeek - dayOfYear + 7001 - getFirstDayOfWeek()) % 7; 
    int32_t woy = (dayOfYear - 1 + relDowJan1) / 7; 
    if ((7 - relDowJan1) >= getMinimalDaysInFirstWeek()) {
        ++woy;
    }

    
    
    if (woy == 0) {
        
        
        
        

        int32_t prevDoy = dayOfYear + handleGetYearLength(eyear - 1);
        woy = weekNumber(prevDoy, dayOfWeek);
        yearOfWeekOfYear--;
    } else {
        int32_t lastDoy = handleGetYearLength(eyear);
        
        
        
        
        
        
        if (dayOfYear >= (lastDoy - 5)) {
            int32_t lastRelDow = (relDow + lastDoy - dayOfYear) % 7;
            if (lastRelDow < 0) {
                lastRelDow += 7;
            }
            if (((6 - lastRelDow) >= getMinimalDaysInFirstWeek()) &&
                ((dayOfYear + 7 - relDow) > lastDoy)) {
                    woy = 1;
                    yearOfWeekOfYear++;
                }
        }
    }
    fFields[UCAL_WEEK_OF_YEAR] = woy;
    fFields[UCAL_YEAR_WOY] = yearOfWeekOfYear;
    

    int32_t dayOfMonth = fFields[UCAL_DAY_OF_MONTH];
    fFields[UCAL_WEEK_OF_MONTH] = weekNumber(dayOfMonth, dayOfWeek);
    fFields[UCAL_DAY_OF_WEEK_IN_MONTH] = (dayOfMonth-1) / 7 + 1;
#if defined (U_DEBUG_CAL)
    if(fFields[UCAL_DAY_OF_WEEK_IN_MONTH]==0) fprintf(stderr, "%s:%d: DOWIM %d on %g\n", 
        __FILE__, __LINE__,fFields[UCAL_DAY_OF_WEEK_IN_MONTH], fTime);
#endif
}


int32_t Calendar::weekNumber(int32_t desiredDay, int32_t dayOfPeriod, int32_t dayOfWeek)
{
    
    
    
    int32_t periodStartDayOfWeek = (dayOfWeek - getFirstDayOfWeek() - dayOfPeriod + 1) % 7;
    if (periodStartDayOfWeek < 0) periodStartDayOfWeek += 7;

    
    
    
    int32_t weekNo = (desiredDay + periodStartDayOfWeek - 1)/7;

    
    
    
    if ((7 - periodStartDayOfWeek) >= getMinimalDaysInFirstWeek()) ++weekNo;

    return weekNo;
}

void Calendar::handleComputeFields(int32_t , UErrorCode &)
{
    internalSet(UCAL_MONTH, getGregorianMonth());
    internalSet(UCAL_DAY_OF_MONTH, getGregorianDayOfMonth());
    internalSet(UCAL_DAY_OF_YEAR, getGregorianDayOfYear());
    int32_t eyear = getGregorianYear();
    internalSet(UCAL_EXTENDED_YEAR, eyear);
    int32_t era = GregorianCalendar::AD;
    if (eyear < 1) {
        era = GregorianCalendar::BC;
        eyear = 1 - eyear;
    }
    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, eyear);
}



void Calendar::roll(EDateFields field, int32_t amount, UErrorCode& status) 
{
    roll((UCalendarDateFields)field, amount, status);
}

void Calendar::roll(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if (amount == 0) {
        return; 
    }

    complete(status);

    if(U_FAILURE(status)) {
        return;
    }
    switch (field) {
    case UCAL_DAY_OF_MONTH:
    case UCAL_AM_PM:
    case UCAL_MINUTE:
    case UCAL_SECOND:
    case UCAL_MILLISECOND:
    case UCAL_MILLISECONDS_IN_DAY:
    case UCAL_ERA:
        
        
        
        {
            int32_t min = getActualMinimum(field,status);
            int32_t max = getActualMaximum(field,status);
            int32_t gap = max - min + 1;

            int32_t value = internalGet(field) + amount;
            value = (value - min) % gap;
            if (value < 0) {
                value += gap;
            }
            value += min;

            set(field, value);
            return;
        }

    case UCAL_HOUR:
    case UCAL_HOUR_OF_DAY:
        
        
        
        
        
        
        
        {
            
            double start = getTimeInMillis(status);
            int32_t oldHour = internalGet(field);
            int32_t max = getMaximum(field);
            int32_t newHour = (oldHour + amount) % (max + 1);
            if (newHour < 0) {
                newHour += max + 1;
            }
            setTimeInMillis(start + kOneHour * (newHour - oldHour),status);
            return;
        }

    case UCAL_MONTH:
        
        
        
        
        {
            int32_t max = getActualMaximum(UCAL_MONTH, status);
            int32_t mon = (internalGet(UCAL_MONTH) + amount) % (max+1);

            if (mon < 0) {
                mon += (max + 1);
            }
            set(UCAL_MONTH, mon);

            
            
            
            pinField(UCAL_DAY_OF_MONTH,status);
            return;
        }

    case UCAL_YEAR:
    case UCAL_YEAR_WOY:
        {
            
            
            
            UBool era0WithYearsThatGoBackwards = FALSE;
            int32_t era = get(UCAL_ERA, status);
            if (era == 0) {
                const char * calType = getType();
                if ( uprv_strcmp(calType,"gregorian")==0 || uprv_strcmp(calType,"roc")==0 || uprv_strcmp(calType,"coptic")==0 ) {
                    amount = -amount;
                    era0WithYearsThatGoBackwards = TRUE;
                }
            }
            int32_t newYear = internalGet(field) + amount;
            if (era > 0 || newYear >= 1) {
                int32_t maxYear = getActualMaximum(field, status);
                if (maxYear < 32768) {
                    
                    if (newYear < 1) {
                        newYear = maxYear - ((-newYear) % maxYear);
                    } else if (newYear > maxYear) {
                        newYear = ((newYear - 1) % maxYear) + 1;
                    }
                
                } else if (newYear < 1) {
                    newYear = 1;
                }
            
            
            
            } else if (era0WithYearsThatGoBackwards) {
                newYear = 1;
            }
            set(field, newYear);
            pinField(UCAL_MONTH,status);
            pinField(UCAL_DAY_OF_MONTH,status);
            return;
        }

    case UCAL_EXTENDED_YEAR:
        
        set(field, internalGet(field) + amount);
        pinField(UCAL_MONTH,status);
        pinField(UCAL_DAY_OF_MONTH,status);
        return;

    case UCAL_WEEK_OF_MONTH:
        {
            
            

            
            
            

            
            
            

            
            
            
            
            

            
            
            
            
            
            
            
            
            

            
            
            int32_t dow = internalGet(UCAL_DAY_OF_WEEK) - getFirstDayOfWeek();
            if (dow < 0) dow += 7;

            
            
            int32_t fdm = (dow - internalGet(UCAL_DAY_OF_MONTH) + 1) % 7;
            if (fdm < 0) fdm += 7;

            
            
            
            
            int32_t start;
            if ((7 - fdm) < getMinimalDaysInFirstWeek())
                start = 8 - fdm; 
            else
                start = 1 - fdm; 

            
            
            int32_t monthLen = getActualMaximum(UCAL_DAY_OF_MONTH, status);
            int32_t ldm = (monthLen - internalGet(UCAL_DAY_OF_MONTH) + dow) % 7;
            

            
            
            
            
            int32_t limit = monthLen + 7 - ldm;

            
            int32_t gap = limit - start;
            int32_t day_of_month = (internalGet(UCAL_DAY_OF_MONTH) + amount*7 -
                start) % gap;
            if (day_of_month < 0) day_of_month += gap;
            day_of_month += start;

            
            if (day_of_month < 1) day_of_month = 1;
            if (day_of_month > monthLen) day_of_month = monthLen;

            
            
            
            
            
            
            set(UCAL_DAY_OF_MONTH, day_of_month);
            return;
        }
    case UCAL_WEEK_OF_YEAR:
        {
            
            
            

            
            
            int32_t dow = internalGet(UCAL_DAY_OF_WEEK) - getFirstDayOfWeek();
            if (dow < 0) dow += 7;

            
            
            int32_t fdy = (dow - internalGet(UCAL_DAY_OF_YEAR) + 1) % 7;
            if (fdy < 0) fdy += 7;

            
            
            
            
            int32_t start;
            if ((7 - fdy) < getMinimalDaysInFirstWeek())
                start = 8 - fdy; 
            else
                start = 1 - fdy; 

            
            
            int32_t yearLen = getActualMaximum(UCAL_DAY_OF_YEAR,status);
            int32_t ldy = (yearLen - internalGet(UCAL_DAY_OF_YEAR) + dow) % 7;
            

            
            
            
            
            int32_t limit = yearLen + 7 - ldy;

            
            int32_t gap = limit - start;
            int32_t day_of_year = (internalGet(UCAL_DAY_OF_YEAR) + amount*7 -
                start) % gap;
            if (day_of_year < 0) day_of_year += gap;
            day_of_year += start;

            
            if (day_of_year < 1) day_of_year = 1;
            if (day_of_year > yearLen) day_of_year = yearLen;

            
            
            
            
            set(UCAL_DAY_OF_YEAR, day_of_year);
            clear(UCAL_MONTH);
            return;
        }
    case UCAL_DAY_OF_YEAR:
        {
            
            
            double delta = amount * kOneDay; 
            double min2 = internalGet(UCAL_DAY_OF_YEAR)-1;
            min2 *= kOneDay;
            min2 = internalGetTime() - min2;

            
            double newtime;

            double yearLength = getActualMaximum(UCAL_DAY_OF_YEAR,status);
            double oneYear = yearLength;
            oneYear *= kOneDay;
            newtime = uprv_fmod((internalGetTime() + delta - min2), oneYear);
            if (newtime < 0) newtime += oneYear;
            setTimeInMillis(newtime + min2, status);
            return;
        }
    case UCAL_DAY_OF_WEEK:
    case UCAL_DOW_LOCAL:
        {
            
            
            
            double delta = amount * kOneDay; 
            
            
            int32_t leadDays = internalGet(field);
            leadDays -= (field == UCAL_DAY_OF_WEEK) ? getFirstDayOfWeek() : 1;
            if (leadDays < 0) leadDays += 7;
            double min2 = internalGetTime() - leadDays * kOneDay;
            double newtime = uprv_fmod((internalGetTime() + delta - min2), kOneWeek);
            if (newtime < 0) newtime += kOneWeek;
            setTimeInMillis(newtime + min2, status);
            return;
        }
    case UCAL_DAY_OF_WEEK_IN_MONTH:
        {
            
            
            
            double delta = amount * kOneWeek; 
            
            
            int32_t preWeeks = (internalGet(UCAL_DAY_OF_MONTH) - 1) / 7;
            
            
            int32_t postWeeks = (getActualMaximum(UCAL_DAY_OF_MONTH,status) -
                internalGet(UCAL_DAY_OF_MONTH)) / 7;
            
            double min2 = internalGetTime() - preWeeks * kOneWeek;
            double gap2 = kOneWeek * (preWeeks + postWeeks + 1); 
            
            double newtime = uprv_fmod((internalGetTime() + delta - min2), gap2);
            if (newtime < 0) newtime += gap2;
            setTimeInMillis(newtime + min2, status);
            return;
        }
    case UCAL_JULIAN_DAY:
        set(field, internalGet(field) + amount);
        return;
    default:
        
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d: ILLEGAL ARG because of roll on non-rollable field %s\n", 
            __FILE__, __LINE__,fldName(field));
#endif
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

void Calendar::add(EDateFields field, int32_t amount, UErrorCode& status)
{
    Calendar::add((UCalendarDateFields)field, amount, status);
}


void Calendar::add(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if (amount == 0) {
        return;   
    }

    
    
    
    
    
    
    
    

    
    
    
    
    
    
    

    
    
    

    double delta = amount; 
    UBool keepHourInvariant = TRUE;

    switch (field) {
    case UCAL_ERA:
        set(field, get(field, status) + amount);
        pinField(UCAL_ERA, status);
        return;

    case UCAL_YEAR:
    case UCAL_YEAR_WOY:
      {
        
        
        
        
        
        
        
        int32_t era = get(UCAL_ERA, status);
        if (era == 0) {
          const char * calType = getType();
          if ( uprv_strcmp(calType,"gregorian")==0 || uprv_strcmp(calType,"roc")==0 || uprv_strcmp(calType,"coptic")==0 ) {
            amount = -amount;
          }
        }
      }
      
    case UCAL_EXTENDED_YEAR:
    case UCAL_MONTH:
      {
        UBool oldLenient = isLenient();
        setLenient(TRUE);
        set(field, get(field, status) + amount);
        pinField(UCAL_DAY_OF_MONTH, status);
        if(oldLenient==FALSE) {
          complete(status); 
          setLenient(oldLenient);
        }
      }
      return;

    case UCAL_WEEK_OF_YEAR:
    case UCAL_WEEK_OF_MONTH:
    case UCAL_DAY_OF_WEEK_IN_MONTH:
        delta *= kOneWeek;
        break;

    case UCAL_AM_PM:
        delta *= 12 * kOneHour;
        break;

    case UCAL_DAY_OF_MONTH:
    case UCAL_DAY_OF_YEAR:
    case UCAL_DAY_OF_WEEK:
    case UCAL_DOW_LOCAL:
    case UCAL_JULIAN_DAY:
        delta *= kOneDay;
        break;

    case UCAL_HOUR_OF_DAY:
    case UCAL_HOUR:
        delta *= kOneHour;
        keepHourInvariant = FALSE;
        break;

    case UCAL_MINUTE:
        delta *= kOneMinute;
        keepHourInvariant = FALSE;
        break;

    case UCAL_SECOND:
        delta *= kOneSecond;
        keepHourInvariant = FALSE;
        break;

    case UCAL_MILLISECOND:
    case UCAL_MILLISECONDS_IN_DAY:
        keepHourInvariant = FALSE;
        break;

    default:
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d: ILLEGAL ARG because field %s not addable",
            __FILE__, __LINE__, fldName(field));
#endif
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
        
        
    }

    
    
    
    
    int32_t prevOffset = 0;
    int32_t hour = 0;
    if (keepHourInvariant) {
        prevOffset = get(UCAL_DST_OFFSET, status) + get(UCAL_ZONE_OFFSET, status);
        hour = internalGet(UCAL_HOUR_OF_DAY);
    }

    setTimeInMillis(getTimeInMillis(status) + delta, status);

    if (keepHourInvariant) {
        int32_t newOffset = get(UCAL_DST_OFFSET, status) + get(UCAL_ZONE_OFFSET, status);
        if (newOffset != prevOffset) {
            
            
            
            
            
            
            

            
            
            
            
            int32_t adjAmount = prevOffset - newOffset;
            adjAmount = adjAmount >= 0 ? adjAmount % (int32_t)kOneDay : -(-adjAmount % (int32_t)kOneDay);
            if (adjAmount != 0) {
                double t = internalGetTime();
                setTimeInMillis(t + adjAmount, status);
                if (get(UCAL_HOUR_OF_DAY, status) != hour) {
                    setTimeInMillis(t, status);
                }
            }
        }
    } 
}


int32_t Calendar::fieldDifference(UDate when, EDateFields field, UErrorCode& status) {
    return fieldDifference(when, (UCalendarDateFields) field, status);
}

int32_t Calendar::fieldDifference(UDate targetMs, UCalendarDateFields field, UErrorCode& ec) {
    if (U_FAILURE(ec)) return 0;
    int32_t min = 0;
    double startMs = getTimeInMillis(ec);
    
    
    
    
    
    
    if (startMs < targetMs) {
        int32_t max = 1;
        
        while (U_SUCCESS(ec)) {
            setTimeInMillis(startMs, ec);
            add(field, max, ec);
            double ms = getTimeInMillis(ec);
            if (ms == targetMs) {
                return max;
            } else if (ms > targetMs) {
                break;
            } else if (max < INT32_MAX) {
                min = max;
                max <<= 1;
                if (max < 0) {
                    max = INT32_MAX;
                }
            } else {
                
#if defined (U_DEBUG_CAL)
                fprintf(stderr, "%s:%d: ILLEGAL ARG because field %s's max too large for int32_t\n",
                    __FILE__, __LINE__, fldName(field));
#endif
                ec = U_ILLEGAL_ARGUMENT_ERROR;
            }
        }
        
        while ((max - min) > 1 && U_SUCCESS(ec)) {
            int32_t t = min + (max - min)/2; 
            setTimeInMillis(startMs, ec);
            add(field, t, ec);
            double ms = getTimeInMillis(ec);
            if (ms == targetMs) {
                return t;
            } else if (ms > targetMs) {
                max = t;
            } else {
                min = t;
            }
        }
    } else if (startMs > targetMs) {
        int32_t max = -1;
        
        while (U_SUCCESS(ec)) {
            setTimeInMillis(startMs, ec);
            add(field, max, ec);
            double ms = getTimeInMillis(ec);
            if (ms == targetMs) {
                return max;
            } else if (ms < targetMs) {
                break;
            } else {
                min = max;
                max <<= 1;
                if (max == 0) {
                    
#if defined (U_DEBUG_CAL)
                    fprintf(stderr, "%s:%d: ILLEGAL ARG because field %s's max too large for int32_t\n",
                        __FILE__, __LINE__, fldName(field));
#endif
                    ec = U_ILLEGAL_ARGUMENT_ERROR;
                }
            }
        }
        
        while ((min - max) > 1 && U_SUCCESS(ec)) {
            int32_t t = min + (max - min)/2; 
            setTimeInMillis(startMs, ec);
            add(field, t, ec);
            double ms = getTimeInMillis(ec);
            if (ms == targetMs) {
                return t;
            } else if (ms < targetMs) {
                max = t;
            } else {
                min = t;
            }
        }
    }
    
    setTimeInMillis(startMs, ec);
    add(field, min, ec);

    
    if(U_FAILURE(ec)) {
        return 0;
    }
    return min;
}



void
Calendar::adoptTimeZone(TimeZone* zone)
{
    
    if (zone == NULL) return;

    
    if (fZone != NULL) delete fZone;
    fZone = zone;

    
    fAreFieldsSet = FALSE;
}


void
Calendar::setTimeZone(const TimeZone& zone)
{
    adoptTimeZone(zone.clone());
}



const TimeZone&
Calendar::getTimeZone() const
{
    return *fZone;
}



TimeZone*
Calendar::orphanTimeZone()
{
    TimeZone *z = fZone;
    
    fZone = TimeZone::createDefault();
    return z;
}



void
Calendar::setLenient(UBool lenient)
{
    fLenient = lenient;
}



UBool
Calendar::isLenient() const
{
    return fLenient;
}



void
Calendar::setRepeatedWallTimeOption(UCalendarWallTimeOption option)
{
    if (option == UCAL_WALLTIME_LAST || option == UCAL_WALLTIME_FIRST) {
        fRepeatedWallTime = option;
    }
}



UCalendarWallTimeOption
Calendar::getRepeatedWallTimeOption(void) const
{
    return fRepeatedWallTime;
}



void
Calendar::setSkippedWallTimeOption(UCalendarWallTimeOption option)
{
    fSkippedWallTime = option;
}



UCalendarWallTimeOption
Calendar::getSkippedWallTimeOption(void) const
{
    return fSkippedWallTime;
}



void
Calendar::setFirstDayOfWeek(UCalendarDaysOfWeek value)
{
    if (fFirstDayOfWeek != value &&
        value >= UCAL_SUNDAY && value <= UCAL_SATURDAY) {
            fFirstDayOfWeek = value;
            fAreFieldsSet = FALSE;
        }
}



Calendar::EDaysOfWeek
Calendar::getFirstDayOfWeek() const
{
    return (Calendar::EDaysOfWeek)fFirstDayOfWeek;
}

UCalendarDaysOfWeek
Calendar::getFirstDayOfWeek(UErrorCode & ) const
{
    return fFirstDayOfWeek;
}


void
Calendar::setMinimalDaysInFirstWeek(uint8_t value)
{
    
    
    
    if (value < 1) {
        value = 1;
    } else if (value > 7) {
        value = 7;
    }
    if (fMinimalDaysInFirstWeek != value) {
        fMinimalDaysInFirstWeek = value;
        fAreFieldsSet = FALSE;
    }
}



uint8_t
Calendar::getMinimalDaysInFirstWeek() const
{
    return fMinimalDaysInFirstWeek;
}




UCalendarWeekdayType
Calendar::getDayOfWeekType(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const
{
    if (U_FAILURE(status)) {
        return UCAL_WEEKDAY;
    }
    if (dayOfWeek < UCAL_SUNDAY || dayOfWeek > UCAL_SATURDAY) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return UCAL_WEEKDAY;
    }
    if (fWeekendOnset < fWeekendCease) {
        if (dayOfWeek < fWeekendOnset || dayOfWeek > fWeekendCease) {
            return UCAL_WEEKDAY;
        }
    } else {
        if (dayOfWeek > fWeekendCease && dayOfWeek < fWeekendOnset) {
            return UCAL_WEEKDAY;
        }
    }
    if (dayOfWeek == fWeekendOnset) {
        return (fWeekendOnsetMillis == 0) ? UCAL_WEEKEND : UCAL_WEEKEND_ONSET;
    }
    if (dayOfWeek == fWeekendCease) {
        return (fWeekendCeaseMillis == 0) ? UCAL_WEEKDAY : UCAL_WEEKEND_CEASE;
    }
    return UCAL_WEEKEND;
}

int32_t
Calendar::getWeekendTransition(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
    if (dayOfWeek == fWeekendOnset) {
        return fWeekendOnsetMillis;
    } else if (dayOfWeek == fWeekendCease) {
        return fWeekendCeaseMillis;
    }
    status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
}

UBool
Calendar::isWeekend(UDate date, UErrorCode &status) const
{
    if (U_FAILURE(status)) {
        return FALSE;
    }
    
    Calendar *work = (Calendar*)this->clone();
    if (work == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    UBool result = FALSE;
    work->setTime(date, status);
    if (U_SUCCESS(status)) {
        result = work->isWeekend();
    }
    delete work;
    return result;
}

UBool
Calendar::isWeekend(void) const
{
    UErrorCode status = U_ZERO_ERROR;
    UCalendarDaysOfWeek dayOfWeek = (UCalendarDaysOfWeek)get(UCAL_DAY_OF_WEEK, status);
    UCalendarWeekdayType dayType = getDayOfWeekType(dayOfWeek, status);
    if (U_SUCCESS(status)) {
        switch (dayType) {
            case UCAL_WEEKDAY:
                return FALSE;
            case UCAL_WEEKEND:
                return TRUE;
            case UCAL_WEEKEND_ONSET:
            case UCAL_WEEKEND_CEASE:
                
                {
                    int32_t millisInDay = internalGet(UCAL_MILLISECONDS_IN_DAY);
                    int32_t transitionMillis = getWeekendTransition(dayOfWeek, status);
                    if (U_SUCCESS(status)) {
                        return (dayType == UCAL_WEEKEND_ONSET)?
                            (millisInDay >= transitionMillis):
                            (millisInDay <  transitionMillis);
                    }
                    
                }
            default:
                break;
        }
    }
    return FALSE;
}



int32_t 
Calendar::getMinimum(EDateFields field) const {
    return getLimit((UCalendarDateFields) field,UCAL_LIMIT_MINIMUM);
}

int32_t
Calendar::getMinimum(UCalendarDateFields field) const
{
    return getLimit(field,UCAL_LIMIT_MINIMUM);
}


int32_t
Calendar::getMaximum(EDateFields field) const
{
    return getLimit((UCalendarDateFields) field,UCAL_LIMIT_MAXIMUM);
}

int32_t
Calendar::getMaximum(UCalendarDateFields field) const
{
    return getLimit(field,UCAL_LIMIT_MAXIMUM);
}


int32_t
Calendar::getGreatestMinimum(EDateFields field) const
{
    return getLimit((UCalendarDateFields)field,UCAL_LIMIT_GREATEST_MINIMUM);
}

int32_t
Calendar::getGreatestMinimum(UCalendarDateFields field) const
{
    return getLimit(field,UCAL_LIMIT_GREATEST_MINIMUM);
}


int32_t
Calendar::getLeastMaximum(EDateFields field) const
{
    return getLimit((UCalendarDateFields) field,UCAL_LIMIT_LEAST_MAXIMUM);
}

int32_t
Calendar::getLeastMaximum(UCalendarDateFields field) const
{
    return getLimit( field,UCAL_LIMIT_LEAST_MAXIMUM);
}


int32_t 
Calendar::getActualMinimum(EDateFields field, UErrorCode& status) const
{
    return getActualMinimum((UCalendarDateFields) field, status);
}

int32_t Calendar::getLimit(UCalendarDateFields field, ELimitType limitType) const {
    switch (field) {
    case UCAL_DAY_OF_WEEK:
    case UCAL_AM_PM:
    case UCAL_HOUR:
    case UCAL_HOUR_OF_DAY:
    case UCAL_MINUTE:
    case UCAL_SECOND:
    case UCAL_MILLISECOND:
    case UCAL_ZONE_OFFSET:
    case UCAL_DST_OFFSET:
    case UCAL_DOW_LOCAL:
    case UCAL_JULIAN_DAY:
    case UCAL_MILLISECONDS_IN_DAY:
    case UCAL_IS_LEAP_MONTH:
        return kCalendarLimits[field][limitType];

    case UCAL_WEEK_OF_MONTH:
        {
            int32_t limit;
            if (limitType == UCAL_LIMIT_MINIMUM) {
                limit = getMinimalDaysInFirstWeek() == 1 ? 1 : 0;
            } else if (limitType == UCAL_LIMIT_GREATEST_MINIMUM) {
                limit = 1;
            } else {
                int32_t minDaysInFirst = getMinimalDaysInFirstWeek();
                int32_t daysInMonth = handleGetLimit(UCAL_DAY_OF_MONTH, limitType);
                if (limitType == UCAL_LIMIT_LEAST_MAXIMUM) {
                    limit = (daysInMonth + (7 - minDaysInFirst)) / 7;
                } else { 
                    limit = (daysInMonth + 6 + (7 - minDaysInFirst)) / 7;
                }
            }
            return limit;
        }
    default:
        return handleGetLimit(field, limitType);
    }
}


int32_t
Calendar::getActualMinimum(UCalendarDateFields field, UErrorCode& status) const
{
    int32_t fieldValue = getGreatestMinimum(field);
    int32_t endValue = getMinimum(field);

    
    if (fieldValue == endValue) {
        return fieldValue;
    }

    
    
    Calendar *work = (Calendar*)this->clone();
    if (work == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    work->setLenient(TRUE);

    
    
    
    int32_t result = fieldValue;

    do {
        work->set(field, fieldValue);
        if (work->get(field, status) != fieldValue) {
            break;
        } 
        else {
            result = fieldValue;
            fieldValue--;
        }
    } while (fieldValue >= endValue);

    delete work;

    
    if(U_FAILURE(status)) {
        return 0;
    }
    return result;
}












void Calendar::validateFields(UErrorCode &status) {
    for (int32_t field = 0; U_SUCCESS(status) && (field < UCAL_FIELD_COUNT); field++) {
        if (fStamp[field] >= kMinimumUserStamp) {
            validateField((UCalendarDateFields)field, status);
        }
    }
}








void Calendar::validateField(UCalendarDateFields field, UErrorCode &status) {
    int32_t y;
    switch (field) {
    case UCAL_DAY_OF_MONTH:
        y = handleGetExtendedYear();
        validateField(field, 1, handleGetMonthLength(y, internalGet(UCAL_MONTH)), status);
        break;
    case UCAL_DAY_OF_YEAR:
        y = handleGetExtendedYear();
        validateField(field, 1, handleGetYearLength(y), status);
        break;
    case UCAL_DAY_OF_WEEK_IN_MONTH:
        if (internalGet(field) == 0) {
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d: ILLEGAL ARG because DOW in month cannot be 0\n", 
                __FILE__, __LINE__);
#endif
            status = U_ILLEGAL_ARGUMENT_ERROR; 
            return;
        }
        validateField(field, getMinimum(field), getMaximum(field), status);
        break;
    default:
        validateField(field, getMinimum(field), getMaximum(field), status);
        break;
    }
}








void Calendar::validateField(UCalendarDateFields field, int32_t min, int32_t max, UErrorCode& status)
{
    int32_t value = fFields[field];
    if (value < min || value > max) {
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d: ILLEGAL ARG because of field %s out of range %d..%d  at %d\n", 
            __FILE__, __LINE__,fldName(field),min,max,value);
#endif
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
}



const UFieldResolutionTable* Calendar::getFieldResolutionTable() const {
    return kDatePrecedence;
}


UCalendarDateFields Calendar::newerField(UCalendarDateFields defaultField, UCalendarDateFields alternateField) const
{
    if (fStamp[alternateField] > fStamp[defaultField]) {
        return alternateField;
    }
    return defaultField;
}

UCalendarDateFields Calendar::resolveFields(const UFieldResolutionTable* precedenceTable) {
    int32_t bestField = UCAL_FIELD_COUNT;
    int32_t tempBestField;
    for (int32_t g=0; precedenceTable[g][0][0] != -1 && (bestField == UCAL_FIELD_COUNT); ++g) {
        int32_t bestStamp = kUnset;
        for (int32_t l=0; precedenceTable[g][l][0] != -1; ++l) {
            int32_t lineStamp = kUnset;
            
            for (int32_t i=((precedenceTable[g][l][0]>=kResolveRemap)?1:0); precedenceTable[g][l][i]!=-1; ++i) {
                U_ASSERT(precedenceTable[g][l][i] < UCAL_FIELD_COUNT);
                int32_t s = fStamp[precedenceTable[g][l][i]];
                
                if (s == kUnset) {
                    goto linesInGroup;
                } else if(s > lineStamp) {
                    lineStamp = s;
                }
            }
            
            if (lineStamp > bestStamp) {
                tempBestField = precedenceTable[g][l][0]; 
                if (tempBestField >= kResolveRemap) {
                    tempBestField &= (kResolveRemap-1);
                    
                    if (tempBestField != UCAL_DATE || (fStamp[UCAL_WEEK_OF_MONTH] < fStamp[tempBestField])) {
                        bestField = tempBestField;
                    }
                } else {
                    bestField = tempBestField;
                }

                if (bestField == tempBestField) {
                    bestStamp = lineStamp;
                }
            }
linesInGroup:
            ;
        }
    }
    return (UCalendarDateFields)bestField;
}

const UFieldResolutionTable Calendar::kDatePrecedence[] =
{ 
    {
        { UCAL_DAY_OF_MONTH, kResolveSTOP },
        { UCAL_WEEK_OF_YEAR, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_WEEK_OF_YEAR, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_DAY_OF_YEAR, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_MONTH, UCAL_YEAR, kResolveSTOP },  
        { kResolveRemap | UCAL_WEEK_OF_YEAR, UCAL_YEAR_WOY, kResolveSTOP },  
        { kResolveSTOP }
    },
    {
        { UCAL_WEEK_OF_YEAR, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { kResolveSTOP }
    }, 
    {{kResolveSTOP}}
};


const UFieldResolutionTable Calendar::kDOWPrecedence[] = 
{
    {
        { UCAL_DAY_OF_WEEK,kResolveSTOP, kResolveSTOP },
        { UCAL_DOW_LOCAL,kResolveSTOP, kResolveSTOP },
        {kResolveSTOP}
    },
    {{kResolveSTOP}}
};


const UFieldResolutionTable Calendar::kYearPrecedence[] = 
{
    {
        { UCAL_YEAR, kResolveSTOP },
        { UCAL_EXTENDED_YEAR, kResolveSTOP },
        { UCAL_YEAR_WOY, UCAL_WEEK_OF_YEAR, kResolveSTOP },  
        { kResolveSTOP }
    },
    {{kResolveSTOP}}
};





void Calendar::computeTime(UErrorCode& status) {
    if (!isLenient()) {
        validateFields(status);
        if (U_FAILURE(status)) {
            return;
        }
    }

    
    int32_t julianDay = computeJulianDay();

    double millis = Grego::julianDayToMillis(julianDay);

#if defined (U_DEBUG_CAL)
    
    
    
    
    
    
#endif

    int32_t millisInDay;

    
    
    
    
    
    if (fStamp[UCAL_MILLISECONDS_IN_DAY] >= ((int32_t)kMinimumUserStamp) &&
            newestStamp(UCAL_AM_PM, UCAL_MILLISECOND, kUnset) <= fStamp[UCAL_MILLISECONDS_IN_DAY]) {
        millisInDay = internalGet(UCAL_MILLISECONDS_IN_DAY);
    } else {
        millisInDay = computeMillisInDay();
    }

    UDate t = 0;
    if (fStamp[UCAL_ZONE_OFFSET] >= ((int32_t)kMinimumUserStamp) || fStamp[UCAL_DST_OFFSET] >= ((int32_t)kMinimumUserStamp)) {
        t = millis + millisInDay - (internalGet(UCAL_ZONE_OFFSET) + internalGet(UCAL_DST_OFFSET));
    } else {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (!isLenient() || fSkippedWallTime == UCAL_WALLTIME_NEXT_VALID) {
            
            
            
            int32_t zoneOffset = computeZoneOffset(millis, millisInDay, status);
            UDate tmpTime = millis + millisInDay - zoneOffset;

            int32_t raw, dst;
            fZone->getOffset(tmpTime, FALSE, raw, dst, status);

            if (U_SUCCESS(status)) {
                
                
                if (zoneOffset != (raw + dst)) {
                    if (!isLenient()) {
                        status = U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        U_ASSERT(fSkippedWallTime == UCAL_WALLTIME_NEXT_VALID);
                        
                        
                        

                        BasicTimeZone *btz = getBasicTimeZone();
                        if (btz) {
                            TimeZoneTransition transition;
                            UBool hasTransition = btz->getPreviousTransition(tmpTime, TRUE, transition);
                            if (hasTransition) {
                                t = transition.getTime();
                            } else {
                                
                                
                                status = U_INTERNAL_PROGRAM_ERROR;
                            }
                        } else {
                            
                            
                            status = U_UNSUPPORTED_ERROR;
                        }
                    }
                } else {
                    t = tmpTime;
                }
            }
        } else {
            t = millis + millisInDay - computeZoneOffset(millis, millisInDay, status);
        }
    }
    if (U_SUCCESS(status)) {
        internalSetTime(t);
    }
}








int32_t Calendar::computeMillisInDay() {
  

    int32_t millisInDay = 0;

    
    
    
    int32_t hourOfDayStamp = fStamp[UCAL_HOUR_OF_DAY];
    int32_t hourStamp = (fStamp[UCAL_HOUR] > fStamp[UCAL_AM_PM])?fStamp[UCAL_HOUR]:fStamp[UCAL_AM_PM];
    int32_t bestStamp = (hourStamp > hourOfDayStamp) ? hourStamp : hourOfDayStamp;

    
    if (bestStamp != kUnset) {
        if (bestStamp == hourOfDayStamp) {
            
            
            millisInDay += internalGet(UCAL_HOUR_OF_DAY);
        } else {
            
            
            millisInDay += internalGet(UCAL_HOUR);
            millisInDay += 12 * internalGet(UCAL_AM_PM); 
        }
    }

    
    
    millisInDay *= 60;
    millisInDay += internalGet(UCAL_MINUTE); 
    millisInDay *= 60;
    millisInDay += internalGet(UCAL_SECOND); 
    millisInDay *= 1000;
    millisInDay += internalGet(UCAL_MILLISECOND); 

    return millisInDay;
}








int32_t Calendar::computeZoneOffset(double millis, int32_t millisInDay, UErrorCode &ec) {
    int32_t rawOffset, dstOffset;
    UDate wall = millis + millisInDay;
    BasicTimeZone* btz = getBasicTimeZone();
    if (btz) {
        int duplicatedTimeOpt = (fRepeatedWallTime == UCAL_WALLTIME_FIRST) ? BasicTimeZone::kFormer : BasicTimeZone::kLatter;
        int nonExistingTimeOpt = (fSkippedWallTime == UCAL_WALLTIME_FIRST) ? BasicTimeZone::kLatter : BasicTimeZone::kFormer;
        btz->getOffsetFromLocal(wall, nonExistingTimeOpt, duplicatedTimeOpt, rawOffset, dstOffset, ec);
    } else {
        const TimeZone& tz = getTimeZone();
        
        tz.getOffset(wall, TRUE, rawOffset, dstOffset, ec);

        UBool sawRecentNegativeShift = FALSE;
        if (fRepeatedWallTime == UCAL_WALLTIME_FIRST) {
            
            UDate tgmt = wall - (rawOffset + dstOffset);

            
            
            
            int32_t tmpRaw, tmpDst;
            tz.getOffset(tgmt - 6*60*60*1000, FALSE, tmpRaw, tmpDst, ec);
            int32_t offsetDelta = (rawOffset + dstOffset) - (tmpRaw + tmpDst);

            U_ASSERT(offsetDelta < -6*60*60*1000);
            if (offsetDelta < 0) {
                sawRecentNegativeShift = TRUE;
                
                
                
                tz.getOffset(wall + offsetDelta, TRUE, rawOffset, dstOffset, ec);
            }
        }
        if (!sawRecentNegativeShift && fSkippedWallTime == UCAL_WALLTIME_FIRST) {
            
            
            
            
            
            UDate tgmt = wall - (rawOffset + dstOffset);
            tz.getOffset(tgmt, FALSE, rawOffset, dstOffset, ec);
        }
    }
    return rawOffset + dstOffset;
}

int32_t Calendar::computeJulianDay() 
{
    
    
    
    
    
    
    
    
    if (fStamp[UCAL_JULIAN_DAY] >= (int32_t)kMinimumUserStamp) {
        int32_t bestStamp = newestStamp(UCAL_ERA, UCAL_DAY_OF_WEEK_IN_MONTH, kUnset);
        bestStamp = newestStamp(UCAL_YEAR_WOY, UCAL_EXTENDED_YEAR, bestStamp);
        if (bestStamp <= fStamp[UCAL_JULIAN_DAY]) {
            return internalGet(UCAL_JULIAN_DAY);
        }
    }

    UCalendarDateFields bestField = resolveFields(getFieldResolutionTable());
    if (bestField == UCAL_FIELD_COUNT) {
        bestField = UCAL_DAY_OF_MONTH;
    }

    return handleComputeJulianDay(bestField);
}



int32_t Calendar::handleComputeJulianDay(UCalendarDateFields bestField)  {
    UBool useMonth = (bestField == UCAL_DAY_OF_MONTH ||
        bestField == UCAL_WEEK_OF_MONTH ||
        bestField == UCAL_DAY_OF_WEEK_IN_MONTH);
    int32_t year;

    if (bestField == UCAL_WEEK_OF_YEAR) {
        year = internalGet(UCAL_YEAR_WOY, handleGetExtendedYear());
        internalSet(UCAL_EXTENDED_YEAR, year);
    } else {
        year = handleGetExtendedYear();
        internalSet(UCAL_EXTENDED_YEAR, year);
    }

#if defined (U_DEBUG_CAL) 
    fprintf(stderr, "%s:%d: bestField= %s - y=%d\n", __FILE__, __LINE__, fldName(bestField), year);
#endif 

    
    

    
    int32_t month;

    if(isSet(UCAL_MONTH)) {
        month = internalGet(UCAL_MONTH);
    } else {
        month = getDefaultMonthInYear(year);
    }

    int32_t julianDay = handleComputeMonthStart(year, useMonth ? month : 0, useMonth);

    if (bestField == UCAL_DAY_OF_MONTH) {

        
        int32_t dayOfMonth;
        if(isSet(UCAL_DAY_OF_MONTH)) {
            dayOfMonth = internalGet(UCAL_DAY_OF_MONTH,1);
        } else {
            dayOfMonth = getDefaultDayInMonth(year, month);
        }
        return julianDay + dayOfMonth;
    }

    if (bestField == UCAL_DAY_OF_YEAR) {
        return julianDay + internalGet(UCAL_DAY_OF_YEAR);
    }

    int32_t firstDayOfWeek = getFirstDayOfWeek(); 

    
    
    
    

    
    
    
    

    
    
    int32_t first = julianDayToDayOfWeek(julianDay + 1) - firstDayOfWeek;
    if (first < 0) {
        first += 7;
    }

    int32_t dowLocal = getLocalDOW();

    
    
    
    int32_t date = 1 - first + dowLocal;

    if (bestField == UCAL_DAY_OF_WEEK_IN_MONTH) {
        
        if (date < 1) {
            date += 7;
        }

        
        
        int32_t dim = internalGet(UCAL_DAY_OF_WEEK_IN_MONTH, 1);
        if (dim >= 0) {
            date += 7*(dim - 1);

        } else {
            
            
            
            
            
            
            int32_t m = internalGet(UCAL_MONTH, UCAL_JANUARY);
            int32_t monthLength = handleGetMonthLength(year, m);
            date += ((monthLength - date) / 7 + dim + 1) * 7;
        }
    } else {
#if defined (U_DEBUG_CAL) 
        fprintf(stderr, "%s:%d - bf= %s\n", __FILE__, __LINE__, fldName(bestField));
#endif 

        if(bestField == UCAL_WEEK_OF_YEAR) {  
            if(!isSet(UCAL_YEAR_WOY) ||  
                ( (resolveFields(kYearPrecedence) != UCAL_YEAR_WOY) 
                && (fStamp[UCAL_YEAR_WOY]!=kInternallySet) ) ) 
            {
                
                int32_t woy = internalGet(bestField);

                int32_t nextJulianDay = handleComputeMonthStart(year+1, 0, FALSE); 
                int32_t nextFirst = julianDayToDayOfWeek(nextJulianDay + 1) - firstDayOfWeek; 

                if (nextFirst < 0) { 
                    nextFirst += 7;
                }

                if(woy==1) {  
#if defined (U_DEBUG_CAL) 
                    fprintf(stderr, "%s:%d - woy=%d, yp=%d, nj(%d)=%d, nf=%d", __FILE__, __LINE__, 
                        internalGet(bestField), resolveFields(kYearPrecedence), year+1, 
                        nextJulianDay, nextFirst);

                    fprintf(stderr, " next: %d DFW,  min=%d   \n", (7-nextFirst), getMinimalDaysInFirstWeek() );
#endif 

                    
                    if((nextFirst > 0) &&   
                        (7-nextFirst) >= getMinimalDaysInFirstWeek()) 
                    {
                        
#if defined (U_DEBUG_CAL) 
                        fprintf(stderr, "%s:%d - was going to move JD from %d to %d [d%d]\n", __FILE__, __LINE__, 
                            julianDay, nextJulianDay, (nextJulianDay-julianDay));
#endif 
                        julianDay = nextJulianDay;

                        
                        first = julianDayToDayOfWeek(julianDay + 1) - firstDayOfWeek;
                        if (first < 0) {
                            first += 7;
                        }
                        
                        date = 1 - first + dowLocal;
                    }
                } else if(woy>=getLeastMaximum(bestField)) {          
                    
                    int32_t testDate = date;
                    if ((7 - first) < getMinimalDaysInFirstWeek()) {
                        testDate += 7;
                    }

                    
                    testDate += 7 * (woy - 1);

#if defined (U_DEBUG_CAL) 
                    fprintf(stderr, "%s:%d - y=%d, y-1=%d doy%d, njd%d (C.F. %d)\n",
                        __FILE__, __LINE__, year, year-1, testDate, julianDay+testDate, nextJulianDay);
#endif
                    if(julianDay+testDate > nextJulianDay) { 
                        
                        julianDay = handleComputeMonthStart(year-1, 0, FALSE); 
                        first = julianDayToDayOfWeek(julianDay + 1) - firstDayOfWeek; 

                        if(first < 0) { 
                            first += 7;
                        }
                        date = 1 - first + dowLocal;

#if defined (U_DEBUG_CAL) 
                        fprintf(stderr, "%s:%d - date now %d, jd%d, ywoy%d\n",
                            __FILE__, __LINE__, date, julianDay, year-1);
#endif


                    } 
                } 
            } 
        } 

        
        
        if ((7 - first) < getMinimalDaysInFirstWeek()) {
            date += 7;
        }

        
        date += 7 * (internalGet(bestField) - 1);
    }

    return julianDay + date;
}

int32_t
Calendar::getDefaultMonthInYear(int32_t ) 
{
    return 0;
}

int32_t
Calendar::getDefaultDayInMonth(int32_t , int32_t ) 
{
    return 1;
}


int32_t Calendar::getLocalDOW()
{
  
    
    int32_t dowLocal = 0;
    switch (resolveFields(kDOWPrecedence)) {
    case UCAL_DAY_OF_WEEK:
        dowLocal = internalGet(UCAL_DAY_OF_WEEK) - fFirstDayOfWeek;
        break;
    case UCAL_DOW_LOCAL:
        dowLocal = internalGet(UCAL_DOW_LOCAL) - 1;
        break;
    default:
        break;
    }
    dowLocal = dowLocal % 7;
    if (dowLocal < 0) {
        dowLocal += 7;
    }
    return dowLocal;
}

int32_t Calendar::handleGetExtendedYearFromWeekFields(int32_t yearWoy, int32_t woy)
{
    
    
    
    

    
    UCalendarDateFields bestField = resolveFields(kDatePrecedence); 

    
    int32_t dowLocal = getLocalDOW(); 
    int32_t firstDayOfWeek = getFirstDayOfWeek(); 
    int32_t jan1Start = handleComputeMonthStart(yearWoy, 0, FALSE);
    int32_t nextJan1Start = handleComputeMonthStart(yearWoy+1, 0, FALSE); 

    
    
    
    

    
    
    
    

    
    
    int32_t first = julianDayToDayOfWeek(jan1Start + 1) - firstDayOfWeek;
    if (first < 0) {
        first += 7;
    }
    int32_t nextFirst = julianDayToDayOfWeek(nextJan1Start + 1) - firstDayOfWeek;
    if (nextFirst < 0) {
        nextFirst += 7;
    }

    int32_t minDays = getMinimalDaysInFirstWeek();
    UBool jan1InPrevYear = FALSE;  
    

    if((7 - first) < minDays) { 
        jan1InPrevYear = TRUE;
    }

    
    
    

    switch(bestField) {
    case UCAL_WEEK_OF_YEAR:
        if(woy == 1) {
            if(jan1InPrevYear == TRUE) {
                
                
                return yearWoy;
            } else {
                
                if( dowLocal < first) { 
                    return yearWoy-1; 
                } else {
                    return yearWoy; 
                }
            }
        } else if(woy >= getLeastMaximum(bestField)) {  
            
            int32_t jd =  
                jan1Start +  
                (7-first) + 
                (woy-1)*7 + 
                dowLocal;   
            if(jan1InPrevYear==FALSE) {
                jd -= 7; 
            }

            if( (jd+1) >= nextJan1Start ) {
                
                return yearWoy+1;
            } else {
                
                return yearWoy;
            }
        } else {
            
            return yearWoy;
        }

    case UCAL_DATE:
        if((internalGet(UCAL_MONTH)==0) &&
            (woy >= getLeastMaximum(UCAL_WEEK_OF_YEAR))) {
                return yearWoy+1; 
            } else if(woy==1) {
                
                if(internalGet(UCAL_MONTH)==0) {
                    return yearWoy;
                } else {
                    return yearWoy-1;
                }
                
            }

            
            
            
            return yearWoy;

    default: 
        return yearWoy;
    }
}

int32_t Calendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const
{
    return handleComputeMonthStart(extendedYear, month+1, TRUE) -
        handleComputeMonthStart(extendedYear, month, TRUE);
}

int32_t Calendar::handleGetYearLength(int32_t eyear) const  {
    return handleComputeMonthStart(eyear+1, 0, FALSE) -
        handleComputeMonthStart(eyear, 0, FALSE);
}

int32_t
Calendar::getActualMaximum(UCalendarDateFields field, UErrorCode& status) const
{
    int32_t result;
    switch (field) {
    case UCAL_DATE:
        {
            if(U_FAILURE(status)) return 0;
            Calendar *cal = clone();
            if(!cal) { status = U_MEMORY_ALLOCATION_ERROR; return 0; }
            cal->setLenient(TRUE);
            cal->prepareGetActual(field,FALSE,status);
            result = handleGetMonthLength(cal->get(UCAL_EXTENDED_YEAR, status), cal->get(UCAL_MONTH, status));
            delete cal;
        }
        break;

    case UCAL_DAY_OF_YEAR:
        {
            if(U_FAILURE(status)) return 0;
            Calendar *cal = clone();
            if(!cal) { status = U_MEMORY_ALLOCATION_ERROR; return 0; }
            cal->setLenient(TRUE);
            cal->prepareGetActual(field,FALSE,status);
            result = handleGetYearLength(cal->get(UCAL_EXTENDED_YEAR, status));
            delete cal;
        }
        break;

    case UCAL_DAY_OF_WEEK:
    case UCAL_AM_PM:
    case UCAL_HOUR:
    case UCAL_HOUR_OF_DAY:
    case UCAL_MINUTE:
    case UCAL_SECOND:
    case UCAL_MILLISECOND:
    case UCAL_ZONE_OFFSET:
    case UCAL_DST_OFFSET:
    case UCAL_DOW_LOCAL:
    case UCAL_JULIAN_DAY:
    case UCAL_MILLISECONDS_IN_DAY:
        
        result = getMaximum(field);
        break;

    default:
        
        result = getActualHelper(field, getLeastMaximum(field), getMaximum(field),status);
        break;
    }
    return result;
}























void Calendar::prepareGetActual(UCalendarDateFields field, UBool isMinimum, UErrorCode &status)
{
    set(UCAL_MILLISECONDS_IN_DAY, 0);

    switch (field) {
    case UCAL_YEAR:
    case UCAL_EXTENDED_YEAR:
        set(UCAL_DAY_OF_YEAR, getGreatestMinimum(UCAL_DAY_OF_YEAR));
        break;

    case UCAL_YEAR_WOY:
        set(UCAL_WEEK_OF_YEAR, getGreatestMinimum(UCAL_WEEK_OF_YEAR));

    case UCAL_MONTH:
        set(UCAL_DATE, getGreatestMinimum(UCAL_DATE));
        break;

    case UCAL_DAY_OF_WEEK_IN_MONTH:
        
        
        set(UCAL_DATE, 1);
        set(UCAL_DAY_OF_WEEK, get(UCAL_DAY_OF_WEEK, status)); 
        break;

    case UCAL_WEEK_OF_MONTH:
    case UCAL_WEEK_OF_YEAR:
        
        
        
        
        {
            int32_t dow = fFirstDayOfWeek;
            if (isMinimum) {
                dow = (dow + 6) % 7; 
                if (dow < UCAL_SUNDAY) {
                    dow += 7;
                }
            }
#if defined (U_DEBUG_CAL) 
            fprintf(stderr, "prepareGetActualHelper(WOM/WOY) - dow=%d\n", dow);
#endif
            set(UCAL_DAY_OF_WEEK, dow);
        }
        break;
    default:
        break;
    }

    
    set(field, getGreatestMinimum(field));
}

int32_t Calendar::getActualHelper(UCalendarDateFields field, int32_t startValue, int32_t endValue, UErrorCode &status) const
{
#if defined (U_DEBUG_CAL) 
    fprintf(stderr, "getActualHelper(%d,%d .. %d, %s)\n", field, startValue, endValue, u_errorName(status));
#endif
    if (startValue == endValue) {
        
        return startValue;
    }

    int32_t delta = (endValue > startValue) ? 1 : -1;

    
    
    if(U_FAILURE(status)) return startValue;
    Calendar *work = clone();
    if(!work) { status = U_MEMORY_ALLOCATION_ERROR; return startValue; }

    
    
    work->complete(status);

    work->setLenient(TRUE);
    work->prepareGetActual(field, delta < 0, status);

    
    
    
    work->set(field, startValue);

    
    
    
    
    
    int32_t result = startValue;
    if ((work->get(field, status) != startValue
         && field != UCAL_WEEK_OF_MONTH && delta > 0 ) || U_FAILURE(status)) {
#if defined (U_DEBUG_CAL) 
        fprintf(stderr, "getActualHelper(fld %d) - got  %d (not %d) - %s\n", field, work->get(field,status), startValue, u_errorName(status));
#endif
    } else {
        do {
            startValue += delta;
            work->add(field, delta, status);
            if (work->get(field, status) != startValue || U_FAILURE(status)) {
#if defined (U_DEBUG_CAL)
                fprintf(stderr, "getActualHelper(fld %d) - got  %d (not %d), BREAK - %s\n", field, work->get(field,status), startValue, u_errorName(status));
#endif
                break;
            }
            result = startValue;
        } while (startValue != endValue);
    }
    delete work;
#if defined (U_DEBUG_CAL) 
    fprintf(stderr, "getActualHelper(%d) = %d\n", field, result);
#endif
    return result;
}






void
Calendar::setWeekData(const Locale& desiredLocale, const char *type, UErrorCode& status)
{

    if (U_FAILURE(status)) return;

    fFirstDayOfWeek = UCAL_SUNDAY;
    fMinimalDaysInFirstWeek = 1;
    fWeekendOnset = UCAL_SATURDAY;
    fWeekendOnsetMillis = 0;
    fWeekendCease = UCAL_SUNDAY;
    fWeekendCeaseMillis = 86400000; 

    
    
    
    
    
    
    
 
    char minLocaleID[ULOC_FULLNAME_CAPACITY] = { 0 };
    UErrorCode myStatus = U_ZERO_ERROR;

    uloc_minimizeSubtags(desiredLocale.getName(),minLocaleID,ULOC_FULLNAME_CAPACITY,&myStatus);
    Locale min = Locale::createFromName(minLocaleID);
    Locale useLocale;
    if ( uprv_strlen(desiredLocale.getCountry()) == 0 || 
         (uprv_strlen(desiredLocale.getScript()) > 0 && uprv_strlen(min.getScript()) == 0) ) {
        char maxLocaleID[ULOC_FULLNAME_CAPACITY] = { 0 };
        myStatus = U_ZERO_ERROR;
        uloc_addLikelySubtags(desiredLocale.getName(),maxLocaleID,ULOC_FULLNAME_CAPACITY,&myStatus);
        Locale max = Locale::createFromName(maxLocaleID);
        useLocale = Locale(max.getLanguage(),max.getCountry());
    } else {
        useLocale = Locale(desiredLocale);
    }
 
    





    CalendarData calData(useLocale,type,status);
    UResourceBundle *monthNames = calData.getByKey(gMonthNames,status);
    if (U_SUCCESS(status)) {
        U_LOCALE_BASED(locBased,*this);
        locBased.setLocaleIDs(ures_getLocaleByType(monthNames, ULOC_VALID_LOCALE, &status),
                              ures_getLocaleByType(monthNames, ULOC_ACTUAL_LOCALE, &status));
    } else {
        status = U_USING_FALLBACK_WARNING;
        return;
    }

    
    
    UResourceBundle *rb = ures_openDirect(NULL, "supplementalData", &status);
    ures_getByKey(rb, "weekData", rb, &status);
    UResourceBundle *weekData = ures_getByKey(rb, useLocale.getCountry(), NULL, &status);
    if (status == U_MISSING_RESOURCE_ERROR && rb != NULL) {
        status = U_ZERO_ERROR;
        weekData = ures_getByKey(rb, "001", NULL, &status);
    }

    if (U_FAILURE(status)) {
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, " Failure loading weekData from supplemental = %s\n", u_errorName(status));
#endif
        status = U_USING_FALLBACK_WARNING;
    } else {
        int32_t arrLen;
        const int32_t *weekDataArr = ures_getIntVector(weekData,&arrLen,&status);
        if( U_SUCCESS(status) && arrLen == 6
                && 1 <= weekDataArr[0] && weekDataArr[0] <= 7
                && 1 <= weekDataArr[1] && weekDataArr[1] <= 7
                && 1 <= weekDataArr[2] && weekDataArr[2] <= 7
                && 1 <= weekDataArr[4] && weekDataArr[4] <= 7) {
            fFirstDayOfWeek = (UCalendarDaysOfWeek)weekDataArr[0];
            fMinimalDaysInFirstWeek = (uint8_t)weekDataArr[1];
            fWeekendOnset = (UCalendarDaysOfWeek)weekDataArr[2];
            fWeekendOnsetMillis = weekDataArr[3];
            fWeekendCease = (UCalendarDaysOfWeek)weekDataArr[4];
            fWeekendCeaseMillis = weekDataArr[5];
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    ures_close(weekData);
    ures_close(rb);
}






void 
Calendar::updateTime(UErrorCode& status) 
{
    computeTime(status);
    if(U_FAILURE(status))
        return;

    
    
    
    if (isLenient() || ! fAreAllFieldsSet) 
        fAreFieldsSet = FALSE;

    fIsTimeSet = TRUE;
    fAreFieldsVirtuallySet = FALSE;
}

Locale 
Calendar::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

const char *
Calendar::getLocaleID(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocaleID(type, status);
}

void
Calendar::recalculateStamp() {
    int32_t index;
    int32_t currentValue;
    int32_t j, i;

    fNextStamp = 1;

    for (j = 0; j < UCAL_FIELD_COUNT; j++) {
        currentValue = STAMP_MAX;
        index = -1;
        for (i = 0; i < UCAL_FIELD_COUNT; i++) {
            if (fStamp[i] > fNextStamp && fStamp[i] < currentValue) {
                currentValue = fStamp[i];
                index = i;
            }
        }

        if (index >= 0) {
            fStamp[index] = ++fNextStamp;
        } else {
            break;
        }
    }
    fNextStamp++;
}


void
Calendar::internalSet(EDateFields field, int32_t value)
{
    internalSet((UCalendarDateFields) field, value);
}

BasicTimeZone*
Calendar::getBasicTimeZone(void) const {
    if (dynamic_cast<const OlsonTimeZone *>(fZone) != NULL
        || dynamic_cast<const SimpleTimeZone *>(fZone) != NULL
        || dynamic_cast<const RuleBasedTimeZone *>(fZone) != NULL
        || dynamic_cast<const VTimeZone *>(fZone) != NULL) {
        return (BasicTimeZone*)fZone;
    }
    return NULL;
}

U_NAMESPACE_END

#endif 



