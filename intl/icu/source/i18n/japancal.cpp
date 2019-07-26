












#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "japancal.h"
#include "unicode/gregocal.h"
#include "umutex.h"
#include "uassert.h"



#ifdef U_DEBUG_JCAL
#include <stdio.h>
#endif

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(JapaneseCalendar)



static const struct { 
    int16_t year;
    int8_t  month;
    int8_t  day;
} kEraInfo[] =  {
    
    {   645,    6, 19 },   
    {   650,    2, 15 },   
    {   672,    1,  1 },   
    {   686,    7, 20 },   
    {   701,    3, 21 },   
    {   704,    5, 10 },   
    {   708,    1, 11 },   
    {   715,    9,  2 },   
    {   717,   11, 17 },   
    {   724,    2,  4 },   
    {   729,    8,  5 },   
    {   749,    4, 14 },   
    {   749,    7,  2 },   
    {   757,    8, 18 },   
    {   765,    1,  7 },   
    {   767,    8, 16 },   
    {   770,   10,  1 },   
    {   781,    1,  1 },   
    {   782,    8, 19 },   
    {   806,    5, 18 },   
    {   810,    9, 19 },   
    {   824,    1,  5 },   
    {   834,    1,  3 },   
    {   848,    6, 13 },   
    {   851,    4, 28 },   
    {   854,   11, 30 },   
    {   857,    2, 21 },   
    {   859,    4, 15 },   
    {   877,    4, 16 },   
    {   885,    2, 21 },   
    {   889,    4, 27 },   
    {   898,    4, 26 },   
    {   901,    7, 15 },   
    {   923,    4, 11 },   
    {   931,    4, 26 },   
    {   938,    5, 22 },   
    {   947,    4, 22 },   
    {   957,   10, 27 },   
    {   961,    2, 16 },   
    {   964,    7, 10 },   
    {   968,    8, 13 },   
    {   970,    3, 25 },   
    {   973,   12, 20 },   
    {   976,    7, 13 },   
    {   978,   11, 29 },   
    {   983,    4, 15 },   
    {   985,    4, 27 },   
    {   987,    4,  5 },   
    {   989,    8,  8 },   
    {   990,   11,  7 },   
    {   995,    2, 22 },   
    {   999,    1, 13 },   
    {  1004,    7, 20 },   
    {  1012,   12, 25 },   
    {  1017,    4, 23 },   
    {  1021,    2,  2 },   
    {  1024,    7, 13 },   
    {  1028,    7, 25 },   
    {  1037,    4, 21 },   
    {  1040,   11, 10 },   
    {  1044,   11, 24 },   
    {  1046,    4, 14 },   
    {  1053,    1, 11 },   
    {  1058,    8, 29 },   
    {  1065,    8,  2 },   
    {  1069,    4, 13 },   
    {  1074,    8, 23 },   
    {  1077,   11, 17 },   
    {  1081,    2, 10 },   
    {  1084,    2,  7 },   
    {  1087,    4,  7 },   
    {  1094,   12, 15 },   
    {  1096,   12, 17 },   
    {  1097,   11, 21 },   
    {  1099,    8, 28 },   
    {  1104,    2, 10 },   
    {  1106,    4,  9 },   
    {  1108,    8,  3 },   
    {  1110,    7, 13 },   
    {  1113,    7, 13 },   
    {  1118,    4,  3 },   
    {  1120,    4, 10 },   
    {  1124,    4,  3 },   
    {  1126,    1, 22 },   
    {  1131,    1, 29 },   
    {  1132,    8, 11 },   
    {  1135,    4, 27 },   
    {  1141,    7, 10 },   
    {  1142,    4, 28 },   
    {  1144,    2, 23 },   
    {  1145,    7, 22 },   
    {  1151,    1, 26 },   
    {  1154,   10, 28 },   
    {  1156,    4, 27 },   
    {  1159,    4, 20 },   
    {  1160,    1, 10 },   
    {  1161,    9,  4 },   
    {  1163,    3, 29 },   
    {  1165,    6,  5 },   
    {  1166,    8, 27 },   
    {  1169,    4,  8 },   
    {  1171,    4, 21 },   
    {  1175,    7, 28 },   
    {  1177,    8,  4 },   
    {  1181,    7, 14 },   
    {  1182,    5, 27 },   
    {  1184,    4, 16 },   
    {  1185,    8, 14 },   
    {  1190,    4, 11 },   
    {  1199,    4, 27 },   
    {  1201,    2, 13 },   
    {  1204,    2, 20 },   
    {  1206,    4, 27 },   
    {  1207,   10, 25 },   
    {  1211,    3,  9 },   
    {  1213,   12,  6 },   
    {  1219,    4, 12 },   
    {  1222,    4, 13 },   
    {  1224,   11, 20 },   
    {  1225,    4, 20 },   
    {  1227,   12, 10 },   
    {  1229,    3,  5 },   
    {  1232,    4,  2 },   
    {  1233,    4, 15 },   
    {  1234,   11,  5 },   
    {  1235,    9, 19 },   
    {  1238,   11, 23 },   
    {  1239,    2,  7 },   
    {  1240,    7, 16 },   
    {  1243,    2, 26 },   
    {  1247,    2, 28 },   
    {  1249,    3, 18 },   
    {  1256,   10,  5 },   
    {  1257,    3, 14 },   
    {  1259,    3, 26 },   
    {  1260,    4, 13 },   
    {  1261,    2, 20 },   
    {  1264,    2, 28 },   
    {  1275,    4, 25 },   
    {  1278,    2, 29 },   
    {  1288,    4, 28 },   
    {  1293,    8, 55 },   
    {  1299,    4, 25 },   
    {  1302,   11, 21 },   
    {  1303,    8,  5 },   
    {  1306,   12, 14 },   
    {  1308,   10,  9 },   
    {  1311,    4, 28 },   
    {  1312,    3, 20 },   
    {  1317,    2,  3 },   
    {  1319,    4, 28 },   
    {  1321,    2, 23 },   
    {  1324,   12,  9 },   
    {  1326,    4, 26 },   
    {  1329,    8, 29 },   
    {  1331,    8,  9 },   
    {  1334,    1, 29 },   
    {  1336,    2, 29 },   
    {  1340,    4, 28 },   
    {  1346,   12,  8 },   
    {  1370,    7, 24 },   
    {  1372,    4,  1 },   
    {  1375,    5, 27 },   
    {  1379,    3, 22 },   
    {  1381,    2, 10 },   
    {  1384,    4, 28 },   
    {  1384,    2, 27 },   
    {  1387,    8, 23 },   
    {  1389,    2,  9 },   
    {  1390,    3, 26 },   
    {  1394,    7,  5 },   
    {  1428,    4, 27 },   
    {  1429,    9,  5 },   
    {  1441,    2, 17 },   
    {  1444,    2,  5 },   
    {  1449,    7, 28 },   
    {  1452,    7, 25 },   
    {  1455,    7, 25 },   
    {  1457,    9, 28 },   
    {  1460,   12, 21 },   
    {  1466,    2, 28 },   
    {  1467,    3,  3 },   
    {  1469,    4, 28 },   
    {  1487,    7, 29 },   
    {  1489,    8, 21 },   
    {  1492,    7, 19 },   
    {  1501,    2, 29 },   
    {  1504,    2, 30 },   
    {  1521,    8, 23 },   
    {  1528,    8, 20 },   
    {  1532,    7, 29 },   
    {  1555,   10, 23 },   
    {  1558,    2, 28 },   
    {  1570,    4, 23 },   
    {  1573,    7, 28 },   
    {  1592,   12,  8 },   
    {  1596,   10, 27 },   
    {  1615,    7, 13 },   
    {  1624,    2, 30 },   
    {  1644,   12, 16 },   
    {  1648,    2, 15 },   
    {  1652,    9, 18 },   
    {  1655,    4, 13 },   
    {  1658,    7, 23 },   
    {  1661,    4, 25 },   
    {  1673,    9, 21 },   
    {  1681,    9, 29 },   
    {  1684,    2, 21 },   
    {  1688,    9, 30 },   
    {  1704,    3, 13 },   
    {  1711,    4, 25 },   
    {  1716,    6, 22 },   
    {  1736,    4, 28 },   
    {  1741,    2, 27 },   
    {  1744,    2, 21 },   
    {  1748,    7, 12 },   
    {  1751,   10, 27 },   
    {  1764,    6,  2 },   
    {  1772,   11, 16 },   
    {  1781,    4,  2 },   
    {  1789,    1, 25 },   
    {  1801,    2,  5 },   
    {  1804,    2, 11 },   
    {  1818,    4, 22 },   
    {  1830,   12, 10 },   
    {  1844,   12,  2 },   
    {  1848,    2, 28 },   
    {  1854,   11, 27 },   
    {  1860,    3, 18 },   
    {  1861,    2, 19 },   
    {  1864,    2, 20 },   
    {  1865,    4,  7 },   
    {  1868,    9,  8 },   
    {  1912,    7, 30 },   
    {  1926,   12, 25 },   
    {  1989,    1,  8 }   
};

#define kEraCount (sizeof(kEraInfo)/sizeof(kEraInfo[0]))




static const int32_t kCurrentEra = (kEraCount-1);  

static const int32_t kGregorianEpoch = 1970;    


uint32_t JapaneseCalendar::getCurrentEra() {
    return kCurrentEra;
}

JapaneseCalendar::JapaneseCalendar(const Locale& aLocale, UErrorCode& success)
:   GregorianCalendar(aLocale, success)
{
    setTimeInMillis(getNow(), success); 
}

JapaneseCalendar::~JapaneseCalendar()
{
}

JapaneseCalendar::JapaneseCalendar(const JapaneseCalendar& source)
: GregorianCalendar(source)
{
}

JapaneseCalendar& JapaneseCalendar::operator= ( const JapaneseCalendar& right)
{
    GregorianCalendar::operator=(right);
    return *this;
}

Calendar* JapaneseCalendar::clone(void) const
{
    return new JapaneseCalendar(*this);
}

const char *JapaneseCalendar::getType() const
{
    return "japanese";
}

int32_t JapaneseCalendar::getDefaultMonthInYear(int32_t eyear) 
{
    int32_t era = internalGetEra();
    

    int32_t month = 0;

    

    if(eyear == kEraInfo[era].year) {
        
        return kEraInfo[era].month-1;
    }

    return month;
}

int32_t JapaneseCalendar::getDefaultDayInMonth(int32_t eyear, int32_t month) 
{
    int32_t era = internalGetEra();
    int32_t day = 1;

    if(eyear == kEraInfo[era].year) {
        if(month == (kEraInfo[era].month-1)) {
            return kEraInfo[era].day;
        }
    }

    return day;
}


int32_t JapaneseCalendar::internalGetEra() const
{
    return internalGet(UCAL_ERA, kCurrentEra);
}

int32_t JapaneseCalendar::handleGetExtendedYear()
{
    
    
    int32_t year;

    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR &&
        newerField(UCAL_EXTENDED_YEAR, UCAL_ERA) == UCAL_EXTENDED_YEAR) {
            year = internalGet(UCAL_EXTENDED_YEAR, kGregorianEpoch);
        } else {
            
            year = internalGet(UCAL_YEAR) + kEraInfo[internalGetEra()].year - 1;
        }
        return year;
}


void JapaneseCalendar::handleComputeFields(int32_t julianDay, UErrorCode& status)
{
    
    GregorianCalendar::handleComputeFields(julianDay, status);
    int32_t year = internalGet(UCAL_EXTENDED_YEAR); 

    int32_t low = 0;

    
    
    
    
#ifdef U_DEBUG_JCAL
    fprintf(stderr, "==  %d \n", year);
#endif

    if (year > kEraInfo[kCurrentEra].year) {
        low = kCurrentEra;
#ifdef U_DEBUG_JCAL
        fprintf(stderr, " low=%d (special)\n", low);
#endif
    } else {
        
        int32_t high = kEraCount;

#ifdef U_DEBUG_JCAL
        fprintf(stderr, " high=%d\n", high);
#endif
        while (low < high - 1) {
            int32_t i = (low + high) / 2;
            int32_t diff = year - kEraInfo[i].year;

#ifdef U_DEBUG_JCAL
            fprintf(stderr, "  d=%d   low=%d, high=%d. Considering %d:M%d D%d Y%d. { we are ?:M%d D%d Y%d }\n",
                diff,low, high, i, kEraInfo[i].month-1, kEraInfo[i].day,  kEraInfo[i].year, internalGet(UCAL_MONTH), internalGet(UCAL_DATE),year);
#endif

            
            
            
            if (diff == 0) {
                diff = internalGet(UCAL_MONTH) - (kEraInfo[i].month - 1);
#ifdef U_DEBUG_JCAL
                fprintf(stderr, "diff now %d (M)  = %d - %d - 1\n", diff, internalGet(UCAL_MONTH), kEraInfo[i].month);
#endif
                if (diff == 0) {
                    diff = internalGet(UCAL_DATE) - kEraInfo[i].day;
#ifdef U_DEBUG_JCAL
                    fprintf(stderr, "diff now %d (D)\n", diff);
#endif
                }
            }
            if (diff >= 0) {
                low = i;
            } else {
                high = i;
            }
#ifdef U_DEBUG_JCAL
            fprintf(stderr, ". low=%d, high=%d, i=%d, diff=%d.. %d\n", low, high, i, diff, year);
#endif

        }
    }

#ifdef U_DEBUG_JCAL
    fprintf(stderr, "  low[era]=%d,.. %d\n", low, year);
#endif
    
    
    
    

    internalSet(UCAL_ERA, low);
    internalSet(UCAL_YEAR, year - kEraInfo[low].year + 1);
#ifdef U_DEBUG_JCAL
    fprintf(stderr, "  Set ERA=%d, year=%d\n", low, year-kEraInfo[low].year+1);
#endif

}




UBool JapaneseCalendar::haveDefaultCentury() const
{
    return FALSE;
}

UDate JapaneseCalendar::defaultCenturyStart() const
{
    return 0;
}

int32_t JapaneseCalendar::defaultCenturyStartYear() const
{
    return 0;
}

int32_t JapaneseCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    switch(field) {
    case UCAL_ERA:
        if (limitType == UCAL_LIMIT_MINIMUM || limitType == UCAL_LIMIT_GREATEST_MINIMUM) {
            return 0;
        }
        return kCurrentEra;
    case UCAL_YEAR:
        {
            switch (limitType) {
            case UCAL_LIMIT_MINIMUM:
            case UCAL_LIMIT_GREATEST_MINIMUM:
                return 1;
            case UCAL_LIMIT_LEAST_MAXIMUM:
                return 1;
            case  UCAL_LIMIT_COUNT: 
            case UCAL_LIMIT_MAXIMUM:
                return GregorianCalendar::handleGetLimit(UCAL_YEAR, UCAL_LIMIT_MAXIMUM) - kEraInfo[kCurrentEra].year;
            default:
                return 1;    
            }
        }
    default:
        return GregorianCalendar::handleGetLimit(field,limitType);
    }
}

int32_t JapaneseCalendar::getActualMaximum(UCalendarDateFields field, UErrorCode& status) const {
    if (field == UCAL_YEAR) {
        int32_t era = get(UCAL_ERA, status);
        if (U_FAILURE(status)) {
            return 0; 
        }
        if (era == kCurrentEra) {
            
            return handleGetLimit(UCAL_YEAR, UCAL_LIMIT_MAXIMUM);
        } else {
            int32_t nextEraYear = kEraInfo[era + 1].year;
            int32_t nextEraMonth = kEraInfo[era + 1].month;
            int32_t nextEraDate = kEraInfo[era + 1].day;

            int32_t maxYear = nextEraYear - kEraInfo[era].year + 1; 
            if (nextEraMonth == 1 && nextEraDate == 1) {
                
                maxYear--;
            }
            return maxYear;
        }
    }
    return GregorianCalendar::getActualMaximum(field, status);
}

U_NAMESPACE_END

#endif
