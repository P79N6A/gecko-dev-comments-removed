

























#define ZID_KEY_MAX 128

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/ures.h"
#include "unicode/msgfmt.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/timezone.h"
#include "unicode/decimfmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/basictz.h"
#include "unicode/simpletz.h"
#include "unicode/rbtz.h"
#include "unicode/tzfmt.h"
#include "unicode/utf16.h"
#include "unicode/vtzone.h"
#include "unicode/udisplaycontext.h"
#include "olsontz.h"
#include "patternprops.h"
#include "fphdlimp.h"
#include "gregoimp.h"
#include "hebrwcal.h"
#include "cstring.h"
#include "uassert.h"
#include "cmemory.h"
#include "umutex.h"
#include <float.h>
#include "smpdtfst.h"

#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)
#include <stdio.h>
#endif





U_NAMESPACE_BEGIN

static const UChar PATTERN_CHAR_BASE = 0x40;



















typedef enum GmtPatSize {
    kGmtLen = 3,
    kGmtPatLen = 6,
    kNegHmsLen = 9,
    kNegHmLen = 6,
    kPosHmsLen = 9,
    kPosHmLen = 6,
    kUtLen = 2,
    kUtcLen = 3
} GmtPatSize;



typedef enum OvrStrType {
    kOvrStrDate = 0,
    kOvrStrTime = 1,
    kOvrStrBoth = 2
} OvrStrType;

static const UDateFormatField kDateFields[] = {
    UDAT_YEAR_FIELD,
    UDAT_MONTH_FIELD,
    UDAT_DATE_FIELD,
    UDAT_DAY_OF_YEAR_FIELD,
    UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
    UDAT_WEEK_OF_YEAR_FIELD,
    UDAT_WEEK_OF_MONTH_FIELD,
    UDAT_YEAR_WOY_FIELD,
    UDAT_EXTENDED_YEAR_FIELD,
    UDAT_JULIAN_DAY_FIELD,
    UDAT_STANDALONE_DAY_FIELD,
    UDAT_STANDALONE_MONTH_FIELD,
    UDAT_QUARTER_FIELD,
    UDAT_STANDALONE_QUARTER_FIELD,
    UDAT_YEAR_NAME_FIELD };
static const int8_t kDateFieldsCount = 15;

static const UDateFormatField kTimeFields[] = {
    UDAT_HOUR_OF_DAY1_FIELD,
    UDAT_HOUR_OF_DAY0_FIELD,
    UDAT_MINUTE_FIELD,
    UDAT_SECOND_FIELD,
    UDAT_FRACTIONAL_SECOND_FIELD,
    UDAT_HOUR1_FIELD,
    UDAT_HOUR0_FIELD,
    UDAT_MILLISECONDS_IN_DAY_FIELD,
    UDAT_TIMEZONE_RFC_FIELD };
static const int8_t kTimeFieldsCount = 9;




static const UChar gDefaultPattern[] =
{
    0x79, 0x79, 0x79, 0x79, 0x4D, 0x4D, 0x64, 0x64, 0x20, 0x68, 0x68, 0x3A, 0x6D, 0x6D, 0x20, 0x61, 0
};  




static const UChar SUPPRESS_NEGATIVE_PREFIX[] = {0xAB00, 0};





static const char gDateTimePatternsTag[]="DateTimePatterns";


static const UChar QUOTE = 0x27; 










static const int32_t gFieldRangeBias[] = {
    -1,  
    -1,  
     1,  
     0,  
    -1,  
    -1,  
     0,  
     0,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
    -1,  
     0,  
     1,  
    -1,  
    -1,  
    -1   
    -1,  
};



static const int32_t HEBREW_CAL_CUR_MILLENIUM_START_YEAR = 5000;
static const int32_t HEBREW_CAL_CUR_MILLENIUM_END_YEAR = 6000;

static UMutex LOCK = U_MUTEX_INITIALIZER;

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SimpleDateFormat)



SimpleDateFormat::~SimpleDateFormat()
{
    delete fSymbols;
    if (fNumberFormatters) {
        uprv_free(fNumberFormatters);
    }
    if (fTimeZoneFormat) {
        delete fTimeZoneFormat;
    }

    while (fOverrideList) {
        NSOverride *cur = fOverrideList;
        fOverrideList = cur->next;
        delete cur->nf;
        uprv_free(cur);
    }
}



SimpleDateFormat::SimpleDateFormat(UErrorCode& status)
  :   fLocale(Locale::getDefault()),
      fSymbols(NULL),
      fTimeZoneFormat(NULL),
      fNumberFormatters(NULL),
      fOverrideList(NULL),
      fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    construct(kShort, (EStyle) (kShort + kDateOffset), fLocale, status);
    initializeDefaultCentury();
}



SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   UErrorCode &status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(NULL),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();
    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

}


SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const UnicodeString& override,
                                   UErrorCode &status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(NULL),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    fDateOverride.setTo(override);
    fTimeOverride.setToBogus();
    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

    processOverrideString(fLocale,override,kOvrStrBoth,status);

}



SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(locale),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}



SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const UnicodeString& override,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(locale),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{

    fDateOverride.setTo(override);
    fTimeOverride.setToBogus();

    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

    processOverrideString(locale,override,kOvrStrBoth,status);

}



SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   DateFormatSymbols* symbolsToAdopt,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(symbolsToAdopt),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeCalendar(NULL,fLocale,status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}



SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const DateFormatSymbols& symbols,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(new DateFormatSymbols(symbols)),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeCalendar(NULL, fLocale, status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}




SimpleDateFormat::SimpleDateFormat(EStyle timeStyle,
                                   EStyle dateStyle,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fLocale(locale),
    fSymbols(NULL),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    construct(timeStyle, dateStyle, fLocale, status);
    if(U_SUCCESS(status)) {
      initializeDefaultCentury();
    }
}








SimpleDateFormat::SimpleDateFormat(const Locale& locale,
                                   UErrorCode& status)
:   fPattern(gDefaultPattern),
    fLocale(locale),
    fSymbols(NULL),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    if (U_FAILURE(status)) return;
    initializeSymbols(fLocale, initializeCalendar(NULL, fLocale, status),status);
    if (U_FAILURE(status))
    {
        status = U_ZERO_ERROR;
        delete fSymbols;
        
        fSymbols = new DateFormatSymbols(status);
        
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initialize(fLocale, status);
    if(U_SUCCESS(status)) {
      initializeDefaultCentury();
    }
}



SimpleDateFormat::SimpleDateFormat(const SimpleDateFormat& other)
:   DateFormat(other),
    fLocale(other.fLocale),
    fSymbols(NULL),
    fTimeZoneFormat(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    *this = other;
}



SimpleDateFormat& SimpleDateFormat::operator=(const SimpleDateFormat& other)
{
    if (this == &other) {
        return *this;
    }
    DateFormat::operator=(other);

    delete fSymbols;
    fSymbols = NULL;

    if (other.fSymbols)
        fSymbols = new DateFormatSymbols(*other.fSymbols);

    fDefaultCenturyStart         = other.fDefaultCenturyStart;
    fDefaultCenturyStartYear     = other.fDefaultCenturyStartYear;
    fHaveDefaultCentury          = other.fHaveDefaultCentury;

    fPattern = other.fPattern;

    
    if (fLocale != other.fLocale) {
        delete fTimeZoneFormat;
        fTimeZoneFormat = NULL; 
        fLocale = other.fLocale;
    }

    fCapitalizationContext = other.fCapitalizationContext;

    return *this;
}



Format*
SimpleDateFormat::clone() const
{
    return new SimpleDateFormat(*this);
}



UBool
SimpleDateFormat::operator==(const Format& other) const
{
    if (DateFormat::operator==(other)) {
        
        SimpleDateFormat* that = (SimpleDateFormat*)&other;
        return (fPattern             == that->fPattern &&
                fSymbols             != NULL && 
                that->fSymbols       != NULL && 
                *fSymbols            == *that->fSymbols &&
                fHaveDefaultCentury  == that->fHaveDefaultCentury &&
                fDefaultCenturyStart == that->fDefaultCenturyStart &&
                fCapitalizationContext == that->fCapitalizationContext);
    }
    return FALSE;
}



void SimpleDateFormat::construct(EStyle timeStyle,
                                 EStyle dateStyle,
                                 const Locale& locale,
                                 UErrorCode& status)
{
    
    if (U_FAILURE(status)) return;

    
    initializeCalendar(NULL, locale, status);
    if (U_FAILURE(status)) return;

    CalendarData calData(locale, fCalendar?fCalendar->getType():NULL, status);
    UResourceBundle *dateTimePatterns = calData.getByKey(gDateTimePatternsTag, status);
    UResourceBundle *currentBundle;

    if (U_FAILURE(status)) return;

    if (ures_getSize(dateTimePatterns) <= kDateTime)
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }

    setLocaleIDs(ures_getLocaleByType(dateTimePatterns, ULOC_VALID_LOCALE, &status),
                 ures_getLocaleByType(dateTimePatterns, ULOC_ACTUAL_LOCALE, &status));

    
    initializeSymbols(locale,fCalendar, status);
    if (U_FAILURE(status)) return;
    
    if (fSymbols == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    const UChar *resStr,*ovrStr;
    int32_t resStrLen,ovrStrLen = 0;
    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    
    
    
    
    if ((timeStyle != kNone) && (dateStyle != kNone))
    {
        Formattable timeDateArray[2];

        
        
        

        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)timeStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fTimeOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        ures_close(currentBundle);

        UnicodeString *tempus1 = new UnicodeString(TRUE, resStr, resStrLen);
        
        if (tempus1 == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        timeDateArray[0].adoptString(tempus1);

        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)dateStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        ures_close(currentBundle);

        UnicodeString *tempus2 = new UnicodeString(TRUE, resStr, resStrLen);
        
        if (tempus2 == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        timeDateArray[1].adoptString(tempus2);

        int32_t glueIndex = kDateTime;
        int32_t patternsSize = ures_getSize(dateTimePatterns);
        if (patternsSize >= (kDateTimeOffset + kShort + 1)) {
            
            glueIndex = (int32_t)(kDateTimeOffset + (dateStyle - kDateOffset));
        }

        resStr = ures_getStringByIndex(dateTimePatterns, glueIndex, &resStrLen, &status);
        MessageFormat::format(UnicodeString(TRUE, resStr, resStrLen), timeDateArray, 2, fPattern, status);
    }
    
    
    
    else if (timeStyle != kNone) {
        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)timeStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
                ures_close(currentBundle);
               return;
            }
        }
        fPattern.setTo(TRUE, resStr, resStrLen);
        ures_close(currentBundle);
    }
    else if (dateStyle != kNone) {
        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)dateStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        fPattern.setTo(TRUE, resStr, resStrLen);
        ures_close(currentBundle);
    }

    
    else
        status = U_INVALID_FORMAT_ERROR;

    
    initialize(locale, status);
}



Calendar*
SimpleDateFormat::initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status)
{
    if(!U_FAILURE(status)) {
        fCalendar = Calendar::createInstance(adoptZone?adoptZone:TimeZone::createDefault(), locale, status);
    }
    if (U_SUCCESS(status) && fCalendar == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return fCalendar;
}

void
SimpleDateFormat::initializeSymbols(const Locale& locale, Calendar* calendar, UErrorCode& status)
{
  if(U_FAILURE(status)) {
    fSymbols = NULL;
  } else {
    
    fSymbols = new DateFormatSymbols(locale, calendar?calendar->getType() :NULL , status);
    
    if (fSymbols == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
  }
}

void
SimpleDateFormat::initialize(const Locale& locale,
                             UErrorCode& status)
{
    if (U_FAILURE(status)) return;

    
    
    fNumberFormat = NumberFormat::createInstance(locale, status);
    if (fNumberFormat != NULL && U_SUCCESS(status))
    {
        
        
        

        fNumberFormat->setGroupingUsed(FALSE);
        DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fNumberFormat);
        if (decfmt != NULL) {
            decfmt->setDecimalSeparatorAlwaysShown(FALSE);
        }
        fNumberFormat->setParseIntegerOnly(TRUE);
        fNumberFormat->setMinimumFractionDigits(0); 

        

        initNumberFormatters(locale,status);

    }
    else if (U_SUCCESS(status))
    {
        status = U_MISSING_RESOURCE_ERROR;
    }
}




void SimpleDateFormat::initializeDefaultCentury()
{
  if(fCalendar) {
    fHaveDefaultCentury = fCalendar->haveDefaultCentury();
    if(fHaveDefaultCentury) {
      fDefaultCenturyStart = fCalendar->defaultCenturyStart();
      fDefaultCenturyStartYear = fCalendar->defaultCenturyStartYear();
    } else {
      fDefaultCenturyStart = DBL_MIN;
      fDefaultCenturyStartYear = -1;
    }
  }
}




void SimpleDateFormat::parseAmbiguousDatesAsAfter(UDate startDate, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    if(!fCalendar) {
      status = U_ILLEGAL_ARGUMENT_ERROR;
      return;
    }

    fCalendar->setTime(startDate, status);
    if(U_SUCCESS(status)) {
        fHaveDefaultCentury = TRUE;
        fDefaultCenturyStart = startDate;
        fDefaultCenturyStartYear = fCalendar->get(UCAL_YEAR, status);
    }
}



UnicodeString&
SimpleDateFormat::format(Calendar& cal, UnicodeString& appendTo, FieldPosition& pos) const
{
  UErrorCode status = U_ZERO_ERROR;
  FieldPositionOnlyHandler handler(pos);
  return _format(cal, appendTo, handler, status);
}



UnicodeString&
SimpleDateFormat::format(Calendar& cal, UnicodeString& appendTo,
                         FieldPositionIterator* posIter, UErrorCode& status) const
{
  FieldPositionIteratorHandler handler(posIter, status);
  return _format(cal, appendTo, handler, status);
}



UnicodeString&
SimpleDateFormat::_format(Calendar& cal, UnicodeString& appendTo,
                            FieldPositionHandler& handler, UErrorCode& status) const
{
    if ( U_FAILURE(status) ) {
       return appendTo; 
    }
    Calendar* workCal = &cal;
    Calendar* calClone = NULL;
    if (&cal != fCalendar && uprv_strcmp(cal.getType(), fCalendar->getType()) != 0) {
        
        
        
        calClone = fCalendar->clone();
        if (calClone != NULL) {
            UDate t = cal.getTime(status);
            calClone->setTime(t, status);
            calClone->setTimeZone(cal.getTimeZone());
            workCal = calClone;
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
            return appendTo;
        }
    }

    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;
    int32_t fieldNum = 0;

    
    for (int32_t i = 0; i < fPattern.length() && U_SUCCESS(status); ++i) {
        UChar ch = fPattern[i];

        
        
        if (ch != prevCh && count > 0) {
            subFormat(appendTo, prevCh, count, fCapitalizationContext, fieldNum++, handler, *workCal, status);
            count = 0;
        }
        if (ch == QUOTE) {
            
            
            if ((i+1) < fPattern.length() && fPattern[i+1] == QUOTE) {
                appendTo += (UChar)QUOTE;
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        }
        else if ( ! inQuote && ((ch >= 0x0061  && ch <= 0x007A )
                    || (ch >= 0x0041  && ch <= 0x005A ))) {
            
            
            prevCh = ch;
            ++count;
        }
        else {
            
            appendTo += ch;
        }
    }

    
    if (count > 0) {
        subFormat(appendTo, prevCh, count, fCapitalizationContext, fieldNum++, handler, *workCal, status);
    }

    if (calClone != NULL) {
        delete calClone;
    }

    return appendTo;
}









const int32_t
SimpleDateFormat::fgCalendarFieldToLevel[] =
{
     0, 10, 20,
     20, 30,
     30, 20, 30, 30,
     40, 50, 50, 60,
     70, 80,
     0, 0, 10,
     30, 10, 0,
     40
};






const int32_t
SimpleDateFormat::fgPatternCharToLevel[] = {
    
        -1, 40, -1, -1, 20, 30, 30,  0, 50, -1, -1, 50, 20, 20, -1, -1,
    
        -1, 20, -1, 80, -1, 10,  0, 30, -1, 10,  0, -1, -1, -1, -1, -1,
    
        -1, 40, -1, 30, 30, 30, -1,  0, 50, -1, -1, 50, -1, 60, -1, -1,
    
        -1, 20, -1, 70, -1, 10,  0, 20, -1, 10,  0, -1, -1, -1, -1, -1
};



const UCalendarDateFields
SimpleDateFormat::fgPatternIndexToCalendarField[] =
{
     UCAL_ERA, UCAL_YEAR, UCAL_MONTH,
     UCAL_DATE, UCAL_HOUR_OF_DAY, UCAL_HOUR_OF_DAY,
     UCAL_MINUTE, UCAL_SECOND, UCAL_MILLISECOND,
     UCAL_DAY_OF_WEEK, UCAL_DAY_OF_YEAR, UCAL_DAY_OF_WEEK_IN_MONTH,
     UCAL_WEEK_OF_YEAR, UCAL_WEEK_OF_MONTH, UCAL_AM_PM,
     UCAL_HOUR, UCAL_HOUR, UCAL_ZONE_OFFSET,
     UCAL_YEAR_WOY, UCAL_DOW_LOCAL, UCAL_EXTENDED_YEAR,
     UCAL_JULIAN_DAY, UCAL_MILLISECONDS_IN_DAY, UCAL_ZONE_OFFSET,
       UCAL_ZONE_OFFSET,
       UCAL_DOW_LOCAL,
       UCAL_MONTH,
       UCAL_MONTH,
       UCAL_MONTH,
       UCAL_ZONE_OFFSET,
       UCAL_YEAR,
};


const UDateFormatField
SimpleDateFormat::fgPatternIndexToDateFormatField[] = {
     UDAT_ERA_FIELD, UDAT_YEAR_FIELD, UDAT_MONTH_FIELD,
     UDAT_DATE_FIELD, UDAT_HOUR_OF_DAY1_FIELD, UDAT_HOUR_OF_DAY0_FIELD,
     UDAT_MINUTE_FIELD, UDAT_SECOND_FIELD, UDAT_FRACTIONAL_SECOND_FIELD,
     UDAT_DAY_OF_WEEK_FIELD, UDAT_DAY_OF_YEAR_FIELD, UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
     UDAT_WEEK_OF_YEAR_FIELD, UDAT_WEEK_OF_MONTH_FIELD, UDAT_AM_PM_FIELD,
     UDAT_HOUR1_FIELD, UDAT_HOUR0_FIELD, UDAT_TIMEZONE_FIELD,
     UDAT_YEAR_WOY_FIELD, UDAT_DOW_LOCAL_FIELD, UDAT_EXTENDED_YEAR_FIELD,
     UDAT_JULIAN_DAY_FIELD, UDAT_MILLISECONDS_IN_DAY_FIELD, UDAT_TIMEZONE_RFC_FIELD,
       UDAT_TIMEZONE_GENERIC_FIELD,
       UDAT_STANDALONE_DAY_FIELD,
       UDAT_STANDALONE_MONTH_FIELD,
       UDAT_QUARTER_FIELD,
       UDAT_STANDALONE_QUARTER_FIELD,
       UDAT_TIMEZONE_SPECIAL_FIELD,
       UDAT_YEAR_NAME_FIELD,
};







static inline void
_appendSymbol(UnicodeString& dst,
              int32_t value,
              const UnicodeString* symbols,
              int32_t symbolsCount) {
    U_ASSERT(0 <= value && value < symbolsCount);
    if (0 <= value && value < symbolsCount) {
        dst += symbols[value];
    }
}

static inline void
_appendSymbolWithMonthPattern(UnicodeString& dst, int32_t value, const UnicodeString* symbols, int32_t symbolsCount,
              const UnicodeString* monthPattern, UErrorCode& status) {
    U_ASSERT(0 <= value && value < symbolsCount);
    if (0 <= value && value < symbolsCount) {
        if (monthPattern == NULL) {
            dst += symbols[value];
        } else {
            Formattable monthName((const UnicodeString&)(symbols[value]));
            MessageFormat::format(*monthPattern, &monthName, 1, dst, status);
        }
    }
}


void
SimpleDateFormat::initNumberFormatters(const Locale &locale,UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ( fDateOverride.isBogus() && fTimeOverride.isBogus() ) {
        return;
    }
    umtx_lock(&LOCK);
    if (fNumberFormatters == NULL) {
        fNumberFormatters = (NumberFormat**)uprv_malloc(UDAT_FIELD_COUNT * sizeof(NumberFormat*));
        if (fNumberFormatters) {
            for (int32_t i = 0; i < UDAT_FIELD_COUNT; i++) {
                fNumberFormatters[i] = fNumberFormat;
            }
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    umtx_unlock(&LOCK);

    processOverrideString(locale,fDateOverride,kOvrStrDate,status);
    processOverrideString(locale,fTimeOverride,kOvrStrTime,status);

}

void
SimpleDateFormat::processOverrideString(const Locale &locale, const UnicodeString &str, int8_t type, UErrorCode &status) {
    if (str.isBogus()) {
        return;
    }
    int32_t start = 0;
    int32_t len;
    UnicodeString nsName;
    UnicodeString ovrField;
    UBool moreToProcess = TRUE;

    while (moreToProcess) {
        int32_t delimiterPosition = str.indexOf((UChar)ULOC_KEYWORD_ITEM_SEPARATOR_UNICODE,start);
        if (delimiterPosition == -1) {
            moreToProcess = FALSE;
            len = str.length() - start;
        } else {
            len = delimiterPosition - start;
        }
        UnicodeString currentString(str,start,len);
        int32_t equalSignPosition = currentString.indexOf((UChar)ULOC_KEYWORD_ASSIGN_UNICODE,0);
        if (equalSignPosition == -1) { 
            nsName.setTo(currentString);
            ovrField.setToBogus();
        } else { 
            nsName.setTo(currentString,equalSignPosition+1);
            ovrField.setTo(currentString,0,1); 
        }

        int32_t nsNameHash = nsName.hashCode();
        
        NSOverride *cur = fOverrideList;
        NumberFormat *nf = NULL;
        UBool found = FALSE;
        while ( cur && !found ) {
            if ( cur->hash == nsNameHash ) {
                nf = cur->nf;
                found = TRUE;
            }
            cur = cur->next;
        }

        if (!found) {
           cur = (NSOverride *)uprv_malloc(sizeof(NSOverride));
           if (cur) {
               char kw[ULOC_KEYWORD_AND_VALUES_CAPACITY];
               uprv_strcpy(kw,"numbers=");
               nsName.extract(0,len,kw+8,ULOC_KEYWORD_AND_VALUES_CAPACITY-8,US_INV);

               Locale ovrLoc(locale.getLanguage(),locale.getCountry(),locale.getVariant(),kw);
               nf = NumberFormat::createInstance(ovrLoc,status);

               
               
               

               if (U_SUCCESS(status)) {
                   nf->setGroupingUsed(FALSE);
                   DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(nf);
                   if (decfmt != NULL) {
                       decfmt->setDecimalSeparatorAlwaysShown(FALSE);
                   }
                   nf->setParseIntegerOnly(TRUE);
                   nf->setMinimumFractionDigits(0); 

                   cur->nf = nf;
                   cur->hash = nsNameHash;
                   cur->next = fOverrideList;
                   fOverrideList = cur;
               }
               else {
                   
                   if (cur != NULL) {
                       uprv_free(cur);
                   }
                  return;
               }

           } else {
               status = U_MEMORY_ALLOCATION_ERROR;
               return;
           }
        }

        
        

        if (ovrField.isBogus()) {
            switch (type) {
                case kOvrStrDate:
                case kOvrStrBoth: {
                    for ( int8_t i=0 ; i<kDateFieldsCount; i++ ) {
                        fNumberFormatters[kDateFields[i]] = nf;
                    }
                    if (type==kOvrStrDate) {
                        break;
                    }
                }
                case kOvrStrTime : {
                    for ( int8_t i=0 ; i<kTimeFieldsCount; i++ ) {
                        fNumberFormatters[kTimeFields[i]] = nf;
                    }
                    break;
                }
            }
        } else {
           
           UDateFormatField patternCharIndex =
              DateFormatSymbols::getPatternCharIndex(ovrField.charAt(0));
           if (patternCharIndex == UDAT_FIELD_COUNT) {
               status = U_INVALID_FORMAT_ERROR;
               return;
           }

           
           fNumberFormatters[patternCharIndex] = nf;
        }

        start = delimiterPosition + 1;
    }
}


void
SimpleDateFormat::subFormat(UnicodeString &appendTo,
                            UChar ch,
                            int32_t count,
                            UDisplayContext capitalizationContext,
                            int32_t fieldNum,
                            FieldPositionHandler& handler,
                            Calendar& cal,
                            UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return;
    }

    
    

    UDateFormatField patternCharIndex = DateFormatSymbols::getPatternCharIndex(ch);
    const int32_t maxIntCount = 10;
    int32_t beginOffset = appendTo.length();
    NumberFormat *currentNumberFormat;
    DateFormatSymbols::ECapitalizationContextUsageType capContextUsageType = DateFormatSymbols::kCapContextUsageOther;

    UBool isHebrewCalendar = (uprv_strcmp(cal.getType(),"hebrew") == 0);
    UBool isChineseCalendar = (uprv_strcmp(cal.getType(),"chinese") == 0);

    
    if (patternCharIndex == UDAT_FIELD_COUNT)
    {
        if (ch != 0x6C) { 
            status = U_INVALID_FORMAT_ERROR;
        }
        return;
    }

    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];
    int32_t value = cal.get(field, status);
    if (U_FAILURE(status)) {
        return;
    }

    currentNumberFormat = getNumberFormatByIndex(patternCharIndex);
    UnicodeString hebr("hebr", 4, US_INV);
    
    switch (patternCharIndex) {

    
    
    case UDAT_ERA_FIELD:
        if (isChineseCalendar) {
            zeroPaddingNumber(currentNumberFormat,appendTo, value, 1, 9); 
        } else {
            if (count == 5) {
                _appendSymbol(appendTo, value, fSymbols->fNarrowEras, fSymbols->fNarrowErasCount);
                capContextUsageType = DateFormatSymbols::kCapContextUsageEraNarrow;
            } else if (count == 4) {
                _appendSymbol(appendTo, value, fSymbols->fEraNames, fSymbols->fEraNamesCount);
                capContextUsageType = DateFormatSymbols::kCapContextUsageEraWide;
            } else {
                _appendSymbol(appendTo, value, fSymbols->fEras, fSymbols->fErasCount);
                capContextUsageType = DateFormatSymbols::kCapContextUsageEraAbbrev;
            }
        }
        break;

     case UDAT_YEAR_NAME_FIELD:
        if (fSymbols->fShortYearNames != NULL && value <= fSymbols->fShortYearNamesCount) {
            
            _appendSymbol(appendTo, value - 1, fSymbols->fShortYearNames, fSymbols->fShortYearNamesCount);
            break;
        }
        

   
    






    case UDAT_YEAR_FIELD:
    case UDAT_YEAR_WOY_FIELD:
        if (fDateOverride.compare(hebr)==0 && value>HEBREW_CAL_CUR_MILLENIUM_START_YEAR && value<HEBREW_CAL_CUR_MILLENIUM_END_YEAR) {
            value-=HEBREW_CAL_CUR_MILLENIUM_START_YEAR;
        }
        if(count == 2)
            zeroPaddingNumber(currentNumberFormat, appendTo, value, 2, 2);
        else
            zeroPaddingNumber(currentNumberFormat, appendTo, value, count, maxIntCount);
        break;

    
    
    
    
    case UDAT_MONTH_FIELD:
    case UDAT_STANDALONE_MONTH_FIELD:
        if ( isHebrewCalendar ) {
           HebrewCalendar *hc = (HebrewCalendar*)&cal;
           if (hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value == 6 && count >= 3 )
               value = 13; 
           if (!hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value >= 6 && count < 3 )
               value--; 
        }
        {
            int32_t isLeapMonth = (fSymbols->fLeapMonthPatterns != NULL && fSymbols->fLeapMonthPatternsCount >= DateFormatSymbols::kMonthPatternsCount)?
                        cal.get(UCAL_IS_LEAP_MONTH, status): 0;
            
            if (count == 5) {
                if (patternCharIndex == UDAT_MONTH_FIELD) {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fNarrowMonths, fSymbols->fNarrowMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternFormatNarrow]): NULL, status);
                } else {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fStandaloneNarrowMonths, fSymbols->fStandaloneNarrowMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternStandaloneNarrow]): NULL, status);
                }
                capContextUsageType = DateFormatSymbols::kCapContextUsageMonthNarrow;
            } else if (count == 4) {
                if (patternCharIndex == UDAT_MONTH_FIELD) {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fMonths, fSymbols->fMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternFormatWide]): NULL, status);
                    capContextUsageType = DateFormatSymbols::kCapContextUsageMonthFormat;
                } else {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fStandaloneMonths, fSymbols->fStandaloneMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternStandaloneWide]): NULL, status);
                    capContextUsageType = DateFormatSymbols::kCapContextUsageMonthStandalone;
                }
            } else if (count == 3) {
                if (patternCharIndex == UDAT_MONTH_FIELD) {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fShortMonths, fSymbols->fShortMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternFormatAbbrev]): NULL, status);
                    capContextUsageType = DateFormatSymbols::kCapContextUsageMonthFormat;
                } else {
                    _appendSymbolWithMonthPattern(appendTo, value, fSymbols->fStandaloneShortMonths, fSymbols->fStandaloneShortMonthsCount,
                            (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternStandaloneAbbrev]): NULL, status);
                    capContextUsageType = DateFormatSymbols::kCapContextUsageMonthStandalone;
                }
            } else {
                UnicodeString monthNumber;
                zeroPaddingNumber(currentNumberFormat,monthNumber, value + 1, count, maxIntCount);
                _appendSymbolWithMonthPattern(appendTo, 0, &monthNumber, 1,
                        (isLeapMonth!=0)? &(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternNumeric]): NULL, status);
            }
        }
        break;

    
    case UDAT_HOUR_OF_DAY1_FIELD:
        if (value == 0)
            zeroPaddingNumber(currentNumberFormat,appendTo, cal.getMaximum(UCAL_HOUR_OF_DAY) + 1, count, maxIntCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;

    case UDAT_FRACTIONAL_SECOND_FIELD:
        
        {
            currentNumberFormat->setMinimumIntegerDigits((count > 3) ? 3 : count);
            currentNumberFormat->setMaximumIntegerDigits(maxIntCount);
            if (count == 1) {
                value /= 100;
            } else if (count == 2) {
                value /= 10;
            }
            FieldPosition p(0);
            currentNumberFormat->format(value, appendTo, p);
            if (count > 3) {
                currentNumberFormat->setMinimumIntegerDigits(count - 3);
                currentNumberFormat->format((int32_t)0, appendTo, p);
            }
        }
        break;

    
    
    
    
    case UDAT_DOW_LOCAL_FIELD:
        if ( count < 3 ) {
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
            break;
        }
        
        
        value = cal.get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) {
            return;
        }
        
    case UDAT_DAY_OF_WEEK_FIELD:
        if (count == 5) {
            _appendSymbol(appendTo, value, fSymbols->fNarrowWeekdays,
                          fSymbols->fNarrowWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayNarrow;
        } else if (count == 4) {
            _appendSymbol(appendTo, value, fSymbols->fWeekdays,
                          fSymbols->fWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayFormat;
        } else {
            _appendSymbol(appendTo, value, fSymbols->fShortWeekdays,
                          fSymbols->fShortWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayFormat;
        }
        break;

    
    
    
    case UDAT_STANDALONE_DAY_FIELD:
        if ( count < 3 ) {
            zeroPaddingNumber(currentNumberFormat,appendTo, value, 1, maxIntCount);
            break;
        }
        
        
        value = cal.get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) {
            return;
        }
        if (count == 5) {
            _appendSymbol(appendTo, value, fSymbols->fStandaloneNarrowWeekdays,
                          fSymbols->fStandaloneNarrowWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayNarrow;
        } else if (count == 4) {
            _appendSymbol(appendTo, value, fSymbols->fStandaloneWeekdays,
                          fSymbols->fStandaloneWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayStandalone;
        } else { 
            _appendSymbol(appendTo, value, fSymbols->fStandaloneShortWeekdays,
                          fSymbols->fStandaloneShortWeekdaysCount);
            capContextUsageType = DateFormatSymbols::kCapContextUsageDayStandalone;
        }
        break;

    
    case UDAT_AM_PM_FIELD:
        _appendSymbol(appendTo, value, fSymbols->fAmPms,
                      fSymbols->fAmPmsCount);
        break;

    
    
    case UDAT_HOUR1_FIELD:
        if (value == 0)
            zeroPaddingNumber(currentNumberFormat,appendTo, cal.getLeastMaximum(UCAL_HOUR) + 1, count, maxIntCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;

    
    
    
    
    
    
    case UDAT_TIMEZONE_FIELD:
    case UDAT_TIMEZONE_GENERIC_FIELD:
    case UDAT_TIMEZONE_SPECIAL_FIELD:
    case UDAT_TIMEZONE_RFC_FIELD: 
        {
            UnicodeString zoneString;
            const TimeZone& tz = cal.getTimeZone();
            UDate date = cal.getTime(status);
            if (U_SUCCESS(status)) {
                if (patternCharIndex == UDAT_TIMEZONE_RFC_FIELD) {
                    if (count < 4) {
                        
                        tzFormat()->format(UTZFMT_STYLE_RFC822, tz, date, zoneString);
                    } else if (count == 5) {
                        
                        tzFormat()->format(UTZFMT_STYLE_ISO8601, tz, date, zoneString);
                    } else {
                        
                        tzFormat()->format(UTZFMT_STYLE_LOCALIZED_GMT, tz, date, zoneString);
                    }
                } else if (patternCharIndex == UDAT_TIMEZONE_FIELD) {
                    if (count < 4) {
                        
                        tzFormat()->format(UTZFMT_STYLE_SPECIFIC_SHORT, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageMetazoneShort;
                    } else {
                        
                        tzFormat()->format(UTZFMT_STYLE_SPECIFIC_LONG, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageMetazoneLong;
                    }
                } else if (patternCharIndex == UDAT_TIMEZONE_GENERIC_FIELD) {
                    if (count == 1) {
                        
                        tzFormat()->format(UTZFMT_STYLE_GENERIC_SHORT, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageMetazoneShort;
                    } else if (count == 4) {
                        
                        tzFormat()->format(UTZFMT_STYLE_GENERIC_LONG, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageMetazoneLong;
                    }
                } else { 
                    if (count == 1) {
                        
                        tzFormat()->format(UTZFMT_STYLE_SPECIFIC_SHORT, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageMetazoneShort;
                    } else if (count == 4) {
                        
                        tzFormat()->format(UTZFMT_STYLE_GENERIC_LOCATION, tz, date, zoneString);
                        capContextUsageType = DateFormatSymbols::kCapContextUsageZoneLong;
                    }
                }
            }
            appendTo += zoneString;
        }
        break;

    case UDAT_QUARTER_FIELD:
        if (count >= 4)
            _appendSymbol(appendTo, value/3, fSymbols->fQuarters,
                          fSymbols->fQuartersCount);
        else if (count == 3)
            _appendSymbol(appendTo, value/3, fSymbols->fShortQuarters,
                          fSymbols->fShortQuartersCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, (value/3) + 1, count, maxIntCount);
        break;

    case UDAT_STANDALONE_QUARTER_FIELD:
        if (count >= 4)
            _appendSymbol(appendTo, value/3, fSymbols->fStandaloneQuarters,
                          fSymbols->fStandaloneQuartersCount);
        else if (count == 3)
            _appendSymbol(appendTo, value/3, fSymbols->fStandaloneShortQuarters,
                          fSymbols->fStandaloneShortQuartersCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, (value/3) + 1, count, maxIntCount);
        break;


    
    
    default:
        zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;
    }
#if !UCONFIG_NO_BREAK_ITERATION
    if (fieldNum == 0) {
        
        UBool titlecase = FALSE;
        switch (capitalizationContext) {
            case UDISPCTX_CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE:
                titlecase = TRUE;
                break;
            case UDISPCTX_CAPITALIZATION_FOR_UI_LIST_OR_MENU:
                titlecase = fSymbols->fCapitalization[capContextUsageType][0];
                break;
            case UDISPCTX_CAPITALIZATION_FOR_STANDALONE:
                titlecase = fSymbols->fCapitalization[capContextUsageType][1];
                break;
            default:
                
                break;
        }
        if (titlecase) {
            UnicodeString firstField(appendTo, beginOffset);
            firstField.toTitle(NULL, fLocale, U_TITLECASE_NO_LOWERCASE | U_TITLECASE_NO_BREAK_ADJUSTMENT);
            appendTo.replaceBetween(beginOffset, appendTo.length(), firstField);
        }
    }
#endif

    handler.addAttribute(fgPatternIndexToDateFormatField[patternCharIndex], beginOffset, appendTo.length());
}



NumberFormat *
SimpleDateFormat::getNumberFormatByIndex(UDateFormatField index) const {
    if (fNumberFormatters != NULL) {
        return fNumberFormatters[index];
    } else {
        return fNumberFormat;
    }
}


void
SimpleDateFormat::zeroPaddingNumber(NumberFormat *currentNumberFormat,UnicodeString &appendTo,
                                    int32_t value, int32_t minDigits, int32_t maxDigits) const
{
    if (currentNumberFormat!=NULL) {
        FieldPosition pos(0);

        currentNumberFormat->setMinimumIntegerDigits(minDigits);
        currentNumberFormat->setMaximumIntegerDigits(maxDigits);
        currentNumberFormat->format(value, appendTo, pos);  
    }
}







UBool SimpleDateFormat::isNumeric(UChar formatChar, int32_t count) {
    return DateFormatSymbols::isNumericPatternChar(formatChar, count);
}

UBool
SimpleDateFormat::isAtNumericField(const UnicodeString &pattern, int32_t patternOffset) {
    if (patternOffset >= pattern.length()) {
        
        return FALSE;
    }
    UChar ch = pattern.charAt(patternOffset);
    UDateFormatField f = DateFormatSymbols::getPatternCharIndex(ch);
    if (f == UDAT_FIELD_COUNT) {
        
        return FALSE;
    }
    int32_t i = patternOffset;
    while (pattern.charAt(++i) == ch) {}
    return DateFormatSymbols::isNumericField(f, i - patternOffset);
}

UBool
SimpleDateFormat::isAfterNonNumericField(const UnicodeString &pattern, int32_t patternOffset) {
    if (patternOffset <= 0) {
        
        return FALSE;
    }
    UChar ch = pattern.charAt(--patternOffset);
    UDateFormatField f = DateFormatSymbols::getPatternCharIndex(ch);
    if (f == UDAT_FIELD_COUNT) {
        
        return FALSE;
    }
    int32_t i = patternOffset;
    while (pattern.charAt(--i) == ch) {}
    return !DateFormatSymbols::isNumericField(f, patternOffset - i);
}

void
SimpleDateFormat::parse(const UnicodeString& text, Calendar& cal, ParsePosition& parsePos) const
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t pos = parsePos.getIndex();
    int32_t start = pos;

    UBool ambiguousYear[] = { FALSE };
    int32_t saveHebrewMonth = -1;
    int32_t count = 0;

    UBool lenient = isLenient();

    
    ((SimpleDateFormat*)this)->tztype = UTZFMT_TIME_TYPE_UNKNOWN;

    
    
    
    
    
    int32_t abutPat = -1; 
    int32_t abutStart = 0;
    int32_t abutPass = 0;
    UBool inQuote = FALSE;

    MessageFormat * numericLeapMonthFormatter = NULL;

    Calendar* calClone = NULL;
    Calendar *workCal = &cal;
    if (&cal != fCalendar && uprv_strcmp(cal.getType(), fCalendar->getType()) != 0) {
        
        
        
        calClone = fCalendar->clone();
        if (calClone != NULL) {
            calClone->setTime(cal.getTime(status),status);
            if (U_FAILURE(status)) {
                goto ExitParse;
            }
            calClone->setTimeZone(cal.getTimeZone());
            workCal = calClone;
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto ExitParse;
        }
    }
    
    if (fSymbols->fLeapMonthPatterns != NULL && fSymbols->fLeapMonthPatternsCount >= DateFormatSymbols::kMonthPatternsCount) {
        numericLeapMonthFormatter = new MessageFormat(fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternNumeric], fLocale, status);
        if (numericLeapMonthFormatter == NULL) {
             status = U_MEMORY_ALLOCATION_ERROR;
             goto ExitParse;
        } else if (U_FAILURE(status)) {
             goto ExitParse; 
        }
    }

    for (int32_t i=0; i<fPattern.length(); ++i) {
        UChar ch = fPattern.charAt(i);

        
        if (!inQuote && ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A))) { 
            int32_t fieldPat = i;

            
            count = 1;
            while ((i+1)<fPattern.length() &&
                   fPattern.charAt(i+1) == ch) {
                ++count;
                ++i;
            }

            if (isNumeric(ch, count)) {
                if (abutPat < 0) {
                    
                    
                    if (isAtNumericField(fPattern, i + 1)) {
                        abutPat = fieldPat;
                        abutStart = pos;
                        abutPass = 0;
                    }
                }
            } else {
                abutPat = -1; 
            }

            
            
            
            
            
            
            
            if (abutPat >= 0) {
                
                
                
                
                if (fieldPat == abutPat) {
                    count -= abutPass++;
                    if (count == 0) {
                        status = U_PARSE_ERROR;
                        goto ExitParse;
                    }
                }

                pos = subParse(text, pos, ch, count,
                               TRUE, FALSE, ambiguousYear, saveHebrewMonth, *workCal, i, numericLeapMonthFormatter);

                
                
                if (pos < 0) {
                    i = abutPat - 1;
                    pos = abutStart;
                    continue;
                }
            }

            
            
            else if (ch != 0x6C) { 
                int32_t s = subParse(text, pos, ch, count,
                               FALSE, TRUE, ambiguousYear, saveHebrewMonth, *workCal, i, numericLeapMonthFormatter);

                if (s == -pos-1) {
                    
                    
                    s = pos;

                    if (i+1 < fPattern.length()) {
                        
                        UChar ch = fPattern.charAt(i+1);

                        
                        if (PatternProps::isWhiteSpace(ch)) {
                            i++;
                            
                            while ((i+1)<fPattern.length() &&
                                   PatternProps::isWhiteSpace(fPattern.charAt(i+1))) {
                                ++i;
                            }
                        }
                    }
                }
                else if (s <= 0) {
                    status = U_PARSE_ERROR;
                    goto ExitParse;
                }
                pos = s;
            }
        }

        
        
        
        else {

            abutPat = -1; 
            
            if (! matchLiterals(fPattern, i, text, pos, lenient)) {
                status = U_PARSE_ERROR;
                goto ExitParse;
            }
        }
    }

    
    if (text.charAt(pos) == 0x2e && lenient) {
        
        if (isAfterNonNumericField(fPattern, fPattern.length())) {
            pos++; 
        }
    }

    
    
    

    parsePos.setIndex(pos);

    
    
    
    
    
    
    
    
    






    
    
    
    
    
    
    
    if (ambiguousYear[0] || tztype != UTZFMT_TIME_TYPE_UNKNOWN) 
    {
        
        
        
        
        Calendar *copy;
        if (ambiguousYear[0]) {
            copy = cal.clone();
            
            if (copy == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto ExitParse;
            }
            UDate parsedDate = copy->getTime(status);
            
            if (fHaveDefaultCentury && (parsedDate < fDefaultCenturyStart)) {
                
                cal.set(UCAL_YEAR, fDefaultCenturyStartYear + 100);
            }
            delete copy;
        }

        if (tztype != UTZFMT_TIME_TYPE_UNKNOWN) {
            copy = cal.clone();
            
            if (copy == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto ExitParse;
            }
            const TimeZone & tz = cal.getTimeZone();
            BasicTimeZone *btz = NULL;

            if (dynamic_cast<const OlsonTimeZone *>(&tz) != NULL
                || dynamic_cast<const SimpleTimeZone *>(&tz) != NULL
                || dynamic_cast<const RuleBasedTimeZone *>(&tz) != NULL
                || dynamic_cast<const VTimeZone *>(&tz) != NULL) {
                btz = (BasicTimeZone*)&tz;
            }

            
            copy->set(UCAL_ZONE_OFFSET, 0);
            copy->set(UCAL_DST_OFFSET, 0);
            UDate localMillis = copy->getTime(status);

            
            
            int32_t raw, dst;
            if (btz != NULL) {
                if (tztype == UTZFMT_TIME_TYPE_STANDARD) {
                    btz->getOffsetFromLocal(localMillis,
                        BasicTimeZone::kStandard, BasicTimeZone::kStandard, raw, dst, status);
                } else {
                    btz->getOffsetFromLocal(localMillis,
                        BasicTimeZone::kDaylight, BasicTimeZone::kDaylight, raw, dst, status);
                }
            } else {
                
                
                tz.getOffset(localMillis, TRUE, raw, dst, status);
            }

            
            int32_t resolvedSavings = dst;
            if (tztype == UTZFMT_TIME_TYPE_STANDARD) {
                if (dst != 0) {
                    
                    resolvedSavings = 0;
                }
            } else { 
                if (dst == 0) {
                    if (btz != NULL) {
                        UDate time = localMillis + raw;
                        
                        TimeZoneTransition beforeTrs, afterTrs;
                        UDate beforeT = time, afterT = time;
                        int32_t beforeSav = 0, afterSav = 0;
                        UBool beforeTrsAvail, afterTrsAvail;

                        
                        while (TRUE) {
                            beforeTrsAvail = btz->getPreviousTransition(beforeT, TRUE, beforeTrs);
                            if (!beforeTrsAvail) {
                                break;
                            }
                            beforeT = beforeTrs.getTime() - 1;
                            beforeSav = beforeTrs.getFrom()->getDSTSavings();
                            if (beforeSav != 0) {
                                break;
                            }
                        }

                        
                        while (TRUE) {
                            afterTrsAvail = btz->getNextTransition(afterT, FALSE, afterTrs);
                            if (!afterTrsAvail) {
                                break;
                            }
                            afterT = afterTrs.getTime();
                            afterSav = afterTrs.getTo()->getDSTSavings();
                            if (afterSav != 0) {
                                break;
                            }
                        }

                        if (beforeTrsAvail && afterTrsAvail) {
                            if (time - beforeT > afterT - time) {
                                resolvedSavings = afterSav;
                            } else {
                                resolvedSavings = beforeSav;
                            }
                        } else if (beforeTrsAvail && beforeSav != 0) {
                            resolvedSavings = beforeSav;
                        } else if (afterTrsAvail && afterSav != 0) {
                            resolvedSavings = afterSav;
                        } else {
                            resolvedSavings = btz->getDSTSavings();
                        }
                    } else {
                        resolvedSavings = tz.getDSTSavings();
                    }
                    if (resolvedSavings == 0) {
                        
                        resolvedSavings = U_MILLIS_PER_HOUR;
                    }
                }
            }
            cal.set(UCAL_ZONE_OFFSET, raw);
            cal.set(UCAL_DST_OFFSET, resolvedSavings);
            delete copy;
        }
    }
ExitParse:
    
    
    if (U_SUCCESS(status) && workCal != &cal) {
        cal.setTimeZone(workCal->getTimeZone());
        cal.setTime(workCal->getTime(status), status);
    }

    if (numericLeapMonthFormatter != NULL) {
        delete numericLeapMonthFormatter;
    }
    if (calClone != NULL) {
        delete calClone;
    }

    
    
    
    if (U_FAILURE(status)) {
        parsePos.setErrorIndex(pos);
        parsePos.setIndex(start);
    }
}

UDate
SimpleDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    
    
    return DateFormat::parse(text, pos);
}

UDate
SimpleDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    
    
    return DateFormat::parse(text, status);
}


static UBool
newBestMatchWithOptionalDot(const UnicodeString &lcaseText,
                            const UnicodeString &data,
                            UnicodeString &bestMatchName,
                            int32_t &bestMatchLength);

int32_t SimpleDateFormat::matchQuarterString(const UnicodeString& text,
                              int32_t start,
                              UCalendarDateFields field,
                              const UnicodeString* data,
                              int32_t dataCount,
                              Calendar& cal) const
{
    int32_t i = 0;
    int32_t count = dataCount;

    
    
    
    
    int32_t bestMatchLength = 0, bestMatch = -1;
    UnicodeString bestMatchName;

    
    
    
    
    
    
    UnicodeString lcaseText;
    text.extract(start, INT32_MAX, lcaseText);
    lcaseText.foldCase();

    for (; i < count; ++i)
    {
        
        

        if (newBestMatchWithOptionalDot(lcaseText, data[i], bestMatchName, bestMatchLength)) {
            bestMatch = i;
        }
    }
    if (bestMatch >= 0)
    {
        cal.set(field, bestMatch * 3);

        
        
        

        
        
        
        
        
        
        int32_t len = bestMatchName.length(); 
        int32_t n = text.length() - start;
        for (i=0; i<=n; ++i) {
            int32_t j=i;
            if (i == 0) {
                j = len;
            } else if (i == len) {
                continue; 
            }
            text.extract(start, j, lcaseText);
            lcaseText.foldCase();
            if (bestMatchName == lcaseText) {
                return start + j;
            }
        }
    }

    return -start;
}


UBool SimpleDateFormat::matchLiterals(const UnicodeString &pattern,
                                      int32_t &patternOffset,
                                      const UnicodeString &text,
                                      int32_t &textOffset,
                                      UBool lenient)
{
    UBool inQuote = FALSE;
    UnicodeString literal;
    int32_t i = patternOffset;
    
    
    for ( ; i < pattern.length(); i += 1) {
        UChar ch = pattern.charAt(i);
        
        if (!inQuote && ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A))) { 
            break;
        }
        
        if (ch == QUOTE) {
            
            if ((i + 1) < pattern.length() && pattern.charAt(i + 1) == QUOTE) {
                i += 1;
            } else {
                inQuote = !inQuote;
                continue;
            }
        }
        
        literal += ch;
    }
    
    
    
    int32_t p;
    int32_t t = textOffset;
    
    if (lenient) {
        
        
        literal.trim();
        
        
        while (t < text.length() && u_isWhitespace(text.charAt(t))) {
            t += 1;
        }
    }
        
    for (p = 0; p < literal.length() && t < text.length();) {
        UBool needWhitespace = FALSE;
        
        while (p < literal.length() && PatternProps::isWhiteSpace(literal.charAt(p))) {
            needWhitespace = TRUE;
            p += 1;
        }
        
        if (needWhitespace) {
            int32_t tStart = t;
            
            while (t < text.length()) {
                UChar tch = text.charAt(t);
                
                if (!u_isUWhiteSpace(tch) && !PatternProps::isWhiteSpace(tch)) {
                    break;
                }
                
                t += 1;
            }
            
            
            
            
            if (!lenient && t == tStart) {
                
                
                return FALSE;
            }
            
            
            
            if (p >= literal.length()) {
                break;
            }
        }
        
        if (t >= text.length() || literal.charAt(p) != text.charAt(t)) {
            
            
            if (lenient) {
                if (t == textOffset && text.charAt(t) == 0x2e &&
                        isAfterNonNumericField(pattern, patternOffset)) {
                    
                    
                    ++t;
                    continue;  
                }
                break;
            }
            
            return FALSE;
        }
        ++p;
        ++t;
    }
    
    
    
    
    if (p <= 0) {
        
        
        const  UnicodeSet *ignorables = NULL;
        UDateFormatField patternCharIndex = DateFormatSymbols::getPatternCharIndex(pattern.charAt(i));
        if (patternCharIndex != UDAT_FIELD_COUNT) {
            ignorables = SimpleDateFormatStaticSets::getIgnorables(patternCharIndex);
        }
        
        for (t = textOffset; t < text.length(); t += 1) {
            UChar ch = text.charAt(t);
            
            if (ignorables == NULL || !ignorables->contains(ch)) {
                break;
            }
        }
    }
    
    
    patternOffset = i - 1;
    textOffset = t;
    
    return TRUE;
}



int32_t SimpleDateFormat::matchString(const UnicodeString& text,
                              int32_t start,
                              UCalendarDateFields field,
                              const UnicodeString* data,
                              int32_t dataCount,
                              const UnicodeString* monthPattern,
                              Calendar& cal) const
{
    int32_t i = 0;
    int32_t count = dataCount;

    if (field == UCAL_DAY_OF_WEEK) i = 1;

    
    
    
    
    int32_t bestMatchLength = 0, bestMatch = -1;
    UnicodeString bestMatchName;
    int32_t isLeapMonth = 0;

    
    
    
    
    
    
    UnicodeString lcaseText;
    text.extract(start, INT32_MAX, lcaseText);
    lcaseText.foldCase();

    for (; i < count; ++i)
    {
        
        

        if (newBestMatchWithOptionalDot(lcaseText, data[i], bestMatchName, bestMatchLength)) {
            bestMatch = i;
            isLeapMonth = 0;
        }

        if (monthPattern != NULL) {
            UErrorCode status = U_ZERO_ERROR;
            UnicodeString leapMonthName;
            Formattable monthName((const UnicodeString&)(data[i]));
            MessageFormat::format(*monthPattern, &monthName, 1, leapMonthName, status);
            if (U_SUCCESS(status)) {
                if (newBestMatchWithOptionalDot(lcaseText, leapMonthName, bestMatchName, bestMatchLength)) {
                    bestMatch = i;
                    isLeapMonth = 1;
                }
            }
        }
    }
    if (bestMatch >= 0)
    {
        
        if (!strcmp(cal.getType(),"hebrew") && field==UCAL_MONTH && bestMatch==13) {
            cal.set(field,6);
        }
        else {
            if (field == UCAL_YEAR) {
                bestMatch++; 
            }
            cal.set(field, bestMatch);
        }
        if (monthPattern != NULL) {
            cal.set(UCAL_IS_LEAP_MONTH, isLeapMonth);
        }

        
        
        

        
        
        
        
        
        
        int32_t len = bestMatchName.length(); 
        int32_t n = text.length() - start;
        for (i=0; i<=n; ++i) {
            int32_t j=i;
            if (i == 0) {
                j = len;
            } else if (i == len) {
                continue; 
            }
            text.extract(start, j, lcaseText);
            lcaseText.foldCase();
            if (bestMatchName == lcaseText) {
                return start + j;
            }
        }
    }

    return -start;
}

static UBool
newBestMatchWithOptionalDot(const UnicodeString &lcaseText,
                            const UnicodeString &data,
                            UnicodeString &bestMatchName,
                            int32_t &bestMatchLength) {
    UnicodeString lcase;
    lcase.fastCopyFrom(data).foldCase();
    int32_t length = lcase.length();
    if (length <= bestMatchLength) {
        
        return FALSE;
    }

    if (lcaseText.compareBetween(0, length, lcase, 0, length) == 0) {
        
        bestMatchName = lcase;
        bestMatchLength = length;
        return TRUE;
    }
    if (lcase.charAt(--length) == 0x2e) {
        if (lcaseText.compareBetween(0, length, lcase, 0, length) == 0) {
            
            bestMatchName = lcase;
            bestMatchName.truncate(length);
            bestMatchLength = length;
            return TRUE;
        }
    }
    return FALSE;
}



void
SimpleDateFormat::set2DigitYearStart(UDate d, UErrorCode& status)
{
    parseAmbiguousDatesAsAfter(d, status);
}











int32_t SimpleDateFormat::subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                           UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], int32_t& saveHebrewMonth, Calendar& cal,
                           int32_t patLoc, MessageFormat * numericLeapMonthFormatter) const
{
    Formattable number;
    int32_t value = 0;
    int32_t i;
    int32_t ps = 0;
    ParsePosition pos(0);
    UDateFormatField patternCharIndex = DateFormatSymbols::getPatternCharIndex(ch);
    NumberFormat *currentNumberFormat;
    UnicodeString temp;
    UBool lenient = isLenient();
    UBool gotNumber = FALSE;

#if defined (U_DEBUG_CAL)
    
#endif

    if (patternCharIndex == UDAT_FIELD_COUNT) {
        return -start;
    }

    currentNumberFormat = getNumberFormatByIndex(patternCharIndex);
    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];
    UnicodeString hebr("hebr", 4, US_INV);

    if (numericLeapMonthFormatter != NULL) {
        numericLeapMonthFormatter->setFormats((const Format **)&currentNumberFormat, 1);
    }
    UBool isChineseCalendar = (uprv_strcmp(cal.getType(),"chinese") == 0);

    
    
    for (;;) {
        if (start >= text.length()) {
            return -start;
        }
        UChar32 c = text.char32At(start);
        if (!u_isUWhiteSpace(c)  && !PatternProps::isWhiteSpace(c)) {
            break;
        }
        start += U16_LENGTH(c);
    }
    pos.setIndex(start);

    
    
    
    
    if (patternCharIndex == UDAT_HOUR_OF_DAY1_FIELD ||                       
        patternCharIndex == UDAT_HOUR_OF_DAY0_FIELD ||                       
        patternCharIndex == UDAT_HOUR1_FIELD ||                              
        patternCharIndex == UDAT_HOUR0_FIELD ||                              
        (patternCharIndex == UDAT_DOW_LOCAL_FIELD && count <= 2) ||          
        (patternCharIndex == UDAT_STANDALONE_DAY_FIELD && count <= 2) ||     
        (patternCharIndex == UDAT_MONTH_FIELD && count <= 2) ||              
        (patternCharIndex == UDAT_STANDALONE_MONTH_FIELD && count <= 2) ||   
        (patternCharIndex == UDAT_QUARTER_FIELD && count <= 2) ||            
        (patternCharIndex == UDAT_STANDALONE_QUARTER_FIELD && count <= 2) || 
        patternCharIndex == UDAT_YEAR_FIELD ||                               
        patternCharIndex == UDAT_YEAR_WOY_FIELD ||                           
        patternCharIndex == UDAT_YEAR_NAME_FIELD ||                          
        (patternCharIndex == UDAT_ERA_FIELD && isChineseCalendar) ||         
        patternCharIndex == UDAT_FRACTIONAL_SECOND_FIELD)                    
    {
        int32_t parseStart = pos.getIndex();
        
        
        const UnicodeString* src;

        UBool parsedNumericLeapMonth = FALSE;
        if (numericLeapMonthFormatter != NULL && (patternCharIndex == UDAT_MONTH_FIELD || patternCharIndex == UDAT_STANDALONE_MONTH_FIELD)) {
            int32_t argCount;
            Formattable * args = numericLeapMonthFormatter->parse(text, pos, argCount);
            if (args != NULL && argCount == 1 && pos.getIndex() > parseStart && args[0].isNumeric()) {
                parsedNumericLeapMonth = TRUE;
                number.setLong(args[0].getLong());
                cal.set(UCAL_IS_LEAP_MONTH, 1);
                delete[] args;
            } else {
                pos.setIndex(parseStart);
                cal.set(UCAL_IS_LEAP_MONTH, 0);
            }
        }

        if (!parsedNumericLeapMonth) {
            if (obeyCount) {
                if ((start+count) > text.length()) {
                    return -start;
                }

                text.extractBetween(0, start + count, temp);
                src = &temp;
            } else {
                src = &text;
            }

            parseInt(*src, number, pos, allowNegative,currentNumberFormat);
        }

        int32_t txtLoc = pos.getIndex();

        if (txtLoc > parseStart) {
            value = number.getLong();
            gotNumber = TRUE;
            
            
            if (value < 0 ) {
                txtLoc = checkIntSuffix(text, txtLoc, patLoc+1, TRUE);
                if (txtLoc != pos.getIndex()) {
                    value *= -1;
                }
            }
            else {
                txtLoc = checkIntSuffix(text, txtLoc, patLoc+1, FALSE);
            }

            if (!lenient) {
                
                int32_t bias = gFieldRangeBias[patternCharIndex];
                if (bias >= 0 && (value > cal.getMaximum(field) + bias || value < cal.getMinimum(field) + bias)) {
                    return -start;
                }
            }

            pos.setIndex(txtLoc);
        }
    }
    
    
    
    
    switch (patternCharIndex) {
        case UDAT_HOUR_OF_DAY1_FIELD:
        case UDAT_HOUR_OF_DAY0_FIELD:
        case UDAT_HOUR1_FIELD:
        case UDAT_HOUR0_FIELD:
            
            if (value < 0 || value > 24) {
                return -start;
            }
            
            
            
        case UDAT_YEAR_FIELD:
        case UDAT_YEAR_WOY_FIELD:
        case UDAT_FRACTIONAL_SECOND_FIELD:
            
            if (! gotNumber) {
                return -start;
            }
            
            break;
            
        default:
            
            break;
    }

    switch (patternCharIndex) {
    case UDAT_ERA_FIELD:
        if (isChineseCalendar) {
            if (!gotNumber) {
                return -start;
            }
            cal.set(UCAL_ERA, value);
            return pos.getIndex();
        }
        if (count == 5) {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fNarrowEras, fSymbols->fNarrowErasCount, NULL, cal);
        } else if (count == 4) {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fEraNames, fSymbols->fEraNamesCount, NULL, cal);
        } else {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fEras, fSymbols->fErasCount, NULL, cal);
        }

        
        
        
        if (ps == -start)
            ps--;

        return ps;

    case UDAT_YEAR_FIELD:
        
        
        
        
        
        
        if (fDateOverride.compare(hebr)==0 && value < 1000) {
            value += HEBREW_CAL_CUR_MILLENIUM_START_YEAR;
        } else if ((pos.getIndex() - start) == 2 && !isChineseCalendar
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1)))
        {
            
            
            
            
            
            
            
            
          if(fHaveDefaultCentury) { 
              int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
              ambiguousYear[0] = (value == ambiguousTwoDigitYear);
              value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
            }
        }
        cal.set(UCAL_YEAR, value);

        
        if (saveHebrewMonth >= 0) {
            HebrewCalendar *hc = (HebrewCalendar*)&cal;
            if (!hc->isLeapYear(value) && saveHebrewMonth >= 6) {
               cal.set(UCAL_MONTH,saveHebrewMonth);
            } else {
               cal.set(UCAL_MONTH,saveHebrewMonth-1);
            }
            saveHebrewMonth = -1;
        }
        return pos.getIndex();

    case UDAT_YEAR_WOY_FIELD:
        
        if (fDateOverride.compare(hebr)==0 && value < 1000) {
            value += HEBREW_CAL_CUR_MILLENIUM_START_YEAR;
        } else if ((pos.getIndex() - start) == 2
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1))
            && fHaveDefaultCentury )
        {
            int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
            ambiguousYear[0] = (value == ambiguousTwoDigitYear);
            value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
        }
        cal.set(UCAL_YEAR_WOY, value);
        return pos.getIndex();

    case UDAT_YEAR_NAME_FIELD:
        if (fSymbols->fShortYearNames != NULL) {
            int32_t newStart = matchString(text, start, UCAL_YEAR, fSymbols->fShortYearNames, fSymbols->fShortYearNamesCount, NULL, cal);
            if (newStart > 0) {
                return newStart;
            }
        }
        if (gotNumber && (lenient || value > fSymbols->fShortYearNamesCount)) {
            cal.set(UCAL_YEAR, value);
            return pos.getIndex();
        }
        return -start;

    case UDAT_MONTH_FIELD:
    case UDAT_STANDALONE_MONTH_FIELD:
        if (gotNumber) 
        {
            
            
            
            if (!strcmp(cal.getType(),"hebrew")) {
                HebrewCalendar *hc = (HebrewCalendar*)&cal;
                if (cal.isSet(UCAL_YEAR)) {
                   UErrorCode status = U_ZERO_ERROR;
                   if (!hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value >= 6) {
                       cal.set(UCAL_MONTH, value);
                   } else {
                       cal.set(UCAL_MONTH, value - 1);
                   }
                } else {
                    saveHebrewMonth = value;
                }
            } else {
                
                
                
                cal.set(UCAL_MONTH, value - 1);
            }
            return pos.getIndex();
        } else {
            
            
            
            UnicodeString * wideMonthPat = NULL;
            UnicodeString * shortMonthPat = NULL;
            if (fSymbols->fLeapMonthPatterns != NULL && fSymbols->fLeapMonthPatternsCount >= DateFormatSymbols::kMonthPatternsCount) {
                if (patternCharIndex==UDAT_MONTH_FIELD) {
                    wideMonthPat = &fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternFormatWide];
                    shortMonthPat = &fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternFormatAbbrev];
                } else {
                    wideMonthPat = &fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternStandaloneWide];
                    shortMonthPat = &fSymbols->fLeapMonthPatterns[DateFormatSymbols::kLeapMonthPatternStandaloneAbbrev];
                }
            }
            int32_t newStart = 0;
            if (patternCharIndex==UDAT_MONTH_FIELD) {
                newStart = matchString(text, start, UCAL_MONTH, fSymbols->fMonths, fSymbols->fMonthsCount, wideMonthPat, cal); 
                if (newStart > 0) {
                    return newStart;
                }
                newStart = matchString(text, start, UCAL_MONTH, fSymbols->fShortMonths, fSymbols->fShortMonthsCount, shortMonthPat, cal); 
            } else {
                newStart = matchString(text, start, UCAL_MONTH, fSymbols->fStandaloneMonths, fSymbols->fStandaloneMonthsCount, wideMonthPat, cal); 
                if (newStart > 0) {
                    return newStart;
                }
                newStart = matchString(text, start, UCAL_MONTH, fSymbols->fStandaloneShortMonths, fSymbols->fStandaloneShortMonthsCount, shortMonthPat, cal); 
            }
            if (newStart > 0 || !lenient)  
                return newStart;
            
        }
        break;

    case UDAT_HOUR_OF_DAY1_FIELD:
        
        if (value == cal.getMaximum(UCAL_HOUR_OF_DAY) + 1)
            value = 0;
            
        
            
    case UDAT_HOUR_OF_DAY0_FIELD:
        cal.set(UCAL_HOUR_OF_DAY, value);
        return pos.getIndex();

    case UDAT_FRACTIONAL_SECOND_FIELD:
        
        i = pos.getIndex() - start;
        if (i < 3) {
            while (i < 3) {
                value *= 10;
                i++;
            }
        } else {
            int32_t a = 1;
            while (i > 3) {
                a *= 10;
                i--;
            }
            value = (value + (a>>1)) / a;
        }
        cal.set(UCAL_MILLISECOND, value);
        return pos.getIndex();

    case UDAT_DOW_LOCAL_FIELD:
        if (gotNumber) 
        {
            
            cal.set(UCAL_DOW_LOCAL, value);
            return pos.getIndex();
        }
        
        
    case UDAT_DAY_OF_WEEK_FIELD:
        {
            
            
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                      fSymbols->fWeekdays, fSymbols->fWeekdaysCount, NULL, cal)) > 0)
                return newStart;
            
            else if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fShortWeekdays, fSymbols->fShortWeekdaysCount, NULL, cal)) > 0)
                return newStart;
            
            else if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fNarrowWeekdays, fSymbols->fNarrowWeekdaysCount, NULL, cal)) > 0)
                return newStart;
            else if (!lenient || patternCharIndex == UDAT_DAY_OF_WEEK_FIELD)
                return newStart;
            
        }
        break;

    case UDAT_STANDALONE_DAY_FIELD:
        {
            if (gotNumber) 
            {
                
                cal.set(UCAL_DOW_LOCAL, value);
                return pos.getIndex();
            }
            
            
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                      fSymbols->fStandaloneWeekdays, fSymbols->fStandaloneWeekdaysCount, NULL, cal)) > 0)
                return newStart;
            else if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                          fSymbols->fStandaloneShortWeekdays, fSymbols->fStandaloneShortWeekdaysCount, NULL, cal)) > 0)
                return newStart;
            else if (!lenient)
                return newStart;
            
        }
        break;

    case UDAT_AM_PM_FIELD:
        return matchString(text, start, UCAL_AM_PM, fSymbols->fAmPms, fSymbols->fAmPmsCount, NULL, cal);

    case UDAT_HOUR1_FIELD:
        
        if (value == cal.getLeastMaximum(UCAL_HOUR)+1)
            value = 0;
            
        
            
    case UDAT_HOUR0_FIELD:
        cal.set(UCAL_HOUR, value);
        return pos.getIndex();

    case UDAT_QUARTER_FIELD:
        if (gotNumber) 
        {
            
            
            
            cal.set(UCAL_MONTH, (value - 1) * 3);
            return pos.getIndex();
        } else {
            
            
            
            int32_t newStart = 0;

            if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                      fSymbols->fQuarters, fSymbols->fQuartersCount, cal)) > 0)
                return newStart;
            else if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                          fSymbols->fShortQuarters, fSymbols->fShortQuartersCount, cal)) > 0)
                return newStart;
            else if (!lenient)
                return newStart;
            
        }
        break;

    case UDAT_STANDALONE_QUARTER_FIELD:
        if (gotNumber) 
        {
            
            
            
            cal.set(UCAL_MONTH, (value - 1) * 3);
            return pos.getIndex();
        } else {
            
            
            
            int32_t newStart = 0;

            if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                      fSymbols->fStandaloneQuarters, fSymbols->fStandaloneQuartersCount, cal)) > 0)
                return newStart;
            else if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                          fSymbols->fStandaloneShortQuarters, fSymbols->fStandaloneShortQuartersCount, cal)) > 0)
                return newStart;
            else if (!lenient)
                return newStart;
            
        }
        break;

    case UDAT_TIMEZONE_FIELD:
        {
            UTimeZoneFormatTimeType tzTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
            UTimeZoneFormatStyle style = (count < 4) ? UTZFMT_STYLE_SPECIFIC_SHORT : UTZFMT_STYLE_SPECIFIC_LONG;
            TimeZone *tz  = tzFormat()->parse(style, text, pos, &tzTimeType);
            if (tz != NULL) {
                ((SimpleDateFormat*)this)->tztype = tzTimeType;
                cal.adoptTimeZone(tz);
                return pos.getIndex();
            }
        }
        break;
    case UDAT_TIMEZONE_RFC_FIELD:
        {
            UTimeZoneFormatTimeType tzTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
            UTimeZoneFormatStyle style = (count < 4) ? UTZFMT_STYLE_RFC822 : ((count == 5) ? UTZFMT_STYLE_ISO8601: UTZFMT_STYLE_LOCALIZED_GMT);
            TimeZone *tz  = tzFormat()->parse(style, text, pos, &tzTimeType);
            if (tz != NULL) {
                ((SimpleDateFormat*)this)->tztype = tzTimeType;
                cal.adoptTimeZone(tz);
                return pos.getIndex();
            }
            return -start;
        }
    case UDAT_TIMEZONE_GENERIC_FIELD:
        {
            UTimeZoneFormatTimeType tzTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
            UTimeZoneFormatStyle style = (count < 4) ? UTZFMT_STYLE_GENERIC_SHORT : UTZFMT_STYLE_GENERIC_LONG;
            TimeZone *tz  = tzFormat()->parse(style, text, pos, &tzTimeType);
            if (tz != NULL) {
                ((SimpleDateFormat*)this)->tztype = tzTimeType;
                cal.adoptTimeZone(tz);
                return pos.getIndex();
            }
            return -start;
        }
    case UDAT_TIMEZONE_SPECIAL_FIELD:
        {
            UTimeZoneFormatTimeType tzTimeType = UTZFMT_TIME_TYPE_UNKNOWN;
            UTimeZoneFormatStyle style = (count < 4) ? UTZFMT_STYLE_SPECIFIC_SHORT : UTZFMT_STYLE_GENERIC_LOCATION;
            TimeZone *tz  = tzFormat()->parse(style, text, pos, &tzTimeType);
            if (tz != NULL) {
                ((SimpleDateFormat*)this)->tztype = tzTimeType;
                cal.adoptTimeZone(tz);
                return pos.getIndex();
            }
            return -start;
        }

    default:
        
        
        break;
    }
    
    
    

    int32_t parseStart = pos.getIndex();
    const UnicodeString* src;
    if (obeyCount) {
        if ((start+count) > text.length()) {
            return -start;
        }
        text.extractBetween(0, start + count, temp);
        src = &temp;
    } else {
        src = &text;
    }
    parseInt(*src, number, pos, allowNegative,currentNumberFormat);
    if (pos.getIndex() != parseStart) {
        int32_t value = number.getLong();

        
        

        if (!lenient) {
            
            int32_t bias = gFieldRangeBias[patternCharIndex];
            if (bias >= 0 && (value > cal.getMaximum(field) + bias || value < cal.getMinimum(field) + bias)) {
                return -start;
            }
        }

        
        
        
        switch (patternCharIndex) {
        case UDAT_MONTH_FIELD:
            
            if (!strcmp(cal.getType(),"hebrew")) {
                HebrewCalendar *hc = (HebrewCalendar*)&cal;
                if (cal.isSet(UCAL_YEAR)) {
                   UErrorCode status = U_ZERO_ERROR;
                   if (!hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value >= 6) {
                       cal.set(UCAL_MONTH, value);
                   } else {
                       cal.set(UCAL_MONTH, value - 1);
                   }
                } else {
                    saveHebrewMonth = value;
                }
            } else {
                cal.set(UCAL_MONTH, value - 1);
            }
            break;
        case UDAT_STANDALONE_MONTH_FIELD:
            cal.set(UCAL_MONTH, value - 1);
            break;
        case UDAT_DOW_LOCAL_FIELD:
        case UDAT_STANDALONE_DAY_FIELD:
            cal.set(UCAL_DOW_LOCAL, value);
            break;
        case UDAT_QUARTER_FIELD:
        case UDAT_STANDALONE_QUARTER_FIELD:
             cal.set(UCAL_MONTH, (value - 1) * 3);
             break;
        default:
            cal.set(field, value);
            break;
        }
        return pos.getIndex();
    }
    return -start;
}





void SimpleDateFormat::parseInt(const UnicodeString& text,
                                Formattable& number,
                                ParsePosition& pos,
                                UBool allowNegative,
                                NumberFormat *fmt) const {
    parseInt(text, number, -1, pos, allowNegative,fmt);
}




void SimpleDateFormat::parseInt(const UnicodeString& text,
                                Formattable& number,
                                int32_t maxDigits,
                                ParsePosition& pos,
                                UBool allowNegative,
                                NumberFormat *fmt) const {
    UnicodeString oldPrefix;
    DecimalFormat* df = NULL;
    if (!allowNegative && (df = dynamic_cast<DecimalFormat*>(fmt)) != NULL) {
        df->getNegativePrefix(oldPrefix);
        df->setNegativePrefix(UnicodeString(TRUE, SUPPRESS_NEGATIVE_PREFIX, -1));
    }
    int32_t oldPos = pos.getIndex();
    fmt->parse(text, number, pos);
    if (df != NULL) {
        df->setNegativePrefix(oldPrefix);
    }

    if (maxDigits > 0) {
        
        
        int32_t nDigits = pos.getIndex() - oldPos;
        if (nDigits > maxDigits) {
            int32_t val = number.getLong();
            nDigits -= maxDigits;
            while (nDigits > 0) {
                val /= 10;
                nDigits--;
            }
            pos.setIndex(oldPos + maxDigits);
            number.setLong(val);
        }
    }
}



void SimpleDateFormat::translatePattern(const UnicodeString& originalPattern,
                                        UnicodeString& translatedPattern,
                                        const UnicodeString& from,
                                        const UnicodeString& to,
                                        UErrorCode& status)
{
  
  
  
  
  
  
  if (U_FAILURE(status))
    return;

  translatedPattern.remove();
  UBool inQuote = FALSE;
  for (int32_t i = 0; i < originalPattern.length(); ++i) {
    UChar c = originalPattern[i];
    if (inQuote) {
      if (c == QUOTE)
    inQuote = FALSE;
    }
    else {
      if (c == QUOTE)
    inQuote = TRUE;
      else if ((c >= 0x0061  && c <= 0x007A) 
           || (c >= 0x0041  && c <= 0x005A )) {
    int32_t ci = from.indexOf(c);
    if (ci == -1) {
      status = U_INVALID_FORMAT_ERROR;
      return;
    }
    c = to[ci];
      }
    }
    translatedPattern += c;
  }
  if (inQuote) {
    status = U_INVALID_FORMAT_ERROR;
    return;
  }
}



UnicodeString&
SimpleDateFormat::toPattern(UnicodeString& result) const
{
    result = fPattern;
    return result;
}



UnicodeString&
SimpleDateFormat::toLocalizedPattern(UnicodeString& result,
                                     UErrorCode& status) const
{
    translatePattern(fPattern, result,
                     UnicodeString(DateFormatSymbols::getPatternUChars()),
                     fSymbols->fLocalPatternChars, status);
    return result;
}



void
SimpleDateFormat::applyPattern(const UnicodeString& pattern)
{
    fPattern = pattern;
}



void
SimpleDateFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                        UErrorCode &status)
{
    translatePattern(pattern, fPattern,
                     fSymbols->fLocalPatternChars,
                     UnicodeString(DateFormatSymbols::getPatternUChars()), status);
}



const DateFormatSymbols*
SimpleDateFormat::getDateFormatSymbols() const
{
    return fSymbols;
}



void
SimpleDateFormat::adoptDateFormatSymbols(DateFormatSymbols* newFormatSymbols)
{
    delete fSymbols;
    fSymbols = newFormatSymbols;
}


void
SimpleDateFormat::setDateFormatSymbols(const DateFormatSymbols& newFormatSymbols)
{
    delete fSymbols;
    fSymbols = new DateFormatSymbols(newFormatSymbols);
}


const TimeZoneFormat*
SimpleDateFormat::getTimeZoneFormat(void) const {
    return (const TimeZoneFormat*)tzFormat();
}


void
SimpleDateFormat::adoptTimeZoneFormat(TimeZoneFormat* timeZoneFormatToAdopt)
{
    delete fTimeZoneFormat;
    fTimeZoneFormat = timeZoneFormatToAdopt;
}


void
SimpleDateFormat::setTimeZoneFormat(const TimeZoneFormat& newTimeZoneFormat)
{
    delete fTimeZoneFormat;
    fTimeZoneFormat = new TimeZoneFormat(newTimeZoneFormat);
}




void SimpleDateFormat::adoptCalendar(Calendar* calendarToAdopt)
{
  UErrorCode status = U_ZERO_ERROR;
  DateFormat::adoptCalendar(calendarToAdopt);
  delete fSymbols;
  fSymbols=NULL;
  initializeSymbols(fLocale, fCalendar, status);  
  initializeDefaultCentury();  
}





void SimpleDateFormat::setContext(UDisplayContext value, UErrorCode& status)
{
    if (U_FAILURE(status))
        return;
    if ( (UDisplayContextType)((uint32_t)value >> 8) == UDISPCTX_TYPE_CAPITALIZATION ) {
        fCapitalizationContext = value;
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
   }
}





UDisplayContext SimpleDateFormat::getContext(UDisplayContextType type, UErrorCode& status) const
{
    if (U_FAILURE(status))
        return (UDisplayContext)0;
    if (type != UDISPCTX_TYPE_CAPITALIZATION) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return (UDisplayContext)0;
    }
    return fCapitalizationContext;
}





UBool
SimpleDateFormat::isFieldUnitIgnored(UCalendarDateFields field) const {
    return isFieldUnitIgnored(fPattern, field);
}


UBool
SimpleDateFormat::isFieldUnitIgnored(const UnicodeString& pattern,
                                     UCalendarDateFields field) {
    int32_t fieldLevel = fgCalendarFieldToLevel[field];
    int32_t level;
    UChar ch;
    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;

    for (int32_t i = 0; i < pattern.length(); ++i) {
        ch = pattern[i];
        if (ch != prevCh && count > 0) {
            level = fgPatternCharToLevel[prevCh - PATTERN_CHAR_BASE];
            
            if ( fieldLevel <= level ) {
                return FALSE;
            }
            count = 0;
        }
        if (ch == QUOTE) {
            if ((i+1) < pattern.length() && pattern[i+1] == QUOTE) {
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        }
        else if ( ! inQuote && ((ch >= 0x0061  && ch <= 0x007A )
                    || (ch >= 0x0041  && ch <= 0x005A ))) {
            prevCh = ch;
            ++count;
        }
    }
    if ( count > 0 ) {
        
        level = fgPatternCharToLevel[prevCh - PATTERN_CHAR_BASE];
            if ( fieldLevel <= level ) {
                return FALSE;
            }
    }
    return TRUE;
}



const Locale&
SimpleDateFormat::getSmpFmtLocale(void) const {
    return fLocale;
}



int32_t
SimpleDateFormat::checkIntSuffix(const UnicodeString& text, int32_t start,
                                 int32_t patLoc, UBool isNegative) const {
    
    UnicodeString suf;
    int32_t patternMatch;
    int32_t textPreMatch;
    int32_t textPostMatch;

    
    if ( (start > text.length()) ||
         (start < 0) ||
         (patLoc < 0) ||
         (patLoc > fPattern.length())) {
        
        return start;
    }

    
    DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fNumberFormat);
    if (decfmt != NULL) {
        if (isNegative) {
            suf = decfmt->getNegativeSuffix(suf);
        }
        else {
            suf = decfmt->getPositiveSuffix(suf);
        }
    }

    
    if (suf.length() <= 0) {
        return start;
    }

    
    patternMatch = compareSimpleAffix(suf,fPattern,patLoc);

    
    textPreMatch = compareSimpleAffix(suf,text,start);

    
    textPostMatch = compareSimpleAffix(suf,text,start-suf.length());

    
    if ((textPreMatch >= 0) && (patternMatch >= 0) && (textPreMatch == patternMatch)) {
        return start;
    }
    else if ((textPostMatch >= 0) && (patternMatch >= 0) && (textPostMatch == patternMatch)) {
        return  start - suf.length();
    }

    
    return start;
}



int32_t
SimpleDateFormat::compareSimpleAffix(const UnicodeString& affix,
                   const UnicodeString& input,
                   int32_t pos) const {
    int32_t start = pos;
    for (int32_t i=0; i<affix.length(); ) {
        UChar32 c = affix.char32At(i);
        int32_t len = U16_LENGTH(c);
        if (PatternProps::isWhiteSpace(c)) {
            
            
            
            
            
            
            UBool literalMatch = FALSE;
            while (pos < input.length() &&
                   input.char32At(pos) == c) {
                literalMatch = TRUE;
                i += len;
                pos += len;
                if (i == affix.length()) {
                    break;
                }
                c = affix.char32At(i);
                len = U16_LENGTH(c);
                if (!PatternProps::isWhiteSpace(c)) {
                    break;
                }
            }

            
            i = skipPatternWhiteSpace(affix, i);

            
            
            
            int32_t s = pos;
            pos = skipUWhiteSpace(input, pos);
            if (pos == s && !literalMatch) {
                return -1;
            }

            
            
            
            i = skipUWhiteSpace(affix, i);
        } else {
            if (pos < input.length() &&
                input.char32At(pos) == c) {
                i += len;
                pos += len;
            } else {
                return -1;
            }
        }
    }
    return pos - start;
}



int32_t
SimpleDateFormat::skipPatternWhiteSpace(const UnicodeString& text, int32_t pos) const {
    const UChar* s = text.getBuffer();
    return (int32_t)(PatternProps::skipWhiteSpace(s + pos, text.length() - pos) - s);
}



int32_t
SimpleDateFormat::skipUWhiteSpace(const UnicodeString& text, int32_t pos) const {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}




TimeZoneFormat *
SimpleDateFormat::tzFormat() const {
    if (fTimeZoneFormat == NULL) {
        umtx_lock(&LOCK);
        {
            if (fTimeZoneFormat == NULL) {
                UErrorCode status = U_ZERO_ERROR;
                TimeZoneFormat *tzfmt = TimeZoneFormat::createInstance(fLocale, status);
                if (U_FAILURE(status)) {
                    return NULL;
                }

                const_cast<SimpleDateFormat *>(this)->fTimeZoneFormat = tzfmt;
            }
        }
        umtx_unlock(&LOCK);
    }
    return fTimeZoneFormat;
}

U_NAMESPACE_END

#endif 


