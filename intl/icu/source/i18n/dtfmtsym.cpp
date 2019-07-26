




















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/ustring.h"
#include "unicode/dtfmtsym.h"
#include "unicode/smpdtfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/tznames.h"
#include "cpputils.h"
#include "ucln_in.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "locbased.h"
#include "gregoimp.h"
#include "hash.h"
#include "uresimp.h"
#include "ureslocs.h"










#define PATTERN_CHARS_LEN 31





static const UChar gPatternChars[] = {
    
    0x47, 0x79, 0x4D, 0x64, 0x6B, 0x48, 0x6D, 0x73, 0x53, 0x45,
    0x44, 0x46, 0x77, 0x57, 0x61, 0x68, 0x4B, 0x7A, 0x59, 0x65,
    0x75, 0x67, 0x41, 0x5A, 0x76, 0x63, 0x4c, 0x51, 0x71, 0x56, 0x55, 0
};


#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))






static const UChar gLastResortMonthNames[13][3] =
{
    {0x0030, 0x0031, 0x0000}, 
    {0x0030, 0x0032, 0x0000}, 
    {0x0030, 0x0033, 0x0000}, 
    {0x0030, 0x0034, 0x0000}, 
    {0x0030, 0x0035, 0x0000}, 
    {0x0030, 0x0036, 0x0000}, 
    {0x0030, 0x0037, 0x0000}, 
    {0x0030, 0x0038, 0x0000}, 
    {0x0030, 0x0039, 0x0000}, 
    {0x0031, 0x0030, 0x0000}, 
    {0x0031, 0x0031, 0x0000}, 
    {0x0031, 0x0032, 0x0000}, 
    {0x0031, 0x0033, 0x0000}  
};


static const UChar gLastResortDayNames[8][2] =
{
    {0x0030, 0x0000}, 
    {0x0031, 0x0000}, 
    {0x0032, 0x0000}, 
    {0x0033, 0x0000}, 
    {0x0034, 0x0000}, 
    {0x0035, 0x0000}, 
    {0x0036, 0x0000}, 
    {0x0037, 0x0000}  
};


static const UChar gLastResortQuarters[4][2] =
{
    {0x0031, 0x0000}, 
    {0x0032, 0x0000}, 
    {0x0033, 0x0000}, 
    {0x0034, 0x0000}, 
};


static const UChar gLastResortAmPmMarkers[2][3] =
{
    {0x0041, 0x004D, 0x0000}, 
    {0x0050, 0x004D, 0x0000}  
};

static const UChar gLastResortEras[2][3] =
{
    {0x0042, 0x0043, 0x0000}, 
    {0x0041, 0x0044, 0x0000}  
};


typedef enum LastResortSize {
    kMonthNum = 13,
    kMonthLen = 3,

    kDayNum = 8,
    kDayLen = 2,

    kAmPmNum = 2,
    kAmPmLen = 3,

    kQuarterNum = 4,
    kQuarterLen = 2,

    kEraNum = 2,
    kEraLen = 3,

    kZoneNum = 5,
    kZoneLen = 4,

    kGmtHourNum = 4,
    kGmtHourLen = 10
} LastResortSize;

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateFormatSymbols)

#define kSUPPLEMENTAL "supplementalData"





static const char gErasTag[]="eras";
static const char gCyclicNameSetsTag[]="cyclicNameSets";
static const char gNameSetYearsTag[]="years";
static const char gMonthNamesTag[]="monthNames";
static const char gMonthPatternsTag[]="monthPatterns";
static const char gDayNamesTag[]="dayNames";
static const char gNamesWideTag[]="wide";
static const char gNamesAbbrTag[]="abbreviated";
static const char gNamesNarrowTag[]="narrow";
static const char gNamesAllTag[]="all";
static const char gNamesLeapTag[]="leap";
static const char gNamesFormatTag[]="format";
static const char gNamesStandaloneTag[]="stand-alone";
static const char gNamesNumericTag[]="numeric";
static const char gAmPmMarkersTag[]="AmPmMarkers";
static const char gQuartersTag[]="quarters";





static const char gContextTransformsTag[]="contextTransforms";

static UMutex LOCK = U_MUTEX_INITIALIZER;





static inline UnicodeString* newUnicodeStringArray(size_t count) {
    return new UnicodeString[count ? count : 1];
}



DateFormatSymbols::DateFormatSymbols(const Locale& locale,
                                     UErrorCode& status)
    : UObject()
{
  initializeData(locale, NULL,  status);
}

DateFormatSymbols::DateFormatSymbols(UErrorCode& status)
    : UObject()
{
  initializeData(Locale::getDefault(), NULL, status, TRUE);
}


DateFormatSymbols::DateFormatSymbols(const Locale& locale,
                                     const char *type,
                                     UErrorCode& status)
    : UObject()
{
  initializeData(locale, type,  status);
}

DateFormatSymbols::DateFormatSymbols(const char *type, UErrorCode& status)
    : UObject()
{
  initializeData(Locale::getDefault(), type, status, TRUE);
}

DateFormatSymbols::DateFormatSymbols(const DateFormatSymbols& other)
    : UObject(other)
{
    copyData(other);
}

void
DateFormatSymbols::assignArray(UnicodeString*& dstArray,
                               int32_t& dstCount,
                               const UnicodeString* srcArray,
                               int32_t srcCount)
{
    
    
    
    
    
    
    
    
    
    
    
    
    dstCount = srcCount;
    dstArray = newUnicodeStringArray(srcCount);
    if(dstArray != NULL) {
        int32_t i;
        for(i=0; i<srcCount; ++i) {
            dstArray[i].fastCopyFrom(srcArray[i]);
        }
    }
}






void
DateFormatSymbols::createZoneStrings(const UnicodeString *const * otherStrings)
{
    int32_t row, col;
    UBool failed = FALSE;

    fZoneStrings = (UnicodeString **)uprv_malloc(fZoneStringsRowCount * sizeof(UnicodeString *));
    if (fZoneStrings != NULL) {
        for (row=0; row<fZoneStringsRowCount; ++row)
        {
            fZoneStrings[row] = newUnicodeStringArray(fZoneStringsColCount);
            if (fZoneStrings[row] == NULL) {
                failed = TRUE;
                break;
            }
            for (col=0; col<fZoneStringsColCount; ++col) {
                
                fZoneStrings[row][col].fastCopyFrom(otherStrings[row][col]);
            }
        }
    }
    
    if (failed) {
        for (int i = row; i >= 0; i--) {
            delete[] fZoneStrings[i];
        }
        uprv_free(fZoneStrings);
        fZoneStrings = NULL;
    }
}




void
DateFormatSymbols::copyData(const DateFormatSymbols& other) {
    assignArray(fEras, fErasCount, other.fEras, other.fErasCount);
    assignArray(fEraNames, fEraNamesCount, other.fEraNames, other.fEraNamesCount);
    assignArray(fNarrowEras, fNarrowErasCount, other.fNarrowEras, other.fNarrowErasCount);
    assignArray(fMonths, fMonthsCount, other.fMonths, other.fMonthsCount);
    assignArray(fShortMonths, fShortMonthsCount, other.fShortMonths, other.fShortMonthsCount);
    assignArray(fNarrowMonths, fNarrowMonthsCount, other.fNarrowMonths, other.fNarrowMonthsCount);
    assignArray(fStandaloneMonths, fStandaloneMonthsCount, other.fStandaloneMonths, other.fStandaloneMonthsCount);
    assignArray(fStandaloneShortMonths, fStandaloneShortMonthsCount, other.fStandaloneShortMonths, other.fStandaloneShortMonthsCount);
    assignArray(fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, other.fStandaloneNarrowMonths, other.fStandaloneNarrowMonthsCount);
    assignArray(fWeekdays, fWeekdaysCount, other.fWeekdays, other.fWeekdaysCount);
    assignArray(fShortWeekdays, fShortWeekdaysCount, other.fShortWeekdays, other.fShortWeekdaysCount);
    assignArray(fNarrowWeekdays, fNarrowWeekdaysCount, other.fNarrowWeekdays, other.fNarrowWeekdaysCount);
    assignArray(fStandaloneWeekdays, fStandaloneWeekdaysCount, other.fStandaloneWeekdays, other.fStandaloneWeekdaysCount);
    assignArray(fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount, other.fStandaloneShortWeekdays, other.fStandaloneShortWeekdaysCount);
    assignArray(fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount, other.fStandaloneNarrowWeekdays, other.fStandaloneNarrowWeekdaysCount);
    assignArray(fAmPms, fAmPmsCount, other.fAmPms, other.fAmPmsCount);
    assignArray(fQuarters, fQuartersCount, other.fQuarters, other.fQuartersCount);
    assignArray(fShortQuarters, fShortQuartersCount, other.fShortQuarters, other.fShortQuartersCount);
    assignArray(fStandaloneQuarters, fStandaloneQuartersCount, other.fStandaloneQuarters, other.fStandaloneQuartersCount);
    assignArray(fStandaloneShortQuarters, fStandaloneShortQuartersCount, other.fStandaloneShortQuarters, other.fStandaloneShortQuartersCount);
    if (other.fLeapMonthPatterns != NULL) {
        assignArray(fLeapMonthPatterns, fLeapMonthPatternsCount, other.fLeapMonthPatterns, other.fLeapMonthPatternsCount);
    } else {
        fLeapMonthPatterns = NULL;
        fLeapMonthPatternsCount = 0;
    }
    if (other.fShortYearNames != NULL) {
        assignArray(fShortYearNames, fShortYearNamesCount, other.fShortYearNames, other.fShortYearNamesCount);
    } else {
        fShortYearNames = NULL;
        fShortYearNamesCount = 0;
    }
 
    if (other.fZoneStrings != NULL) {
        fZoneStringsColCount = other.fZoneStringsColCount;
        fZoneStringsRowCount = other.fZoneStringsRowCount;
        createZoneStrings((const UnicodeString**)other.fZoneStrings);

    } else {
        fZoneStrings = NULL;
        fZoneStringsColCount = 0;
        fZoneStringsRowCount = 0;
    }
    fZSFLocale = other.fZSFLocale;
    
    fLocaleZoneStrings = NULL;

    
    fLocalPatternChars.fastCopyFrom(other.fLocalPatternChars);
    
    uprv_memcpy(fCapitalization, other.fCapitalization, sizeof(fCapitalization));
}




DateFormatSymbols& DateFormatSymbols::operator=(const DateFormatSymbols& other)
{
    dispose();
    copyData(other);

    return *this;
}

DateFormatSymbols::~DateFormatSymbols()
{
    dispose();
}

void DateFormatSymbols::dispose()
{
    if (fEras)                     delete[] fEras;
    if (fEraNames)                 delete[] fEraNames;
    if (fNarrowEras)               delete[] fNarrowEras;
    if (fMonths)                   delete[] fMonths;
    if (fShortMonths)              delete[] fShortMonths;
    if (fNarrowMonths)             delete[] fNarrowMonths;
    if (fStandaloneMonths)         delete[] fStandaloneMonths;
    if (fStandaloneShortMonths)    delete[] fStandaloneShortMonths;
    if (fStandaloneNarrowMonths)   delete[] fStandaloneNarrowMonths;
    if (fWeekdays)                 delete[] fWeekdays;
    if (fShortWeekdays)            delete[] fShortWeekdays;
    if (fNarrowWeekdays)           delete[] fNarrowWeekdays;
    if (fStandaloneWeekdays)       delete[] fStandaloneWeekdays;
    if (fStandaloneShortWeekdays)  delete[] fStandaloneShortWeekdays;
    if (fStandaloneNarrowWeekdays) delete[] fStandaloneNarrowWeekdays;
    if (fAmPms)                    delete[] fAmPms;
    if (fQuarters)                 delete[] fQuarters;
    if (fShortQuarters)            delete[] fShortQuarters;
    if (fStandaloneQuarters)       delete[] fStandaloneQuarters;
    if (fStandaloneShortQuarters)  delete[] fStandaloneShortQuarters;
    if (fLeapMonthPatterns)        delete[] fLeapMonthPatterns;
    if (fShortYearNames)           delete[] fShortYearNames;

    disposeZoneStrings();
}

void DateFormatSymbols::disposeZoneStrings()
{
    if (fZoneStrings) {
        for (int32_t row = 0; row < fZoneStringsRowCount; ++row) {
            delete[] fZoneStrings[row];
        }
        uprv_free(fZoneStrings);
    }
    if (fLocaleZoneStrings) {
        for (int32_t row = 0; row < fZoneStringsRowCount; ++row) {
            delete[] fLocaleZoneStrings[row];
        }
        uprv_free(fLocaleZoneStrings);
    }

    fZoneStrings = NULL;
    fLocaleZoneStrings = NULL;
    fZoneStringsRowCount = 0;
    fZoneStringsColCount = 0;
}

UBool
DateFormatSymbols::arrayCompare(const UnicodeString* array1,
                                const UnicodeString* array2,
                                int32_t count)
{
    if (array1 == array2) return TRUE;
    while (count>0)
    {
        --count;
        if (array1[count] != array2[count]) return FALSE;
    }
    return TRUE;
}

UBool
DateFormatSymbols::operator==(const DateFormatSymbols& other) const
{
    
    if (this == &other) {
        return TRUE;
    }
    if (fErasCount == other.fErasCount &&
        fEraNamesCount == other.fEraNamesCount &&
        fNarrowErasCount == other.fNarrowErasCount &&
        fMonthsCount == other.fMonthsCount &&
        fShortMonthsCount == other.fShortMonthsCount &&
        fNarrowMonthsCount == other.fNarrowMonthsCount &&
        fStandaloneMonthsCount == other.fStandaloneMonthsCount &&
        fStandaloneShortMonthsCount == other.fStandaloneShortMonthsCount &&
        fStandaloneNarrowMonthsCount == other.fStandaloneNarrowMonthsCount &&
        fWeekdaysCount == other.fWeekdaysCount &&
        fShortWeekdaysCount == other.fShortWeekdaysCount &&
        fNarrowWeekdaysCount == other.fNarrowWeekdaysCount &&
        fStandaloneWeekdaysCount == other.fStandaloneWeekdaysCount &&
        fStandaloneShortWeekdaysCount == other.fStandaloneShortWeekdaysCount &&
        fStandaloneNarrowWeekdaysCount == other.fStandaloneNarrowWeekdaysCount &&
        fAmPmsCount == other.fAmPmsCount &&
        fQuartersCount == other.fQuartersCount &&
        fShortQuartersCount == other.fShortQuartersCount &&
        fStandaloneQuartersCount == other.fStandaloneQuartersCount &&
        fStandaloneShortQuartersCount == other.fStandaloneShortQuartersCount &&
        fLeapMonthPatternsCount == other.fLeapMonthPatternsCount &&
        fShortYearNamesCount == other.fShortYearNamesCount &&
        (uprv_memcmp(fCapitalization, other.fCapitalization, sizeof(fCapitalization))==0))
    {
        
        if (arrayCompare(fEras, other.fEras, fErasCount) &&
            arrayCompare(fEraNames, other.fEraNames, fEraNamesCount) &&
            arrayCompare(fNarrowEras, other.fNarrowEras, fNarrowErasCount) &&
            arrayCompare(fMonths, other.fMonths, fMonthsCount) &&
            arrayCompare(fShortMonths, other.fShortMonths, fShortMonthsCount) &&
            arrayCompare(fNarrowMonths, other.fNarrowMonths, fNarrowMonthsCount) &&
            arrayCompare(fStandaloneMonths, other.fStandaloneMonths, fStandaloneMonthsCount) &&
            arrayCompare(fStandaloneShortMonths, other.fStandaloneShortMonths, fStandaloneShortMonthsCount) &&
            arrayCompare(fStandaloneNarrowMonths, other.fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount) &&
            arrayCompare(fWeekdays, other.fWeekdays, fWeekdaysCount) &&
            arrayCompare(fShortWeekdays, other.fShortWeekdays, fShortWeekdaysCount) &&
            arrayCompare(fNarrowWeekdays, other.fNarrowWeekdays, fNarrowWeekdaysCount) &&
            arrayCompare(fStandaloneWeekdays, other.fStandaloneWeekdays, fStandaloneWeekdaysCount) &&
            arrayCompare(fStandaloneShortWeekdays, other.fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount) &&
            arrayCompare(fStandaloneNarrowWeekdays, other.fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount) &&
            arrayCompare(fAmPms, other.fAmPms, fAmPmsCount) &&
            arrayCompare(fQuarters, other.fQuarters, fQuartersCount) &&
            arrayCompare(fShortQuarters, other.fShortQuarters, fShortQuartersCount) &&
            arrayCompare(fStandaloneQuarters, other.fStandaloneQuarters, fStandaloneQuartersCount) &&
            arrayCompare(fStandaloneShortQuarters, other.fStandaloneShortQuarters, fStandaloneShortQuartersCount) &&
            arrayCompare(fLeapMonthPatterns, other.fLeapMonthPatterns, fLeapMonthPatternsCount) &&
            arrayCompare(fShortYearNames, other.fShortYearNames, fShortYearNamesCount))
        {
            
            if (fZoneStrings == NULL && other.fZoneStrings == NULL) {
                if (fZSFLocale == other.fZSFLocale) {
                    return TRUE;
                }
            } else if (fZoneStrings != NULL && other.fZoneStrings != NULL) {
                if (fZoneStringsRowCount == other.fZoneStringsRowCount
                    && fZoneStringsColCount == other.fZoneStringsColCount) {
                    UBool cmpres = TRUE;
                    for (int32_t i = 0; (i < fZoneStringsRowCount) && cmpres; i++) {
                        cmpres = arrayCompare(fZoneStrings[i], other.fZoneStrings[i], fZoneStringsColCount);
                    }
                    return cmpres;
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}



const UnicodeString*
DateFormatSymbols::getEras(int32_t &count) const
{
    count = fErasCount;
    return fEras;
}

const UnicodeString*
DateFormatSymbols::getEraNames(int32_t &count) const
{
    count = fEraNamesCount;
    return fEraNames;
}

const UnicodeString*
DateFormatSymbols::getNarrowEras(int32_t &count) const
{
    count = fNarrowErasCount;
    return fNarrowEras;
}

const UnicodeString*
DateFormatSymbols::getMonths(int32_t &count) const
{
    count = fMonthsCount;
    return fMonths;
}

const UnicodeString*
DateFormatSymbols::getShortMonths(int32_t &count) const
{
    count = fShortMonthsCount;
    return fShortMonths;
}

const UnicodeString*
DateFormatSymbols::getMonths(int32_t &count, DtContextType context, DtWidthType width ) const
{
    UnicodeString *returnValue = NULL;

    switch (context) {
    case FORMAT :
        switch(width) {
        case WIDE :
            count = fMonthsCount;
            returnValue = fMonths;
            break;
        case ABBREVIATED :
            count = fShortMonthsCount;
            returnValue = fShortMonths;
            break;
        case NARROW :
            count = fNarrowMonthsCount;
            returnValue = fNarrowMonths;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch(width) {
        case WIDE :
            count = fStandaloneMonthsCount;
            returnValue = fStandaloneMonths;
            break;
        case ABBREVIATED :
            count = fStandaloneShortMonthsCount;
            returnValue = fStandaloneShortMonths;
            break;
        case NARROW :
            count = fStandaloneNarrowMonthsCount;
            returnValue = fStandaloneNarrowMonths;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getWeekdays(int32_t &count) const
{
    count = fWeekdaysCount;
    return fWeekdays;
}

const UnicodeString*
DateFormatSymbols::getShortWeekdays(int32_t &count) const
{
    count = fShortWeekdaysCount;
    return fShortWeekdays;
}

const UnicodeString*
DateFormatSymbols::getWeekdays(int32_t &count, DtContextType context, DtWidthType width) const
{
    UnicodeString *returnValue = NULL;
    switch (context) {
    case FORMAT :
        switch(width) {
            case WIDE :
                count = fWeekdaysCount;
                returnValue = fWeekdays;
                break;
            case ABBREVIATED :
                count = fShortWeekdaysCount;
                returnValue = fShortWeekdays;
                break;
            case NARROW :
                count = fNarrowWeekdaysCount;
                returnValue = fNarrowWeekdays;
                break;
            case DT_WIDTH_COUNT :
                break;
        }
        break;
    case STANDALONE :
        switch(width) {
            case WIDE :
                count = fStandaloneWeekdaysCount;
                returnValue = fStandaloneWeekdays;
                break;
            case ABBREVIATED :
                count = fStandaloneShortWeekdaysCount;
                returnValue = fStandaloneShortWeekdays;
                break;
            case NARROW :
                count = fStandaloneNarrowWeekdaysCount;
                returnValue = fStandaloneNarrowWeekdays;
                break;
            case DT_WIDTH_COUNT :
                break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getQuarters(int32_t &count, DtContextType context, DtWidthType width ) const
{
    UnicodeString *returnValue = NULL;

    switch (context) {
    case FORMAT :
        switch(width) {
        case WIDE :
            count = fQuartersCount;
            returnValue = fQuarters;
            break;
        case ABBREVIATED :
            count = fShortQuartersCount;
            returnValue = fShortQuarters;
            break;
        case NARROW :
            count = 0;
            returnValue = NULL;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch(width) {
        case WIDE :
            count = fStandaloneQuartersCount;
            returnValue = fStandaloneQuarters;
            break;
        case ABBREVIATED :
            count = fStandaloneShortQuartersCount;
            returnValue = fStandaloneShortQuarters;
            break;
        case NARROW :
            count = 0;
            returnValue = NULL;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getAmPmStrings(int32_t &count) const
{
    count = fAmPmsCount;
    return fAmPms;
}

const UnicodeString*
DateFormatSymbols::getLeapMonthPatterns(int32_t &count) const
{
    count = fLeapMonthPatternsCount;
    return fLeapMonthPatterns;
}



void
DateFormatSymbols::setEras(const UnicodeString* erasArray, int32_t count)
{
    
    if (fEras)
        delete[] fEras;

    
    
    fEras = newUnicodeStringArray(count);
    uprv_arrayCopy(erasArray,fEras,  count);
    fErasCount = count;
}

void
DateFormatSymbols::setEraNames(const UnicodeString* eraNamesArray, int32_t count)
{
    
    if (fEraNames)
        delete[] fEraNames;

    
    
    fEraNames = newUnicodeStringArray(count);
    uprv_arrayCopy(eraNamesArray,fEraNames,  count);
    fEraNamesCount = count;
}

void
DateFormatSymbols::setNarrowEras(const UnicodeString* narrowErasArray, int32_t count)
{
    
    if (fNarrowEras)
        delete[] fNarrowEras;

    
    
    fNarrowEras = newUnicodeStringArray(count);
    uprv_arrayCopy(narrowErasArray,fNarrowEras,  count);
    fNarrowErasCount = count;
}

void
DateFormatSymbols::setMonths(const UnicodeString* monthsArray, int32_t count)
{
    
    if (fMonths)
        delete[] fMonths;

    
    
    fMonths = newUnicodeStringArray(count);
    uprv_arrayCopy( monthsArray,fMonths,count);
    fMonthsCount = count;
}

void
DateFormatSymbols::setShortMonths(const UnicodeString* shortMonthsArray, int32_t count)
{
    
    if (fShortMonths)
        delete[] fShortMonths;

    
    
    fShortMonths = newUnicodeStringArray(count);
    uprv_arrayCopy(shortMonthsArray,fShortMonths,  count);
    fShortMonthsCount = count;
}

void
DateFormatSymbols::setMonths(const UnicodeString* monthsArray, int32_t count, DtContextType context, DtWidthType width)
{
    
    
    

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fMonths)
                delete[] fMonths;
            fMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fMonths,count);
            fMonthsCount = count;
            break;
        case ABBREVIATED :
            if (fShortMonths)
                delete[] fShortMonths;
            fShortMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fShortMonths,count);
            fShortMonthsCount = count;
            break;
        case NARROW :
            if (fNarrowMonths)
                delete[] fNarrowMonths;
            fNarrowMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fNarrowMonths,count);
            fNarrowMonthsCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneMonths)
                delete[] fStandaloneMonths;
            fStandaloneMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneMonths,count);
            fStandaloneMonthsCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortMonths)
                delete[] fStandaloneShortMonths;
            fStandaloneShortMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneShortMonths,count);
            fStandaloneShortMonthsCount = count;
            break;
        case NARROW :
           if (fStandaloneNarrowMonths)
                delete[] fStandaloneNarrowMonths;
            fStandaloneNarrowMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneNarrowMonths,count);
            fStandaloneNarrowMonthsCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void DateFormatSymbols::setWeekdays(const UnicodeString* weekdaysArray, int32_t count)
{
    
    if (fWeekdays)
        delete[] fWeekdays;

    
    
    fWeekdays = newUnicodeStringArray(count);
    uprv_arrayCopy(weekdaysArray,fWeekdays,count);
    fWeekdaysCount = count;
}

void
DateFormatSymbols::setShortWeekdays(const UnicodeString* shortWeekdaysArray, int32_t count)
{
    
    if (fShortWeekdays)
        delete[] fShortWeekdays;

    
    
    fShortWeekdays = newUnicodeStringArray(count);
    uprv_arrayCopy(shortWeekdaysArray, fShortWeekdays, count);
    fShortWeekdaysCount = count;
}

void
DateFormatSymbols::setWeekdays(const UnicodeString* weekdaysArray, int32_t count, DtContextType context, DtWidthType width)
{
    
    
    

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fWeekdays)
                delete[] fWeekdays;
            fWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fWeekdays, count);
            fWeekdaysCount = count;
            break;
        case ABBREVIATED :
            if (fShortWeekdays)
                delete[] fShortWeekdays;
            fShortWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fShortWeekdays, count);
            fShortWeekdaysCount = count;
            break;
        case NARROW :
            if (fNarrowWeekdays)
                delete[] fNarrowWeekdays;
            fNarrowWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fNarrowWeekdays, count);
            fNarrowWeekdaysCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneWeekdays)
                delete[] fStandaloneWeekdays;
            fStandaloneWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneWeekdays, count);
            fStandaloneWeekdaysCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortWeekdays)
                delete[] fStandaloneShortWeekdays;
            fStandaloneShortWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneShortWeekdays, count);
            fStandaloneShortWeekdaysCount = count;
            break;
        case NARROW :
            if (fStandaloneNarrowWeekdays)
                delete[] fStandaloneNarrowWeekdays;
            fStandaloneNarrowWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneNarrowWeekdays, count);
            fStandaloneNarrowWeekdaysCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void
DateFormatSymbols::setQuarters(const UnicodeString* quartersArray, int32_t count, DtContextType context, DtWidthType width)
{
    
    
    

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fQuarters)
                delete[] fQuarters;
            fQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fQuarters,count);
            fQuartersCount = count;
            break;
        case ABBREVIATED :
            if (fShortQuarters)
                delete[] fShortQuarters;
            fShortQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fShortQuarters,count);
            fShortQuartersCount = count;
            break;
        case NARROW :
        






            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneQuarters)
                delete[] fStandaloneQuarters;
            fStandaloneQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fStandaloneQuarters,count);
            fStandaloneQuartersCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortQuarters)
                delete[] fStandaloneShortQuarters;
            fStandaloneShortQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fStandaloneShortQuarters,count);
            fStandaloneShortQuartersCount = count;
            break;
        case NARROW :
        






            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void
DateFormatSymbols::setAmPmStrings(const UnicodeString* amPmsArray, int32_t count)
{
    
    if (fAmPms) delete[] fAmPms;

    
    
    fAmPms = newUnicodeStringArray(count);
    uprv_arrayCopy(amPmsArray,fAmPms,count);
    fAmPmsCount = count;
}

const UnicodeString**
DateFormatSymbols::getZoneStrings(int32_t& rowCount, int32_t& columnCount) const
{
    const UnicodeString **result = NULL;

    umtx_lock(&LOCK);
    if (fZoneStrings == NULL) {
        if (fLocaleZoneStrings == NULL) {
            ((DateFormatSymbols*)this)->initZoneStringsArray();
        }
        result = (const UnicodeString**)fLocaleZoneStrings;
    } else {
        result = (const UnicodeString**)fZoneStrings;
    }
    rowCount = fZoneStringsRowCount;
    columnCount = fZoneStringsColCount;
    umtx_unlock(&LOCK);

    return result;
}


#define ZONE_SET UCAL_ZONE_TYPE_ANY


void
DateFormatSymbols::initZoneStringsArray(void) {
    if (fZoneStrings != NULL || fLocaleZoneStrings != NULL) {
        return;
    }

    UErrorCode status = U_ZERO_ERROR;

    StringEnumeration *tzids = NULL;
    UnicodeString ** zarray = NULL;
    TimeZoneNames *tzNames = NULL;
    int32_t rows = 0;

    do { 

        tzids = TimeZone::createTimeZoneIDEnumeration(ZONE_SET, NULL, NULL, status);
        rows = tzids->count(status);
        if (U_FAILURE(status)) {
            break;
        }

        
        int32_t size = rows * sizeof(UnicodeString*);
        zarray = (UnicodeString**)uprv_malloc(size);
        if (zarray == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            break;
        }
        uprv_memset(zarray, 0, size);

        tzNames = TimeZoneNames::createInstance(fZSFLocale, status);

        const UnicodeString *tzid;
        int32_t i = 0;
        UDate now = Calendar::getNow();
        UnicodeString tzDispName;

        while ((tzid = tzids->snext(status))) {
            if (U_FAILURE(status)) {
                break;
            }

            zarray[i] = new UnicodeString[5];
            if (zarray[i] == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }

            zarray[i][0].setTo(*tzid);
            zarray[i][1].setTo(tzNames->getDisplayName(*tzid, UTZNM_LONG_STANDARD, now, tzDispName));
            zarray[i][2].setTo(tzNames->getDisplayName(*tzid, UTZNM_SHORT_STANDARD, now, tzDispName));
            zarray[i][3].setTo(tzNames->getDisplayName(*tzid, UTZNM_LONG_DAYLIGHT, now, tzDispName));
            zarray[i][4].setTo(tzNames->getDisplayName(*tzid, UTZNM_SHORT_DAYLIGHT, now, tzDispName));
            i++;
        }

    } while (FALSE);

    if (U_FAILURE(status)) {
        if (zarray) {
            for (int32_t i = 0; i < rows; i++) {
                if (zarray[i]) {
                    delete[] zarray[i];
                }
            }
            uprv_free(zarray);
        }
    }

    if (tzNames) {
        delete tzNames;
    }
    if (tzids) {
        delete tzids;
    }

    fLocaleZoneStrings = zarray;
    fZoneStringsRowCount = rows;
    fZoneStringsColCount = 5;
}

void
DateFormatSymbols::setZoneStrings(const UnicodeString* const *strings, int32_t rowCount, int32_t columnCount)
{
    
    
    disposeZoneStrings();
    
    
    fZoneStringsRowCount = rowCount;
    fZoneStringsColCount = columnCount;
    createZoneStrings((const UnicodeString**)strings);
}



const UChar * U_EXPORT2
DateFormatSymbols::getPatternUChars(void)
{
    return gPatternChars;
}

UDateFormatField U_EXPORT2
DateFormatSymbols::getPatternCharIndex(UChar c) {
    const UChar *p = u_strchr(gPatternChars, c);
    if (p == NULL) {
        return UDAT_FIELD_COUNT;
    } else {
        return static_cast<UDateFormatField>(p - gPatternChars);
    }
}

static const uint32_t kNumericFields =
    ((uint32_t)1 << UDAT_YEAR_FIELD) |                      
    ((uint32_t)1 << UDAT_MONTH_FIELD) |                     
    ((uint32_t)1 << UDAT_DATE_FIELD) |                      
    ((uint32_t)1 << UDAT_HOUR_OF_DAY1_FIELD) |              
    ((uint32_t)1 << UDAT_HOUR_OF_DAY0_FIELD) |              
    ((uint32_t)1 << UDAT_MINUTE_FIELD) |                    
    ((uint32_t)1 << UDAT_SECOND_FIELD) |                    
    ((uint32_t)1 << UDAT_FRACTIONAL_SECOND_FIELD) |         
    ((uint32_t)1 << UDAT_DAY_OF_YEAR_FIELD) |               
    ((uint32_t)1 << UDAT_DAY_OF_WEEK_IN_MONTH_FIELD) |      
    ((uint32_t)1 << UDAT_WEEK_OF_YEAR_FIELD) |              
    ((uint32_t)1 << UDAT_WEEK_OF_MONTH_FIELD) |             
    ((uint32_t)1 << UDAT_HOUR1_FIELD) |                     
    ((uint32_t)1 << UDAT_HOUR0_FIELD) |                     
    ((uint32_t)1 << UDAT_YEAR_WOY_FIELD) |                  
    ((uint32_t)1 << UDAT_DOW_LOCAL_FIELD) |                 
    ((uint32_t)1 << UDAT_EXTENDED_YEAR_FIELD);              

UBool U_EXPORT2
DateFormatSymbols::isNumericField(UDateFormatField f, int32_t count) {
    return
        f != UDAT_FIELD_COUNT &&
        (kNumericFields & ((uint32_t)1 << f)) != 0 &&
        (f != UDAT_MONTH_FIELD || count < 3);
}

UBool U_EXPORT2
DateFormatSymbols::isNumericPatternChar(UChar c, int32_t count) {
    return isNumericField(getPatternCharIndex(c), count);
}



UnicodeString&
DateFormatSymbols::getLocalPatternChars(UnicodeString& result) const
{
    
    return result.fastCopyFrom(fLocalPatternChars);
}



void
DateFormatSymbols::setLocalPatternChars(const UnicodeString& newLocalPatternChars)
{
    fLocalPatternChars = newLocalPatternChars;
}



static void
initField(UnicodeString **field, int32_t& length, const UResourceBundle *data, UErrorCode &status) {
    if (U_SUCCESS(status)) {
        int32_t strLen = 0;
        length = ures_getSize(data);
        *field = newUnicodeStringArray(length);
        if (*field) {
            for(int32_t i = 0; i<length; i++) {
                const UChar *resStr = ures_getStringByIndex(data, i, &strLen, &status);
                
                (*(field)+i)->setTo(TRUE, resStr, strLen);
            }
        }
        else {
            length = 0;
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
}

static void
initField(UnicodeString **field, int32_t& length, const UChar *data, LastResortSize numStr, LastResortSize strLen, UErrorCode &status) {
    if (U_SUCCESS(status)) {
        length = numStr;
        *field = newUnicodeStringArray((size_t)numStr);
        if (*field) {
            for(int32_t i = 0; i<length; i++) {
                
                
                (*(field)+i)->setTo(TRUE, data+(i*((int32_t)strLen)), -1);
            }
        }
        else {
            length = 0;
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
}

static void
initLeapMonthPattern(UnicodeString *field, int32_t index, const UResourceBundle *data, UErrorCode &status) {
    field[index].remove();
    if (U_SUCCESS(status)) {
        int32_t strLen = 0;
        const UChar *resStr = ures_getStringByKey(data, gNamesLeapTag, &strLen, &status);
        if (U_SUCCESS(status)) {
            field[index].setTo(TRUE, resStr, strLen);
        }
    }
    status = U_ZERO_ERROR;
}

typedef struct {
    const char * usageTypeName;
    DateFormatSymbols::ECapitalizationContextUsageType usageTypeEnumValue;
} ContextUsageTypeNameToEnumValue;

static const ContextUsageTypeNameToEnumValue contextUsageTypeMap[] = {
   
    { "day-format-except-narrow", DateFormatSymbols::kCapContextUsageDayFormat },
    { "day-narrow",     DateFormatSymbols::kCapContextUsageDayNarrow },
    { "day-standalone-except-narrow", DateFormatSymbols::kCapContextUsageDayStandalone },
    { "era-abbr",       DateFormatSymbols::kCapContextUsageEraAbbrev },
    { "era-name",       DateFormatSymbols::kCapContextUsageEraWide },
    { "era-narrow",     DateFormatSymbols::kCapContextUsageEraNarrow },
    { "metazone-long",  DateFormatSymbols::kCapContextUsageMetazoneLong },
    { "metazone-short", DateFormatSymbols::kCapContextUsageMetazoneShort },
    { "month-format-except-narrow", DateFormatSymbols::kCapContextUsageMonthFormat },
    { "month-narrow",   DateFormatSymbols::kCapContextUsageMonthNarrow },
    { "month-standalone-except-narrow", DateFormatSymbols::kCapContextUsageMonthStandalone },
    { "zone-long",      DateFormatSymbols::kCapContextUsageZoneLong },
    { "zone-short",     DateFormatSymbols::kCapContextUsageZoneShort },
    { NULL, (DateFormatSymbols::ECapitalizationContextUsageType)0 },
};

void
DateFormatSymbols::initializeData(const Locale& locale, const char *type, UErrorCode& status, UBool useLastResortData)
{
    int32_t i;
    int32_t len = 0;
    const UChar *resStr;
    
    fEras = NULL;
    fErasCount = 0;
    fEraNames = NULL;
    fEraNamesCount = 0;
    fNarrowEras = NULL;
    fNarrowErasCount = 0;
    fMonths = NULL;
    fMonthsCount=0;
    fShortMonths = NULL;
    fShortMonthsCount=0;
    fNarrowMonths = NULL;
    fNarrowMonthsCount=0;
    fStandaloneMonths = NULL;
    fStandaloneMonthsCount=0;
    fStandaloneShortMonths = NULL;
    fStandaloneShortMonthsCount=0;
    fStandaloneNarrowMonths = NULL;
    fStandaloneNarrowMonthsCount=0;
    fWeekdays = NULL;
    fWeekdaysCount=0;
    fShortWeekdays = NULL;
    fShortWeekdaysCount=0;
    fNarrowWeekdays = NULL;
    fNarrowWeekdaysCount=0;
    fStandaloneWeekdays = NULL;
    fStandaloneWeekdaysCount=0;
    fStandaloneShortWeekdays = NULL;
    fStandaloneShortWeekdaysCount=0;
    fStandaloneNarrowWeekdays = NULL;
    fStandaloneNarrowWeekdaysCount=0;
    fAmPms = NULL;
    fAmPmsCount=0;
    fQuarters = NULL;
    fQuartersCount = 0;
    fShortQuarters = NULL;
    fShortQuartersCount = 0;
    fStandaloneQuarters = NULL;
    fStandaloneQuartersCount = 0;
    fStandaloneShortQuarters = NULL;
    fStandaloneShortQuartersCount = 0;
    fLeapMonthPatterns = NULL;
    fLeapMonthPatternsCount = 0;
    fShortYearNames = NULL;
    fShortYearNamesCount = 0;
    fZoneStringsRowCount = 0;
    fZoneStringsColCount = 0;
    fZoneStrings = NULL;
    fLocaleZoneStrings = NULL;
    uprv_memset(fCapitalization, 0, sizeof(fCapitalization));

    
    
    
    
    fZSFLocale = locale;
      
    if (U_FAILURE(status)) return;

    




    CalendarData calData(locale, type, status);

    
    UResourceBundle *erasMain = calData.getByKey(gErasTag, status);
    UResourceBundle *eras = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    UErrorCode oldStatus = status;
    UResourceBundle *eraNames = ures_getByKeyWithFallback(erasMain, gNamesWideTag, NULL, &status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { 
       status = oldStatus;
       eraNames = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    }
    
    oldStatus = status;
    UResourceBundle *narrowEras = ures_getByKeyWithFallback(erasMain, gNamesNarrowTag, NULL, &status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = oldStatus;
       narrowEras = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    }

    UErrorCode tempStatus = U_ZERO_ERROR;
    UResourceBundle *monthPatterns = calData.getByKey(gMonthPatternsTag, tempStatus);
    if (U_SUCCESS(tempStatus) && monthPatterns != NULL) {
        fLeapMonthPatterns = newUnicodeStringArray(kMonthPatternsCount);
        if (fLeapMonthPatterns) {
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternFormatWide, calData.getByKey2(gMonthPatternsTag, gNamesWideTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternFormatAbbrev, calData.getByKey2(gMonthPatternsTag, gNamesAbbrTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternFormatNarrow, calData.getByKey2(gMonthPatternsTag, gNamesNarrowTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternStandaloneWide, calData.getByKey3(gMonthPatternsTag, gNamesStandaloneTag, gNamesWideTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternStandaloneAbbrev, calData.getByKey3(gMonthPatternsTag, gNamesStandaloneTag, gNamesAbbrTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternStandaloneNarrow, calData.getByKey3(gMonthPatternsTag, gNamesStandaloneTag, gNamesNarrowTag, tempStatus), tempStatus);
            initLeapMonthPattern(fLeapMonthPatterns, kLeapMonthPatternNumeric, calData.getByKey3(gMonthPatternsTag, gNamesNumericTag, gNamesAllTag, tempStatus), tempStatus);
            if (U_SUCCESS(tempStatus)) {
                fLeapMonthPatternsCount = kMonthPatternsCount;
            } else {
                delete[] fLeapMonthPatterns;
                fLeapMonthPatterns = NULL;
            }
        }
    }

    tempStatus = U_ZERO_ERROR;
    UResourceBundle *cyclicNameSets= calData.getByKey(gCyclicNameSetsTag, tempStatus);
    if (U_SUCCESS(tempStatus) && cyclicNameSets != NULL) {
        UResourceBundle *nameSetYears = ures_getByKeyWithFallback(cyclicNameSets, gNameSetYearsTag, NULL, &tempStatus);
        if (U_SUCCESS(tempStatus)) {
            UResourceBundle *nameSetYearsFmt = ures_getByKeyWithFallback(nameSetYears, gNamesFormatTag, NULL, &tempStatus);
            if (U_SUCCESS(tempStatus)) {
                UResourceBundle *nameSetYearsFmtAbbrev = ures_getByKeyWithFallback(nameSetYearsFmt, gNamesAbbrTag, NULL, &tempStatus);
                if (U_SUCCESS(tempStatus)) {
                    initField(&fShortYearNames, fShortYearNamesCount, nameSetYearsFmtAbbrev, tempStatus);
                    ures_close(nameSetYearsFmtAbbrev);
                }
                ures_close(nameSetYearsFmt);
            }
            ures_close(nameSetYears);
        }
    }

    tempStatus = U_ZERO_ERROR;
    UResourceBundle *localeBundle = ures_open(NULL, locale.getName(), &tempStatus);
    if (U_SUCCESS(tempStatus)) {
        UResourceBundle *contextTransforms = ures_getByKeyWithFallback(localeBundle, gContextTransformsTag, NULL, &tempStatus);
        if (U_SUCCESS(tempStatus)) {
            UResourceBundle *contextTransformUsage;
            while ( (contextTransformUsage = ures_getNextResource(contextTransforms, NULL, &tempStatus)) != NULL ) {
                const int32_t * intVector = ures_getIntVector(contextTransformUsage, &len, &status);
                if (U_SUCCESS(tempStatus) && intVector != NULL && len >= 2) {
                	const char* usageType = ures_getKey(contextTransformUsage);
                	if (usageType != NULL) {
                	    const ContextUsageTypeNameToEnumValue * typeMapPtr = contextUsageTypeMap;
                	    int32_t compResult = 0;
                	    
                	    while ( typeMapPtr->usageTypeName != NULL && (compResult = uprv_strcmp(usageType, typeMapPtr->usageTypeName)) > 0 ) {
                	        ++typeMapPtr;
                	    }
                	    if (typeMapPtr->usageTypeName != NULL && compResult == 0) {
                	        fCapitalization[typeMapPtr->usageTypeEnumValue][0] = intVector[0];
                	        fCapitalization[typeMapPtr->usageTypeEnumValue][1] = intVector[1];
                	    }
                	}
                }
                tempStatus = U_ZERO_ERROR;
                ures_close(contextTransformUsage);
            }
            ures_close(contextTransforms);
        }
        ures_close(localeBundle);
    }

    UResourceBundle *lsweekdaysData = NULL; 
    UResourceBundle *weekdaysData = NULL; 
    UResourceBundle *narrowWeekdaysData = NULL; 
    UResourceBundle *standaloneWeekdaysData = NULL; 
    UResourceBundle *standaloneShortWeekdaysData = NULL; 
    UResourceBundle *standaloneNarrowWeekdaysData = NULL; 

    U_LOCALE_BASED(locBased, *this);
    if (U_FAILURE(status))
    {
        if (useLastResortData)
        {
            
            
            
            

            status = U_USING_FALLBACK_WARNING;

            initField(&fEras, fErasCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fEraNames, fEraNamesCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fNarrowEras, fNarrowErasCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fMonths, fMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen,  status);
            initField(&fShortMonths, fShortMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fNarrowMonths, fNarrowMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fStandaloneMonths, fStandaloneMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen,  status);
            initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fWeekdays, fWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fShortWeekdays, fShortWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fNarrowWeekdays, fNarrowWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneWeekdays, fStandaloneWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fAmPms, fAmPmsCount, (const UChar *)gLastResortAmPmMarkers, kAmPmNum, kAmPmLen, status);
            initField(&fQuarters, fQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fShortQuarters, fShortQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fStandaloneQuarters, fStandaloneQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            fLocalPatternChars.setTo(TRUE, gPatternChars, PATTERN_CHARS_LEN);
        }
        goto cleanup;
    }

    
    
    
    locBased.setLocaleIDs(ures_getLocaleByType(eras, ULOC_VALID_LOCALE, &status),
                          ures_getLocaleByType(eras, ULOC_ACTUAL_LOCALE, &status));

    initField(&fEras, fErasCount, eras, status);
    initField(&fEraNames, fEraNamesCount, eraNames, status);
    initField(&fNarrowEras, fNarrowErasCount, narrowEras, status);

    initField(&fMonths, fMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesWideTag, status), status);
    initField(&fShortMonths, fShortMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);

    initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesNarrowTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status), status);
    }
    if ( status == U_MISSING_RESOURCE_ERROR ) { 
       status = U_ZERO_ERROR;
       initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
    }

    initField(&fStandaloneMonths, fStandaloneMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesWideTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { 
       status = U_ZERO_ERROR;
       initField(&fStandaloneMonths, fStandaloneMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesWideTag, status), status);
    }
    initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesAbbrTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { 
       status = U_ZERO_ERROR;
       initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
    }
    initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { 
       status = U_ZERO_ERROR;
       initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesNarrowTag, status), status);
       if ( status == U_MISSING_RESOURCE_ERROR ) { 
          status = U_ZERO_ERROR;
          initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
       }
    }
    initField(&fAmPms, fAmPmsCount, calData.getByKey(gAmPmMarkersTag, status), status);

    initField(&fQuarters, fQuartersCount, calData.getByKey2(gQuartersTag, gNamesWideTag, status), status);
    initField(&fShortQuarters, fShortQuartersCount, calData.getByKey2(gQuartersTag, gNamesAbbrTag, status), status);

    initField(&fStandaloneQuarters, fStandaloneQuartersCount, calData.getByKey3(gQuartersTag, gNamesStandaloneTag, gNamesWideTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fStandaloneQuarters, fStandaloneQuartersCount, calData.getByKey2(gQuartersTag, gNamesWideTag, status), status);
    }

    initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, calData.getByKey3(gQuartersTag, gNamesStandaloneTag, gNamesAbbrTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, calData.getByKey2(gQuartersTag, gNamesAbbrTag, status), status);
    }

    
    









    fLocalPatternChars.setTo(TRUE, gPatternChars, PATTERN_CHARS_LEN);

    
    weekdaysData = calData.getByKey2(gDayNamesTag, gNamesWideTag, status);
    fWeekdaysCount = ures_getSize(weekdaysData);
    fWeekdays = new UnicodeString[fWeekdaysCount+1];
    
    if (fWeekdays == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(weekdaysData, i, &len, &status);
        
        fWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fWeekdaysCount++;

    lsweekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    fShortWeekdaysCount = ures_getSize(lsweekdaysData);
    fShortWeekdays = new UnicodeString[fShortWeekdaysCount+1];
    
    if (fShortWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fShortWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(lsweekdaysData, i, &len, &status);
        
        fShortWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fShortWeekdaysCount++;

    narrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesNarrowTag, status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        narrowWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status);
    }
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       narrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    }
    fNarrowWeekdaysCount = ures_getSize(narrowWeekdaysData);
    fNarrowWeekdays = new UnicodeString[fNarrowWeekdaysCount+1];
    
    if (fNarrowWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fNarrowWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(narrowWeekdaysData, i, &len, &status);
        
        fNarrowWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fNarrowWeekdaysCount++;

    standaloneWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesWideTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesWideTag, status);
    }
    fStandaloneWeekdaysCount = ures_getSize(standaloneWeekdaysData);
    fStandaloneWeekdays = new UnicodeString[fStandaloneWeekdaysCount+1];
    
    if (fStandaloneWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fStandaloneWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneWeekdaysData, i, &len, &status);
        
        fStandaloneWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneWeekdaysCount++;

    standaloneShortWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesAbbrTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneShortWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    }
    fStandaloneShortWeekdaysCount = ures_getSize(standaloneShortWeekdaysData);
    fStandaloneShortWeekdays = new UnicodeString[fStandaloneShortWeekdaysCount+1];
    
    if (fStandaloneShortWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fStandaloneShortWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneShortWeekdaysData, i, &len, &status);
        
        fStandaloneShortWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneShortWeekdaysCount++;

    standaloneNarrowWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneNarrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesNarrowTag, status);
       if ( status == U_MISSING_RESOURCE_ERROR ) {
          status = U_ZERO_ERROR;
          standaloneNarrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
       }
    }
    fStandaloneNarrowWeekdaysCount = ures_getSize(standaloneNarrowWeekdaysData);
    fStandaloneNarrowWeekdays = new UnicodeString[fStandaloneNarrowWeekdaysCount+1];
    
    if (fStandaloneNarrowWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    
    for(i = 0; i<fStandaloneNarrowWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneNarrowWeekdaysData, i, &len, &status);
        
        fStandaloneNarrowWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneNarrowWeekdaysCount++;

cleanup:
    ures_close(eras);
    ures_close(eraNames);
    ures_close(narrowEras);
}

Locale 
DateFormatSymbols::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

U_NAMESPACE_END

#endif 


