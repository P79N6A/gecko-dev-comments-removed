








































#include "uposixdefs.h"


#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "putilimp.h"
#include "uassert.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "locmap.h"
#include "ucln_cmn.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <float.h>

#ifndef U_COMMON_IMPLEMENTATION
#error U_COMMON_IMPLEMENTATION not set - must be set for all ICU source files in common/ - see http://userguide.icu-project.org/howtouseicu
#endif



#if U_PLATFORM_USES_ONLY_WIN32_API
    




#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#   include "wintz.h"
#elif U_PLATFORM == U_PF_OS400
#   include <float.h>
#   include <qusec.h>       
#   include <qusrjobi.h>
#   include <qliept.h>      
#   include <mih/testptr.h> 
#elif U_PLATFORM == U_PF_CLASSIC_MACOS
#   include <Files.h>
#   include <IntlResources.h>
#   include <Script.h>
#   include <Folders.h>
#   include <MacTypes.h>
#   include <TextUtils.h>
#   define ICU_NO_USER_DATA_OVERRIDE 1
#elif U_PLATFORM == U_PF_OS390
#   include "unicode/ucnv.h"   
#elif U_PLATFORM_IS_DARWIN_BASED || U_PLATFORM_IS_LINUX_BASED || U_PLATFORM == U_PF_BSD
#   include <limits.h>
#   include <unistd.h>
#elif U_PLATFORM == U_PF_QNX
#   include <sys/neutrino.h>
#elif U_PLATFORM == U_PF_SOLARIS
#   ifndef _XPG4_2
#       define _XPG4_2
#   endif
#endif

#if (U_PF_MINGW <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN) && defined(__STRICT_ANSI__)

#undef __STRICT_ANSI__
#endif




#include <time.h>

#if !U_PLATFORM_USES_ONLY_WIN32_API
#include <sys/time.h>
#endif







#if U_HAVE_NL_LANGINFO_CODESET
#include <langinfo.h>
#endif





#if U_PLATFORM_IMPLEMENTS_POSIX
#   if U_PLATFORM == U_PF_OS400
#    define HAVE_DLFCN_H 0
#    define HAVE_DLOPEN 0
#   else
#   ifndef HAVE_DLFCN_H
#    define HAVE_DLFCN_H 1
#   endif
#   ifndef HAVE_DLOPEN
#    define HAVE_DLOPEN 1
#   endif
#   endif
#   ifndef HAVE_GETTIMEOFDAY
#    define HAVE_GETTIMEOFDAY 1
#   endif
#else
#   define HAVE_DLFCN_H 0
#   define HAVE_DLOPEN 0
#   define HAVE_GETTIMEOFDAY 0
#endif

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))


#define DATA_TYPE "dat"


static const char copyright[] = U_COPYRIGHT_STRING;




#define SIGN 0x80000000U


typedef union {
    int64_t i64; 
    double d64;
} BitPatternConversion;
static const BitPatternConversion gNan = { (int64_t) INT64_C(0x7FF8000000000000) };
static const BitPatternConversion gInf = { (int64_t) INT64_C(0x7FF0000000000000) };









#if U_PLATFORM_USES_ONLY_WIN32_API || U_PLATFORM == U_PF_CLASSIC_MACOS || U_PLATFORM == U_PF_OS400
#   undef U_POSIX_LOCALE
#else
#   define U_POSIX_LOCALE    1
#endif





#if !IEEE_754
static char*
u_topNBytesOfDouble(double* d, int n)
{
#if U_IS_BIG_ENDIAN
    return (char*)d;
#else
    return (char*)(d + 1) - n;
#endif
}

static char*
u_bottomNBytesOfDouble(double* d, int n)
{
#if U_IS_BIG_ENDIAN
    return (char*)(d + 1) - n;
#else
    return (char*)d;
#endif
}
#endif   

#if IEEE_754
static UBool
u_signBit(double d) {
    uint8_t hiByte;
#if U_IS_BIG_ENDIAN
    hiByte = *(uint8_t *)&d;
#else
    hiByte = *(((uint8_t *)&d) + sizeof(double) - 1);
#endif
    return (hiByte & 0x80) != 0;
}
#endif



#if defined (U_DEBUG_FAKETIME)



UDate fakeClock_t0 = 0; 
UDate fakeClock_dt = 0; 
UBool fakeClock_set = FALSE; 
static UMutex fakeClockMutex = U_MUTEX_INTIALIZER;

static UDate getUTCtime_real() {
    struct timeval posixTime;
    gettimeofday(&posixTime, NULL);
    return (UDate)(((int64_t)posixTime.tv_sec * U_MILLIS_PER_SECOND) + (posixTime.tv_usec/1000));
}

static UDate getUTCtime_fake() {
    umtx_lock(&fakeClockMutex);
    if(!fakeClock_set) {
        UDate real = getUTCtime_real();
        const char *fake_start = getenv("U_FAKETIME_START");
        if((fake_start!=NULL) && (fake_start[0]!=0)) {
            sscanf(fake_start,"%lf",&fakeClock_t0);
            fakeClock_dt = fakeClock_t0 - real;
            fprintf(stderr,"U_DEBUG_FAKETIME was set at compile time, so the ICU clock will start at a preset value\n"
                    "env variable U_FAKETIME_START=%.0f (%s) for an offset of %.0f ms from the current time %.0f\n",
                    fakeClock_t0, fake_start, fakeClock_dt, real);
        } else {
          fakeClock_dt = 0;
            fprintf(stderr,"U_DEBUG_FAKETIME was set at compile time, but U_FAKETIME_START was not set.\n"
                    "Set U_FAKETIME_START to the number of milliseconds since 1/1/1970 to set the ICU clock.\n");
        }
        fakeClock_set = TRUE;
    }
    umtx_unlock(&fakeClockMutex);

    return getUTCtime_real() + fakeClock_dt;
}
#endif

#if U_PLATFORM_USES_ONLY_WIN32_API
typedef union {
    int64_t int64;
    FILETIME fileTime;
} FileTimeConversion;   


#define EPOCH_BIAS  INT64_C(116444736000000000)
#define HECTONANOSECOND_PER_MILLISECOND   10000

#endif








U_CAPI UDate U_EXPORT2
uprv_getUTCtime()
{
#if defined(U_DEBUG_FAKETIME)
    return getUTCtime_fake(); 
#else
    return uprv_getRawUTCtime();
#endif
}


U_CAPI UDate U_EXPORT2
uprv_getRawUTCtime()
{
#if U_PLATFORM == U_PF_CLASSIC_MACOS
    time_t t, t1, t2;
    struct tm tmrec;

    uprv_memset( &tmrec, 0, sizeof(tmrec) );
    tmrec.tm_year = 70;
    tmrec.tm_mon = 0;
    tmrec.tm_mday = 1;
    t1 = mktime(&tmrec);    

    time(&t);
    uprv_memcpy( &tmrec, gmtime(&t), sizeof(tmrec) );
    t2 = mktime(&tmrec);    
    return (UDate)(t2 - t1) * U_MILLIS_PER_SECOND;         
#elif U_PLATFORM_USES_ONLY_WIN32_API

    FileTimeConversion winTime;
    GetSystemTimeAsFileTime(&winTime.fileTime);
    return (UDate)((winTime.int64 - EPOCH_BIAS) / HECTONANOSECOND_PER_MILLISECOND);
#else

#if HAVE_GETTIMEOFDAY
    struct timeval posixTime;
    gettimeofday(&posixTime, NULL);
    return (UDate)(((int64_t)posixTime.tv_sec * U_MILLIS_PER_SECOND) + (posixTime.tv_usec/1000));
#else
    time_t epochtime;
    time(&epochtime);
    return (UDate)epochtime * U_MILLIS_PER_SECOND;
#endif

#endif
}











U_CAPI UBool U_EXPORT2
uprv_isNaN(double number)
{
#if IEEE_754
    BitPatternConversion convertedNumber;
    convertedNumber.d64 = number;
    
    return (UBool)((convertedNumber.i64 & U_INT64_MAX) > gInf.i64);

#elif U_PLATFORM == U_PF_OS390
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                        sizeof(uint32_t));

    return ((highBits & 0x7F080000L) == 0x7F080000L) &&
      (lowBits == 0x00000000L);

#else
    
    
    
    return number != number;
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isInfinite(double number)
{
#if IEEE_754
    BitPatternConversion convertedNumber;
    convertedNumber.d64 = number;
    
    return (UBool)((convertedNumber.i64 & U_INT64_MAX) == gInf.i64);
#elif U_PLATFORM == U_PF_OS390
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                        sizeof(uint32_t));

    return ((highBits  & ~SIGN) == 0x70FF0000L) && (lowBits == 0x00000000L);

#else
    
    
    
    return number == (2.0 * number);
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isPositiveInfinity(double number)
{
#if IEEE_754 || U_PLATFORM == U_PF_OS390
    return (UBool)(number > 0 && uprv_isInfinite(number));
#else
    return uprv_isInfinite(number);
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isNegativeInfinity(double number)
{
#if IEEE_754 || U_PLATFORM == U_PF_OS390
    return (UBool)(number < 0 && uprv_isInfinite(number));

#else
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    return((highBits & SIGN) && uprv_isInfinite(number));

#endif
}

U_CAPI double U_EXPORT2
uprv_getNaN()
{
#if IEEE_754 || U_PLATFORM == U_PF_OS390
    return gNan.d64;
#else
    
    
    
    return 0.0;
#endif
}

U_CAPI double U_EXPORT2
uprv_getInfinity()
{
#if IEEE_754 || U_PLATFORM == U_PF_OS390
    return gInf.d64;
#else
    
    
    
    return 0.0;
#endif
}

U_CAPI double U_EXPORT2
uprv_floor(double x)
{
    return floor(x);
}

U_CAPI double U_EXPORT2
uprv_ceil(double x)
{
    return ceil(x);
}

U_CAPI double U_EXPORT2
uprv_round(double x)
{
    return uprv_floor(x + 0.5);
}

U_CAPI double U_EXPORT2
uprv_fabs(double x)
{
    return fabs(x);
}

U_CAPI double U_EXPORT2
uprv_modf(double x, double* y)
{
    return modf(x, y);
}

U_CAPI double U_EXPORT2
uprv_fmod(double x, double y)
{
    return fmod(x, y);
}

U_CAPI double U_EXPORT2
uprv_pow(double x, double y)
{
    
    return pow(x, y);
}

U_CAPI double U_EXPORT2
uprv_pow10(int32_t x)
{
    return pow(10.0, (double)x);
}

U_CAPI double U_EXPORT2
uprv_fmax(double x, double y)
{
#if IEEE_754
    
    if(uprv_isNaN(x) || uprv_isNaN(y))
        return uprv_getNaN();

    
    if(x == 0.0 && y == 0.0 && u_signBit(x))
        return y;

#endif

    
    return (x > y ? x : y);
}

U_CAPI double U_EXPORT2
uprv_fmin(double x, double y)
{
#if IEEE_754
    
    if(uprv_isNaN(x) || uprv_isNaN(y))
        return uprv_getNaN();

    
    if(x == 0.0 && y == 0.0 && u_signBit(y))
        return y;

#endif

    
    return (x > y ? y : x);
}








U_CAPI double U_EXPORT2
uprv_trunc(double d)
{
#if IEEE_754
    
    if(uprv_isNaN(d))
        return uprv_getNaN();
    if(uprv_isInfinite(d))
        return uprv_getInfinity();

    if(u_signBit(d))    
        return ceil(d);
    else
        return floor(d);

#else
    return d >= 0 ? floor(d) : ceil(d);

#endif
}





U_CAPI double U_EXPORT2
uprv_maxMantissa(void)
{
    return pow(2.0, DBL_MANT_DIG + 1.0) - 1.0;
}

U_CAPI double U_EXPORT2
uprv_log(double d)
{
    return log(d);
}

U_CAPI void * U_EXPORT2
uprv_maximumPtr(void * base)
{
#if U_PLATFORM == U_PF_OS400
    
















    if ((base != NULL) && (_TESTPTR(base, _C_TERASPACE_CHECK))) {
        
        return ((void *)(((char *)base)-((uint32_t)(base))+((uint32_t)0x7fffefff)));
    }
    
    return ((void *)(((char *)base)-((uint32_t)(base))+((uint32_t)0xffefff)));

#else
    return U_MAX_PTR(base);
#endif
}










U_CAPI void U_EXPORT2
uprv_tzset()
{
#if defined(U_TZSET)
    U_TZSET();
#else
    
#endif
}

U_CAPI int32_t U_EXPORT2
uprv_timezone()
{
#ifdef U_TIMEZONE
    return U_TIMEZONE;
#else
    time_t t, t1, t2;
    struct tm tmrec;
    UBool dst_checked;
    int32_t tdiff = 0;

    time(&t);
    uprv_memcpy( &tmrec, localtime(&t), sizeof(tmrec) );
    dst_checked = (tmrec.tm_isdst != 0); 
    t1 = mktime(&tmrec);                 
    uprv_memcpy( &tmrec, gmtime(&t), sizeof(tmrec) );
    t2 = mktime(&tmrec);                 
    tdiff = t2 - t1;
    








    if (dst_checked)
        tdiff += 3600;
    return tdiff;
#endif
}




#if defined(U_TZNAME) && (U_PLATFORM == U_PF_IRIX || U_PLATFORM_IS_DARWIN_BASED || (U_PLATFORM == U_PF_CYGWIN && !U_PLATFORM_USES_ONLY_WIN32_API))

extern U_IMPORT char *U_TZNAME[];
#endif

#if !UCONFIG_NO_FILE_IO && (U_PLATFORM_IS_DARWIN_BASED || U_PLATFORM_IS_LINUX_BASED || U_PLATFORM == U_PF_BSD)

#define CHECK_LOCALTIME_LINK 1
#if U_PLATFORM_IS_DARWIN_BASED
#include <tzfile.h>
#define TZZONEINFO      (TZDIR "/")
#else
#define TZDEFAULT       "/etc/localtime"
#define TZZONEINFO      "/usr/share/zoneinfo/"
#endif
#if U_HAVE_DIRENT_H
#define TZFILE_SKIP     "posixrules" /* tz file to skip when searching. */



#define TZFILE_SKIP2    "localtime"
#define SEARCH_TZFILE
#include <dirent.h>  
#endif
static char gTimeZoneBuffer[PATH_MAX];
static char *gTimeZoneBufferPtr = NULL;
#endif

#if !U_PLATFORM_USES_ONLY_WIN32_API
#define isNonDigit(ch) (ch < '0' || '9' < ch)
static UBool isValidOlsonID(const char *id) {
    int32_t idx = 0;

    

    while (id[idx] && isNonDigit(id[idx]) && id[idx] != ',') {
        idx++;
    }

    



    return (UBool)(id[idx] == 0
        || uprv_strcmp(id, "PST8PDT") == 0
        || uprv_strcmp(id, "MST7MDT") == 0
        || uprv_strcmp(id, "CST6CDT") == 0
        || uprv_strcmp(id, "EST5EDT") == 0);
}













static void skipZoneIDPrefix(const char** id) {
    if (uprv_strncmp(*id, "posix/", 6) == 0
        || uprv_strncmp(*id, "right/", 6) == 0)
    {
        *id += 6;
    }
}
#endif

#if defined(U_TZNAME) && !U_PLATFORM_USES_ONLY_WIN32_API

#define CONVERT_HOURS_TO_SECONDS(offset) (int32_t)(offset*3600)
typedef struct OffsetZoneMapping {
    int32_t offsetSeconds;
    int32_t daylightType; 
    const char *stdID;
    const char *dstID;
    const char *olsonID;
} OffsetZoneMapping;

enum { U_DAYLIGHT_NONE=0,U_DAYLIGHT_JUNE=1,U_DAYLIGHT_DECEMBER=2 };










static const struct OffsetZoneMapping OFFSET_ZONE_MAPPINGS[] = {
    {-45900, 2, "CHAST", "CHADT", "Pacific/Chatham"},
    {-43200, 1, "PETT", "PETST", "Asia/Kamchatka"},
    {-43200, 2, "NZST", "NZDT", "Pacific/Auckland"},
    {-43200, 1, "ANAT", "ANAST", "Asia/Anadyr"},
    {-39600, 1, "MAGT", "MAGST", "Asia/Magadan"},
    {-37800, 2, "LHST", "LHST", "Australia/Lord_Howe"},
    {-36000, 2, "EST", "EST", "Australia/Sydney"},
    {-36000, 1, "SAKT", "SAKST", "Asia/Sakhalin"},
    {-36000, 1, "VLAT", "VLAST", "Asia/Vladivostok"},
    {-34200, 2, "CST", "CST", "Australia/South"},
    {-32400, 1, "YAKT", "YAKST", "Asia/Yakutsk"},
    {-32400, 1, "CHOT", "CHOST", "Asia/Choibalsan"},
    {-31500, 2, "CWST", "CWST", "Australia/Eucla"},
    {-28800, 1, "IRKT", "IRKST", "Asia/Irkutsk"},
    {-28800, 1, "ULAT", "ULAST", "Asia/Ulaanbaatar"},
    {-28800, 2, "WST", "WST", "Australia/West"},
    {-25200, 1, "HOVT", "HOVST", "Asia/Hovd"},
    {-25200, 1, "KRAT", "KRAST", "Asia/Krasnoyarsk"},
    {-21600, 1, "NOVT", "NOVST", "Asia/Novosibirsk"},
    {-21600, 1, "OMST", "OMSST", "Asia/Omsk"},
    {-18000, 1, "YEKT", "YEKST", "Asia/Yekaterinburg"},
    {-14400, 1, "SAMT", "SAMST", "Europe/Samara"},
    {-14400, 1, "AMT", "AMST", "Asia/Yerevan"},
    {-14400, 1, "AZT", "AZST", "Asia/Baku"},
    {-10800, 1, "AST", "ADT", "Asia/Baghdad"},
    {-10800, 1, "MSK", "MSD", "Europe/Moscow"},
    {-10800, 1, "VOLT", "VOLST", "Europe/Volgograd"},
    {-7200, 0, "EET", "CEST", "Africa/Tripoli"},
    {-7200, 1, "EET", "EEST", "Europe/Athens"}, 
    {-7200, 1, "IST", "IDT", "Asia/Jerusalem"},
    {-3600, 0, "CET", "WEST", "Africa/Algiers"},
    {-3600, 2, "WAT", "WAST", "Africa/Windhoek"},
    {0, 1, "GMT", "IST", "Europe/Dublin"},
    {0, 1, "GMT", "BST", "Europe/London"},
    {0, 0, "WET", "WEST", "Africa/Casablanca"},
    {0, 0, "WET", "WET", "Africa/El_Aaiun"},
    {3600, 1, "AZOT", "AZOST", "Atlantic/Azores"},
    {3600, 1, "EGT", "EGST", "America/Scoresbysund"},
    {10800, 1, "PMST", "PMDT", "America/Miquelon"},
    {10800, 2, "UYT", "UYST", "America/Montevideo"},
    {10800, 1, "WGT", "WGST", "America/Godthab"},
    {10800, 2, "BRT", "BRST", "Brazil/East"},
    {12600, 1, "NST", "NDT", "America/St_Johns"},
    {14400, 1, "AST", "ADT", "Canada/Atlantic"},
    {14400, 2, "AMT", "AMST", "America/Cuiaba"},
    {14400, 2, "CLT", "CLST", "Chile/Continental"},
    {14400, 2, "FKT", "FKST", "Atlantic/Stanley"},
    {14400, 2, "PYT", "PYST", "America/Asuncion"},
    {18000, 1, "CST", "CDT", "America/Havana"},
    {18000, 1, "EST", "EDT", "US/Eastern"}, 
    {21600, 2, "EAST", "EASST", "Chile/EasterIsland"},
    {21600, 0, "CST", "MDT", "Canada/Saskatchewan"},
    {21600, 0, "CST", "CDT", "America/Guatemala"},
    {21600, 1, "CST", "CDT", "US/Central"}, 
    {25200, 1, "MST", "MDT", "US/Mountain"}, 
    {28800, 0, "PST", "PST", "Pacific/Pitcairn"},
    {28800, 1, "PST", "PDT", "US/Pacific"}, 
    {32400, 1, "AKST", "AKDT", "US/Alaska"},
    {36000, 1, "HAST", "HADT", "US/Aleutian"}
};



static const char* remapShortTimeZone(const char *stdID, const char *dstID, int32_t daylightType, int32_t offset)
{
    int32_t idx;
#ifdef DEBUG_TZNAME
    fprintf(stderr, "TZ=%s std=%s dst=%s daylight=%d offset=%d\n", getenv("TZ"), stdID, dstID, daylightType, offset);
#endif
    for (idx = 0; idx < LENGTHOF(OFFSET_ZONE_MAPPINGS); idx++)
    {
        if (offset == OFFSET_ZONE_MAPPINGS[idx].offsetSeconds
            && daylightType == OFFSET_ZONE_MAPPINGS[idx].daylightType
            && strcmp(OFFSET_ZONE_MAPPINGS[idx].stdID, stdID) == 0
            && strcmp(OFFSET_ZONE_MAPPINGS[idx].dstID, dstID) == 0)
        {
            return OFFSET_ZONE_MAPPINGS[idx].olsonID;
        }
    }
    return NULL;
}
#endif

#ifdef SEARCH_TZFILE
#define MAX_PATH_SIZE PATH_MAX /* Set the limit for the size of the path. */
#define MAX_READ_SIZE 512

typedef struct DefaultTZInfo {
    char* defaultTZBuffer;
    int64_t defaultTZFileSize;
    FILE* defaultTZFilePtr;
    UBool defaultTZstatus;
    int32_t defaultTZPosition;
} DefaultTZInfo;





static UBool compareBinaryFiles(const char* defaultTZFileName, const char* TZFileName, DefaultTZInfo* tzInfo) {
    FILE* file; 
    int64_t sizeFile;
    int64_t sizeFileLeft;
    int32_t sizeFileRead;
    int32_t sizeFileToRead;
    char bufferFile[MAX_READ_SIZE];
    UBool result = TRUE;

    if (tzInfo->defaultTZFilePtr == NULL) {
        tzInfo->defaultTZFilePtr = fopen(defaultTZFileName, "r");
    }
    file = fopen(TZFileName, "r");

    tzInfo->defaultTZPosition = 0; 

    if (file != NULL && tzInfo->defaultTZFilePtr != NULL) {
        
        if (tzInfo->defaultTZFileSize == 0) {
            fseek(tzInfo->defaultTZFilePtr, 0, SEEK_END);
            tzInfo->defaultTZFileSize = ftell(tzInfo->defaultTZFilePtr);
        }
        fseek(file, 0, SEEK_END);
        sizeFile = ftell(file);
        sizeFileLeft = sizeFile;

        if (sizeFile != tzInfo->defaultTZFileSize) {
            result = FALSE;
        } else {
            


            if (tzInfo->defaultTZBuffer == NULL) {
                rewind(tzInfo->defaultTZFilePtr);
                tzInfo->defaultTZBuffer = (char*)uprv_malloc(sizeof(char) * tzInfo->defaultTZFileSize);
                sizeFileRead = fread(tzInfo->defaultTZBuffer, 1, tzInfo->defaultTZFileSize, tzInfo->defaultTZFilePtr);
            }
            rewind(file);
            while(sizeFileLeft > 0) {
                uprv_memset(bufferFile, 0, MAX_READ_SIZE);
                sizeFileToRead = sizeFileLeft < MAX_READ_SIZE ? sizeFileLeft : MAX_READ_SIZE;

                sizeFileRead = fread(bufferFile, 1, sizeFileToRead, file);
                if (memcmp(tzInfo->defaultTZBuffer + tzInfo->defaultTZPosition, bufferFile, sizeFileRead) != 0) {
                    result = FALSE;
                    break;
                }
                sizeFileLeft -= sizeFileRead;
                tzInfo->defaultTZPosition += sizeFileRead;
            }
        }
    } else {
        result = FALSE;
    }

    if (file != NULL) {
        fclose(file);
    }

    return result;
}




#define SKIP1 "."
#define SKIP2 ".."
static char SEARCH_TZFILE_RESULT[MAX_PATH_SIZE] = "";
static char* searchForTZFile(const char* path, DefaultTZInfo* tzInfo) {
    char curpath[MAX_PATH_SIZE];
    DIR* dirp = opendir(path);
    DIR* subDirp = NULL;
    struct dirent* dirEntry = NULL;

    char* result = NULL;
    if (dirp == NULL) {
        return result;
    }

    
    uprv_memset(curpath, 0, MAX_PATH_SIZE);
    uprv_strcpy(curpath, path);

    
    while((dirEntry = readdir(dirp)) != NULL) {
        const char* dirName = dirEntry->d_name;
        if (uprv_strcmp(dirName, SKIP1) != 0 && uprv_strcmp(dirName, SKIP2) != 0) {
            
            char newpath[MAX_PATH_SIZE];
            uprv_strcpy(newpath, curpath);
            uprv_strcat(newpath, dirName);

            if ((subDirp = opendir(newpath)) != NULL) {
                
                closedir(subDirp);
                uprv_strcat(newpath, "/");
                result = searchForTZFile(newpath, tzInfo);
                







                if (result != NULL)
                    break;
            } else if (uprv_strcmp(TZFILE_SKIP, dirName) != 0 && uprv_strcmp(TZFILE_SKIP2, dirName) != 0) {
                if(compareBinaryFiles(TZDEFAULT, newpath, tzInfo)) {
                    const char* zoneid = newpath + (sizeof(TZZONEINFO)) - 1;
                    skipZoneIDPrefix(&zoneid);
                    uprv_strcpy(SEARCH_TZFILE_RESULT, zoneid);
                    result = SEARCH_TZFILE_RESULT;
                    
                    break;
                }
            }
        }
    }
    closedir(dirp);
    return result;
}
#endif
U_CAPI const char* U_EXPORT2
uprv_tzname(int n)
{
    const char *tzid = NULL;
#if U_PLATFORM_USES_ONLY_WIN32_API
    tzid = uprv_detectWindowsTimeZone();

    if (tzid != NULL) {
        return tzid;
    }
#else











#ifndef DEBUG_TZNAME
    tzid = getenv("TZ");
    if (tzid != NULL && isValidOlsonID(tzid))
    {
        
        skipZoneIDPrefix(&tzid);
        return tzid;
    }
    
#endif

#if defined(CHECK_LOCALTIME_LINK) && !defined(DEBUG_SKIP_LOCALTIME_LINK)
    
    if (gTimeZoneBufferPtr == NULL) {
        




        int32_t ret = (int32_t)readlink(TZDEFAULT, gTimeZoneBuffer, sizeof(gTimeZoneBuffer));
        if (0 < ret) {
            int32_t tzZoneInfoLen = uprv_strlen(TZZONEINFO);
            gTimeZoneBuffer[ret] = 0;
            if (uprv_strncmp(gTimeZoneBuffer, TZZONEINFO, tzZoneInfoLen) == 0
                && isValidOlsonID(gTimeZoneBuffer + tzZoneInfoLen))
            {
                return (gTimeZoneBufferPtr = gTimeZoneBuffer + tzZoneInfoLen);
            }
        } else {
#if defined(SEARCH_TZFILE)
            DefaultTZInfo* tzInfo = (DefaultTZInfo*)uprv_malloc(sizeof(DefaultTZInfo));
            if (tzInfo != NULL) {
                tzInfo->defaultTZBuffer = NULL;
                tzInfo->defaultTZFileSize = 0;
                tzInfo->defaultTZFilePtr = NULL;
                tzInfo->defaultTZstatus = FALSE;
                tzInfo->defaultTZPosition = 0;

                gTimeZoneBufferPtr = searchForTZFile(TZZONEINFO, tzInfo);

                
                if (tzInfo->defaultTZBuffer != NULL) {
                    uprv_free(tzInfo->defaultTZBuffer);
                }
                if (tzInfo->defaultTZFilePtr != NULL) {
                    fclose(tzInfo->defaultTZFilePtr);
                }
                uprv_free(tzInfo);
            }

            if (gTimeZoneBufferPtr != NULL && isValidOlsonID(gTimeZoneBufferPtr)) {
                return gTimeZoneBufferPtr;
            }
#endif
        }
    }
    else {
        return gTimeZoneBufferPtr;
    }
#endif
#endif

#ifdef U_TZNAME
#if U_PLATFORM_USES_ONLY_WIN32_API
    

    return uprv_strdup(U_TZNAME[n]);
#else
    







    {
        struct tm juneSol, decemberSol;
        int daylightType;
        static const time_t juneSolstice=1182478260; 
        static const time_t decemberSolstice=1198332540; 

        
        localtime_r(&juneSolstice, &juneSol);
        localtime_r(&decemberSolstice, &decemberSol);
        if(decemberSol.tm_isdst > 0) {
          daylightType = U_DAYLIGHT_DECEMBER;
        } else if(juneSol.tm_isdst > 0) {
          daylightType = U_DAYLIGHT_JUNE;
        } else {
          daylightType = U_DAYLIGHT_NONE;
        }
        tzid = remapShortTimeZone(U_TZNAME[0], U_TZNAME[1], daylightType, uprv_timezone());
        if (tzid != NULL) {
            return tzid;
        }
    }
    return U_TZNAME[n];
#endif
#else
    return "";
#endif
}



static char *gDataDirectory = NULL;
#if U_POSIX_LOCALE
 static char *gCorrectedPOSIXLocale = NULL; 
#endif

static UBool U_CALLCONV putil_cleanup(void)
{
    if (gDataDirectory && *gDataDirectory) {
        uprv_free(gDataDirectory);
    }
    gDataDirectory = NULL;
#if U_POSIX_LOCALE
    if (gCorrectedPOSIXLocale) {
        uprv_free(gCorrectedPOSIXLocale);
        gCorrectedPOSIXLocale = NULL;
    }
#endif
    return TRUE;
}






U_CAPI void U_EXPORT2
u_setDataDirectory(const char *directory) {
    char *newDataDir;
    int32_t length;

    if(directory==NULL || *directory==0) {
        



        newDataDir = (char *)"";
    }
    else {
        length=(int32_t)uprv_strlen(directory);
        newDataDir = (char *)uprv_malloc(length + 2);
        
        if (newDataDir == NULL) {
            return;
        }
        uprv_strcpy(newDataDir, directory);

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
        {
            char *p;
            while(p = uprv_strchr(newDataDir, U_FILE_ALT_SEP_CHAR)) {
                *p = U_FILE_SEP_CHAR;
            }
        }
#endif
    }

    umtx_lock(NULL);
    if (gDataDirectory && *gDataDirectory) {
        uprv_free(gDataDirectory);
    }
    gDataDirectory = newDataDir;
    ucln_common_registerCleanup(UCLN_COMMON_PUTIL, putil_cleanup);
    umtx_unlock(NULL);
}

U_CAPI UBool U_EXPORT2
uprv_pathIsAbsolute(const char *path)
{
  if(!path || !*path) {
    return FALSE;
  }

  if(*path == U_FILE_SEP_CHAR) {
    return TRUE;
  }

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
  if(*path == U_FILE_ALT_SEP_CHAR) {
    return TRUE;
  }
#endif

#if U_PLATFORM_USES_ONLY_WIN32_API
  if( (((path[0] >= 'A') && (path[0] <= 'Z')) ||
       ((path[0] >= 'a') && (path[0] <= 'z'))) &&
      path[1] == ':' ) {
    return TRUE;
  }
#endif

  return FALSE;
}



#if U_PLATFORM_IS_DARWIN_BASED && TARGET_IPHONE_SIMULATOR
# if !defined(ICU_DATA_DIR_PREFIX_ENV_VAR)
#  define ICU_DATA_DIR_PREFIX_ENV_VAR "IPHONE_SIMULATOR_ROOT"
# endif
#endif

U_CAPI const char * U_EXPORT2
u_getDataDirectory(void) {
    const char *path = NULL;
#if defined(ICU_DATA_DIR_PREFIX_ENV_VAR)
    char datadir_path_buffer[PATH_MAX];
#endif

    
    UMTX_CHECK(NULL, gDataDirectory, path);

    if(path) {
        return path;
    }

    












#   if !defined(ICU_NO_USER_DATA_OVERRIDE) && !UCONFIG_NO_FILE_IO
    
    path=getenv("ICU_DATA");
#   endif

    






#if defined(ICU_DATA_DIR) || defined(U_ICU_DATA_DEFAULT_DIR)
    if(path==NULL || *path==0) {
# if defined(ICU_DATA_DIR_PREFIX_ENV_VAR)
        const char *prefix = getenv(ICU_DATA_DIR_PREFIX_ENV_VAR);
# endif
# ifdef ICU_DATA_DIR
        path=ICU_DATA_DIR;
# else
        path=U_ICU_DATA_DEFAULT_DIR;
# endif
# if defined(ICU_DATA_DIR_PREFIX_ENV_VAR)
        if (prefix != NULL) {
            snprintf(datadir_path_buffer, PATH_MAX, "%s%s", prefix, path);
            path=datadir_path_buffer;
        }
# endif
    }
#endif

    if(path==NULL) {
        
        path = "";
    }

    u_setDataDirectory(path);
    return gDataDirectory;
}






#if U_PLATFORM == U_PF_CLASSIC_MACOS

typedef struct {
    int32_t script;
    int32_t region;
    int32_t lang;
    int32_t date_region;
    const char* posixID;
} mac_lc_rec;



#define MAC_LC_MAGIC_NUMBER -5
#define MAC_LC_INIT_NUMBER -9

static const mac_lc_rec mac_lc_recs[] = {
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 0, "en_US",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 1, "fr_FR",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 2, "en_GB",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 3, "de_DE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 4, "it_IT",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 5, "nl_NL",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 6, "fr_BE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 7, "sv_SE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 9, "da_DK",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 10, "pt_PT",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 11, "fr_CA",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 13, "is_IS",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 14, "ja_JP",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 15, "en_AU",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 16, "ar_AE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 17, "fi_FI",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 18, "fr_CH",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 19, "de_CH",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 20, "el_GR",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 21, "is_IS",
    
    
    
    
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 24, "tr_TR",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 25, "sh_YU",
    
    
    
    
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 41, "lt_LT",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 42, "pl_PL",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 43, "hu_HU",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 44, "et_EE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 45, "lv_LV",
    
    
    
    
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 48, "fa_IR",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 49, "ru_RU",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 50, "en_IE",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 51, "ko_KR",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 52, "zh_CN",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 53, "zh_TW",
    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 54, "th_TH",
    

    
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER,
    MAC_LC_MAGIC_NUMBER, "en_US"
};

#endif

#if U_POSIX_LOCALE




static const char *uprv_getPOSIXIDForCategory(int category)
{
    const char* posixID = NULL;
    if (category == LC_MESSAGES || category == LC_CTYPE) {
        

















        posixID = setlocale(category, NULL);
        if ((posixID == 0)
            || (uprv_strcmp("C", posixID) == 0)
            || (uprv_strcmp("POSIX", posixID) == 0))
        {
            
            posixID = getenv("LC_ALL");
            if (posixID == 0) {
                posixID = getenv(category == LC_MESSAGES ? "LC_MESSAGES" : "LC_CTYPE");
                if (posixID == 0) {
                    posixID = getenv("LANG");
                }
            }
        }
    }
    if ((posixID==0)
        || (uprv_strcmp("C", posixID) == 0)
        || (uprv_strcmp("POSIX", posixID) == 0))
    {
        
        posixID = "en_US_POSIX";
    }
    return posixID;
}




static const char *uprv_getPOSIXIDForDefaultLocale(void)
{
    static const char* posixID = NULL;
    if (posixID == 0) {
        posixID = uprv_getPOSIXIDForCategory(LC_MESSAGES);
    }
    return posixID;
}

#if !U_CHARSET_IS_UTF8



static const char *uprv_getPOSIXIDForDefaultCodepage(void)
{
    static const char* posixID = NULL;
    if (posixID == 0) {
        posixID = uprv_getPOSIXIDForCategory(LC_CTYPE);
    }
    return posixID;
}
#endif
#endif


U_CAPI const char* U_EXPORT2
uprv_getDefaultLocaleID()
{
#if U_POSIX_LOCALE






















    char *correctedPOSIXLocale = 0;
    const char* posixID = uprv_getPOSIXIDForDefaultLocale();
    const char *p;
    const char *q;
    int32_t len;

    





    if (gCorrectedPOSIXLocale != NULL) {
        return gCorrectedPOSIXLocale;
    }

    if ((p = uprv_strchr(posixID, '.')) != NULL) {
        
        correctedPOSIXLocale = static_cast<char *>(uprv_malloc(uprv_strlen(posixID)+1));
        
        if (correctedPOSIXLocale == NULL) {
            return NULL;
        }
        uprv_strncpy(correctedPOSIXLocale, posixID, p-posixID);
        correctedPOSIXLocale[p-posixID] = 0;

        
        if ((p = uprv_strchr(correctedPOSIXLocale, '@')) != NULL) {
            correctedPOSIXLocale[p-correctedPOSIXLocale] = 0;
        }
    }

    
    if ((p = uprv_strrchr(posixID, '@')) != NULL) {
        if (correctedPOSIXLocale == NULL) {
            correctedPOSIXLocale = static_cast<char *>(uprv_malloc(uprv_strlen(posixID)+1));
            
            if (correctedPOSIXLocale == NULL) {
                return NULL;
            }
            uprv_strncpy(correctedPOSIXLocale, posixID, p-posixID);
            correctedPOSIXLocale[p-posixID] = 0;
        }
        p++;

        
        if (!uprv_strcmp(p, "nynorsk")) {
            p = "NY";
            
        }

        if (uprv_strchr(correctedPOSIXLocale,'_') == NULL) {
            uprv_strcat(correctedPOSIXLocale, "__"); 
        }
        else {
            uprv_strcat(correctedPOSIXLocale, "_"); 
        }

        if ((q = uprv_strchr(p, '.')) != NULL) {
            
            len = (int32_t)(uprv_strlen(correctedPOSIXLocale) + (q-p));
            uprv_strncat(correctedPOSIXLocale, p, q-p);
            correctedPOSIXLocale[len] = 0;
        }
        else {
            
            uprv_strcat(correctedPOSIXLocale, p);
        }

        




    }

    
    if (correctedPOSIXLocale != NULL) {
        posixID = correctedPOSIXLocale;
    }
    else {
        
        correctedPOSIXLocale = (char *)uprv_malloc(uprv_strlen(posixID) + 1);
        
        if (correctedPOSIXLocale == NULL) {
            return NULL;
        }
        posixID = uprv_strcpy(correctedPOSIXLocale, posixID);
    }

    if (gCorrectedPOSIXLocale == NULL) {
        gCorrectedPOSIXLocale = correctedPOSIXLocale;
        ucln_common_registerCleanup(UCLN_COMMON_PUTIL, putil_cleanup);
        correctedPOSIXLocale = NULL;
    }

    if (correctedPOSIXLocale != NULL) {  
        uprv_free(correctedPOSIXLocale);
    }

    return posixID;

#elif U_PLATFORM_USES_ONLY_WIN32_API
    UErrorCode status = U_ZERO_ERROR;
    LCID id = GetThreadLocale();
    const char* locID = uprv_convertToPosix(id, &status);

    if (U_FAILURE(status)) {
        locID = "en_US";
    }
    return locID;

#elif U_PLATFORM == U_PF_CLASSIC_MACOS
    int32_t script = MAC_LC_INIT_NUMBER;
    
    int32_t region = MAC_LC_INIT_NUMBER;
    
    int32_t lang = MAC_LC_INIT_NUMBER;
    
    int32_t date_region = MAC_LC_INIT_NUMBER;
    const char* posixID = 0;
    int32_t count = sizeof(mac_lc_recs) / sizeof(mac_lc_rec);
    int32_t i;
    Intl1Hndl ih;

    ih = (Intl1Hndl) GetIntlResource(1);
    if (ih)
        date_region = ((uint16_t)(*ih)->intl1Vers) >> 8;

    for (i = 0; i < count; i++) {
        if (   ((mac_lc_recs[i].script == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].script == script))
            && ((mac_lc_recs[i].region == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].region == region))
            && ((mac_lc_recs[i].lang == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].lang == lang))
            && ((mac_lc_recs[i].date_region == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].date_region == date_region))
            )
        {
            posixID = mac_lc_recs[i].posixID;
            break;
        }
    }

    return posixID;

#elif U_PLATFORM == U_PF_OS400
    
    static char correctedLocale[64];
    const  char *localeID = getenv("LC_ALL");
           char *p;

    if (localeID == NULL)
        localeID = getenv("LANG");
    if (localeID == NULL)
        localeID = setlocale(LC_ALL, NULL);
    
    if (localeID == NULL)
        return "en_US_POSIX";

    
    if((p = uprv_strrchr(localeID, '/')) != NULL)
    {
        
        p++;
        localeID = p;
    }

    
    uprv_strcpy(correctedLocale, localeID);

    
    if((p = uprv_strchr(correctedLocale, '.')) != NULL) {
        *p = 0;
    }

    
    T_CString_toUpperCase(correctedLocale);

    






    if ((uprv_strcmp("C", correctedLocale) == 0) ||
        (uprv_strcmp("POSIX", correctedLocale) == 0) ||
        (uprv_strncmp("QLGPGCMA", correctedLocale, 8) == 0))
    {
        uprv_strcpy(correctedLocale, "en_US_POSIX");
    }
    else
    {
        int16_t LocaleLen;

        
        for(p = correctedLocale; *p != 0 && *p != '_'; p++)
        {
            *p = uprv_tolower(*p);
        }

        
        LocaleLen = uprv_strlen(correctedLocale);
        if (correctedLocale[LocaleLen - 2] == '_' &&
            correctedLocale[LocaleLen - 1] == 'E')
        {
            uprv_strcat(correctedLocale, "URO");
        }

        


        else if (correctedLocale[LocaleLen - 2] == '_' &&
            correctedLocale[LocaleLen - 1] == 'L')
        {
            correctedLocale[LocaleLen - 2] = 0;
        }

        


        else if (uprv_strncmp(correctedLocale, "zh_HK", 5) == 0)
        {
            uprv_strcpy(correctedLocale, "zh_HK");
        }

        

        else if (uprv_strcmp(correctedLocale, "zh_CN_GBK") == 0)
        {
            uprv_strcpy(correctedLocale, "zh_CN");
        }

    }

    return correctedLocale;
#endif

}

#if !U_CHARSET_IS_UTF8
#if U_POSIX_LOCALE







static const char*
remapPlatformDependentCodepage(const char *locale, const char *name) {
    if (locale != NULL && *locale == 0) {
        
        locale = NULL;
    }
    if (name == NULL) {
        return NULL;
    }
#if U_PLATFORM == U_PF_AIX
    if (uprv_strcmp(name, "IBM-943") == 0) {
        
        name = "Shift-JIS";
    }
    else if (uprv_strcmp(name, "IBM-1252") == 0) {
        
        name = "IBM-5348";
    }
#elif U_PLATFORM == U_PF_SOLARIS
    if (locale != NULL && uprv_strcmp(name, "EUC") == 0) {
        
        if (uprv_strcmp(locale, "zh_CN") == 0) {
            name = "EUC-CN";
        }
        else if (uprv_strcmp(locale, "zh_TW") == 0) {
            name = "EUC-TW";
        }
        else if (uprv_strcmp(locale, "ko_KR") == 0) {
            name = "EUC-KR";
        }
    }
    else if (uprv_strcmp(name, "eucJP") == 0) {
        



        name = "eucjis";
    }
    else if (uprv_strcmp(name, "646") == 0) {
        



        name = "ISO-8859-1";
    }
#elif U_PLATFORM_IS_DARWIN_BASED
    if (locale == NULL && *name == 0) {
        




        name = "UTF-8";
    }
    else if (uprv_strcmp(name, "CP949") == 0) {
        
        name = "EUC-KR";
    }
    else if (locale != NULL && uprv_strcmp(locale, "en_US_POSIX") != 0 && uprv_strcmp(name, "US-ASCII") == 0) {
        


        name = "UTF-8";
    }
#elif U_PLATFORM == U_PF_BSD
    if (uprv_strcmp(name, "CP949") == 0) {
        
        name = "EUC-KR";
    }
#elif U_PLATFORM == U_PF_HPUX
    if (locale != NULL && uprv_strcmp(locale, "zh_HK") == 0 && uprv_strcmp(name, "big5") == 0) {
        
        
        name = "hkbig5";
    }
    else if (uprv_strcmp(name, "eucJP") == 0) {
        




        name = "eucjis";
    }
#elif U_PLATFORM == U_PF_LINUX
    if (locale != NULL && uprv_strcmp(name, "euc") == 0) {
        
        if (uprv_strcmp(locale, "korean") == 0) {
            name = "EUC-KR";
        }
        else if (uprv_strcmp(locale, "japanese") == 0) {
            
            name = "eucjis";
        }
    }
    else if (uprv_strcmp(name, "eucjp") == 0) {
        




        name = "eucjis";
    }
    else if (locale != NULL && uprv_strcmp(locale, "en_US_POSIX") != 0 &&
            (uprv_strcmp(name, "ANSI_X3.4-1968") == 0 || uprv_strcmp(name, "US-ASCII") == 0)) {
        


        name = "UTF-8";
    }
    




#endif
    
    if (*name == 0) {
        name = NULL;
    }
    return name;
}

static const char*
getCodepageFromPOSIXID(const char *localeName, char * buffer, int32_t buffCapacity)
{
    char localeBuf[100];
    const char *name = NULL;
    char *variant = NULL;

    if (localeName != NULL && (name = (uprv_strchr(localeName, '.'))) != NULL) {
        size_t localeCapacity = uprv_min(sizeof(localeBuf), (name-localeName)+1);
        uprv_strncpy(localeBuf, localeName, localeCapacity);
        localeBuf[localeCapacity-1] = 0; 
        name = uprv_strncpy(buffer, name+1, buffCapacity);
        buffer[buffCapacity-1] = 0; 
        if ((variant = const_cast<char *>(uprv_strchr(name, '@'))) != NULL) {
            *variant = 0;
        }
        name = remapPlatformDependentCodepage(localeBuf, name);
    }
    return name;
}
#endif

static const char*
int_getDefaultCodepage()
{
#if U_PLATFORM == U_PF_OS400
    uint32_t ccsid = 37; 
    static char codepage[64];
    Qwc_JOBI0400_t jobinfo;
    Qus_EC_t error = { sizeof(Qus_EC_t) }; 

    EPT_CALL(QUSRJOBI)(&jobinfo, sizeof(jobinfo), "JOBI0400",
        "*                         ", "                ", &error);

    if (error.Bytes_Available == 0) {
        if (jobinfo.Coded_Char_Set_ID != 0xFFFF) {
            ccsid = (uint32_t)jobinfo.Coded_Char_Set_ID;
        }
        else if (jobinfo.Default_Coded_Char_Set_Id != 0xFFFF) {
            ccsid = (uint32_t)jobinfo.Default_Coded_Char_Set_Id;
        }
        
    }
    sprintf(codepage,"ibm-%d", ccsid);
    return codepage;

#elif U_PLATFORM == U_PF_OS390
    static char codepage[64];

    strncpy(codepage, nl_langinfo(CODESET),63-strlen(UCNV_SWAP_LFNL_OPTION_STRING));
    strcat(codepage,UCNV_SWAP_LFNL_OPTION_STRING);
    codepage[63] = 0; 

    return codepage;

#elif U_PLATFORM == U_PF_CLASSIC_MACOS
    return "macintosh"; 

#elif U_PLATFORM_USES_ONLY_WIN32_API
    static char codepage[64];
    sprintf(codepage, "windows-%d", GetACP());
    return codepage;

#elif U_POSIX_LOCALE
    static char codesetName[100];
    const char *localeName = NULL;
    const char *name = NULL;

    localeName = uprv_getPOSIXIDForDefaultCodepage();
    uprv_memset(codesetName, 0, sizeof(codesetName));
#if U_HAVE_NL_LANGINFO_CODESET
    


    {
        const char *codeset = nl_langinfo(U_NL_LANGINFO_CODESET);
#if U_PLATFORM_IS_DARWIN_BASED || U_PLATFORM_IS_LINUX_BASED
        



        if (uprv_strcmp(localeName, "en_US_POSIX") != 0) {
            codeset = remapPlatformDependentCodepage(localeName, codeset);
        } else
#endif
        {
            codeset = remapPlatformDependentCodepage(NULL, codeset);
        }

        if (codeset != NULL) {
            uprv_strncpy(codesetName, codeset, sizeof(codesetName));
            codesetName[sizeof(codesetName)-1] = 0;
            return codesetName;
        }
    }
#endif

    


    uprv_memset(codesetName, 0, sizeof(codesetName));
    name = getCodepageFromPOSIXID(localeName, codesetName, sizeof(codesetName));
    if (name) {
        
        return name;
    }

    if (*codesetName == 0)
    {
        
        (void)uprv_strcpy(codesetName, "US-ASCII");
    }
    return codesetName;
#else
    return "US-ASCII";
#endif
}


U_CAPI const char*  U_EXPORT2
uprv_getDefaultCodepage()
{
    static char const  *name = NULL;
    umtx_lock(NULL);
    if (name == NULL) {
        name = int_getDefaultCodepage();
    }
    umtx_unlock(NULL);
    return name;
}
#endif  






U_CAPI void U_EXPORT2
u_versionFromString(UVersionInfo versionArray, const char *versionString) {
    char *end;
    uint16_t part=0;

    if(versionArray==NULL) {
        return;
    }

    if(versionString!=NULL) {
        for(;;) {
            versionArray[part]=(uint8_t)uprv_strtoul(versionString, &end, 10);
            if(end==versionString || ++part==U_MAX_VERSION_LENGTH || *end!=U_VERSION_DELIMITER) {
                break;
            }
            versionString=end+1;
        }
    }

    while(part<U_MAX_VERSION_LENGTH) {
        versionArray[part++]=0;
    }
}

U_CAPI void U_EXPORT2
u_versionFromUString(UVersionInfo versionArray, const UChar *versionString) {
    if(versionArray!=NULL && versionString!=NULL) {
        char versionChars[U_MAX_VERSION_STRING_LENGTH+1];
        int32_t len = u_strlen(versionString);
        if(len>U_MAX_VERSION_STRING_LENGTH) {
            len = U_MAX_VERSION_STRING_LENGTH;
        }
        u_UCharsToChars(versionString, versionChars, len);
        versionChars[len]=0;
        u_versionFromString(versionArray, versionChars);
    }
}

U_CAPI void U_EXPORT2
u_versionToString(const UVersionInfo versionArray, char *versionString) {
    uint16_t count, part;
    uint8_t field;

    if(versionString==NULL) {
        return;
    }

    if(versionArray==NULL) {
        versionString[0]=0;
        return;
    }

    
    for(count=4; count>0 && versionArray[count-1]==0; --count) {
    }

    if(count <= 1) {
        count = 2;
    }

    
    
    field=versionArray[0];
    if(field>=100) {
        *versionString++=(char)('0'+field/100);
        field%=100;
    }
    if(field>=10) {
        *versionString++=(char)('0'+field/10);
        field%=10;
    }
    *versionString++=(char)('0'+field);

    
    for(part=1; part<count; ++part) {
        
        *versionString++=U_VERSION_DELIMITER;

        
        field=versionArray[part];
        if(field>=100) {
            *versionString++=(char)('0'+field/100);
            field%=100;
        }
        if(field>=10) {
            *versionString++=(char)('0'+field/10);
            field%=10;
        }
        *versionString++=(char)('0'+field);
    }

    
    *versionString=0;
}

U_CAPI void U_EXPORT2
u_getVersion(UVersionInfo versionArray) {
    u_versionFromString(versionArray, U_ICU_VERSION);
}





#if U_ENABLE_DYLOAD
 
#if HAVE_DLOPEN && !U_PLATFORM_USES_ONLY_WIN32_API

#if HAVE_DLFCN_H

#ifdef __MVS__
#ifndef __SUSV3
#define __SUSV3 1
#endif
#endif
#include <dlfcn.h>
#endif

U_INTERNAL void * U_EXPORT2
uprv_dl_open(const char *libName, UErrorCode *status) {
  void *ret = NULL;
  if(U_FAILURE(*status)) return ret;
  ret =  dlopen(libName, RTLD_NOW|RTLD_GLOBAL);
  if(ret==NULL) {
#ifdef U_TRACE_DYLOAD
    printf("dlerror on dlopen(%s): %s\n", libName, dlerror());
#endif
    *status = U_MISSING_RESOURCE_ERROR;
  }
  return ret;
}

U_INTERNAL void U_EXPORT2
uprv_dl_close(void *lib, UErrorCode *status) {
  if(U_FAILURE(*status)) return;
  dlclose(lib);
}

U_INTERNAL UVoidFunction* U_EXPORT2
uprv_dlsym_func(void *lib, const char* sym, UErrorCode *status) {
  union {
      UVoidFunction *fp;
      void *vp;
  } uret;
  uret.fp = NULL;
  if(U_FAILURE(*status)) return uret.fp;
  uret.vp = dlsym(lib, sym);
  if(uret.vp == NULL) {
#ifdef U_TRACE_DYLOAD
    printf("dlerror on dlsym(%p,%s): %s\n", lib,sym, dlerror());
#endif
    *status = U_MISSING_RESOURCE_ERROR;
  }
  return uret.fp;
}

#else



U_INTERNAL void * U_EXPORT2
uprv_dl_open(const char *libName, UErrorCode *status) {
  if(U_FAILURE(*status)) return NULL;
  *status = U_UNSUPPORTED_ERROR;
  return NULL;
}

U_INTERNAL void U_EXPORT2
uprv_dl_close(void *lib, UErrorCode *status) {
  if(U_FAILURE(*status)) return;
  *status = U_UNSUPPORTED_ERROR;
  return;
}


U_INTERNAL UVoidFunction* U_EXPORT2
uprv_dlsym_func(void *lib, const char* sym, UErrorCode *status) {
  if(U_SUCCESS(*status)) {
    *status = U_UNSUPPORTED_ERROR;
  }
  return (UVoidFunction*)NULL;
}



#endif

#elif U_PLATFORM_USES_ONLY_WIN32_API

U_INTERNAL void * U_EXPORT2
uprv_dl_open(const char *libName, UErrorCode *status) {
  HMODULE lib = NULL;
  
  if(U_FAILURE(*status)) return NULL;
  
  lib = LoadLibraryA(libName);
  
  if(lib==NULL) {
    *status = U_MISSING_RESOURCE_ERROR;
  }
  
  return (void*)lib;
}

U_INTERNAL void U_EXPORT2
uprv_dl_close(void *lib, UErrorCode *status) {
  HMODULE handle = (HMODULE)lib;
  if(U_FAILURE(*status)) return;
  
  FreeLibrary(handle);
  
  return;
}


U_INTERNAL UVoidFunction* U_EXPORT2
uprv_dlsym_func(void *lib, const char* sym, UErrorCode *status) {
  HMODULE handle = (HMODULE)lib;
  UVoidFunction* addr = NULL;
  
  if(U_FAILURE(*status) || lib==NULL) return NULL;
  
  addr = (UVoidFunction*)GetProcAddress(handle, sym);
  
  if(addr==NULL) {
    DWORD lastError = GetLastError();
    if(lastError == ERROR_PROC_NOT_FOUND) {
      *status = U_MISSING_RESOURCE_ERROR;
    } else {
      *status = U_UNSUPPORTED_ERROR; 
    }
  }
  
  return addr;
}


#else



U_INTERNAL void * U_EXPORT2
uprv_dl_open(const char *libName, UErrorCode *status) {
    if(U_FAILURE(*status)) return NULL;
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

U_INTERNAL void U_EXPORT2
uprv_dl_close(void *lib, UErrorCode *status) {
    if(U_FAILURE(*status)) return;
    *status = U_UNSUPPORTED_ERROR;
    return;
}


U_INTERNAL UVoidFunction* U_EXPORT2
uprv_dlsym_func(void *lib, const char* sym, UErrorCode *status) {
  if(U_SUCCESS(*status)) {
    *status = U_UNSUPPORTED_ERROR;
  }
  return (UVoidFunction*)NULL;
}

#endif 









