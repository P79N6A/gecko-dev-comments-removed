






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <stdlib.h>

#include "reldtfmt.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/msgfmt.h"

#include "gregoimp.h" 
#include "cmemory.h"

U_NAMESPACE_BEGIN





struct URelativeString {
    int32_t offset;         
    int32_t len;            
    const UChar* string;    
};

static const char DT_DateTimePatternsTag[]="DateTimePatterns";


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RelativeDateFormat)

RelativeDateFormat::RelativeDateFormat(const RelativeDateFormat& other) :
 DateFormat(other), fDateTimeFormatter(NULL), fDatePattern(other.fDatePattern),
 fTimePattern(other.fTimePattern), fCombinedFormat(NULL),
 fDateStyle(other.fDateStyle), fLocale(other.fLocale),
 fDayMin(other.fDayMin), fDayMax(other.fDayMax),
 fDatesLen(other.fDatesLen), fDates(NULL)
{
    if(other.fDateTimeFormatter != NULL) {
        fDateTimeFormatter = (SimpleDateFormat*)other.fDateTimeFormatter->clone();
    }
    if(other.fCombinedFormat != NULL) {
        fCombinedFormat = (MessageFormat*)other.fCombinedFormat->clone();
    }
    if (fDatesLen > 0) {
        fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);
        uprv_memcpy(fDates, other.fDates, sizeof(fDates[0])*fDatesLen);
    }
}

RelativeDateFormat::RelativeDateFormat( UDateFormatStyle timeStyle, UDateFormatStyle dateStyle,
                                        const Locale& locale, UErrorCode& status) :
 DateFormat(), fDateTimeFormatter(NULL), fDatePattern(), fTimePattern(), fCombinedFormat(NULL),
 fDateStyle(dateStyle), fLocale(locale), fDatesLen(0), fDates(NULL)
{
    if(U_FAILURE(status) ) {
        return;
    }
    
    if (timeStyle < UDAT_NONE || timeStyle > UDAT_SHORT) {
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    UDateFormatStyle baseDateStyle = (dateStyle > UDAT_SHORT)? (UDateFormatStyle)(dateStyle & ~UDAT_RELATIVE): dateStyle;
    DateFormat * df;
    
    
    if (baseDateStyle != UDAT_NONE) {
        df = createDateInstance((EStyle)baseDateStyle, locale);
        fDateTimeFormatter=dynamic_cast<SimpleDateFormat *>(df);
        if (fDateTimeFormatter == NULL) {
            status = U_UNSUPPORTED_ERROR;
             return;
        }
        fDateTimeFormatter->toPattern(fDatePattern);
        if (timeStyle != UDAT_NONE) {
            df = createTimeInstance((EStyle)timeStyle, locale);
            SimpleDateFormat *sdf = dynamic_cast<SimpleDateFormat *>(df);
            if (sdf != NULL) {
                sdf->toPattern(fTimePattern);
                delete sdf;
            }
        }
    } else {
        
        df = createTimeInstance((EStyle)timeStyle, locale);
        fDateTimeFormatter=dynamic_cast<SimpleDateFormat *>(df);
        if (fDateTimeFormatter == NULL) {
            status = U_UNSUPPORTED_ERROR;
            return;
        }
        fDateTimeFormatter->toPattern(fTimePattern);
    }
    
    
    initializeCalendar(NULL, locale, status);
    loadDates(status);
}

RelativeDateFormat::~RelativeDateFormat() {
    delete fDateTimeFormatter;
    delete fCombinedFormat;
    uprv_free(fDates);
}


Format* RelativeDateFormat::clone(void) const {
    return new RelativeDateFormat(*this);
}

UBool RelativeDateFormat::operator==(const Format& other) const {
    if(DateFormat::operator==(other)) {
        
        RelativeDateFormat* that = (RelativeDateFormat*)&other;
        return (fDateStyle==that->fDateStyle   &&
                fDatePattern==that->fDatePattern   &&
                fTimePattern==that->fTimePattern   &&
                fLocale==that->fLocale);
    }
    return FALSE;
}

static const UChar APOSTROPHE = (UChar)0x0027;

UnicodeString& RelativeDateFormat::format(  Calendar& cal,
                                UnicodeString& appendTo,
                                FieldPosition& pos) const {
                                
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString relativeDayString;
    
    
    int dayDiff = dayDifference(cal, status);

    
    int32_t len = 0;
    const UChar *theString = getStringForDay(dayDiff, len, status);
    if(U_SUCCESS(status) && (theString!=NULL)) {
        
        relativeDayString.setTo(theString, len);
    }
    
    if (fDatePattern.isEmpty()) {
        fDateTimeFormatter->applyPattern(fTimePattern);
        fDateTimeFormatter->format(cal,appendTo,pos);
    } else if (fTimePattern.isEmpty() || fCombinedFormat == NULL) {
        if (relativeDayString.length() > 0) {
            appendTo.append(relativeDayString);
        } else {
            fDateTimeFormatter->applyPattern(fDatePattern);
            fDateTimeFormatter->format(cal,appendTo,pos);
        }
    } else {
        UnicodeString datePattern;
        if (relativeDayString.length() > 0) {
            
            relativeDayString.findAndReplace(UNICODE_STRING("'", 1), UNICODE_STRING("''", 2) ); 
            relativeDayString.insert(0, APOSTROPHE); 
            relativeDayString.append(APOSTROPHE); 
            datePattern.setTo(relativeDayString);
        } else {
            datePattern.setTo(fDatePattern);
        }
        UnicodeString combinedPattern;
        Formattable timeDatePatterns[] = { fTimePattern, datePattern };
        fCombinedFormat->format(timeDatePatterns, 2, combinedPattern, pos, status); 
        fDateTimeFormatter->applyPattern(combinedPattern);
        fDateTimeFormatter->format(cal,appendTo,pos);
    }
    
    return appendTo;
}



UnicodeString&
RelativeDateFormat::format(const Formattable& obj, 
                         UnicodeString& appendTo, 
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    
    
    
    
    return DateFormat::format(obj, appendTo, pos, status);
}


void RelativeDateFormat::parse( const UnicodeString& text,
                    Calendar& cal,
                    ParsePosition& pos) const {

    int32_t startIndex = pos.getIndex();
    if (fDatePattern.isEmpty()) {
        
        fDateTimeFormatter->applyPattern(fTimePattern);
        fDateTimeFormatter->parse(text,cal,pos);
    } else if (fTimePattern.isEmpty() || fCombinedFormat == NULL) {
        
        
        UBool matchedRelative = FALSE;
        for (int n=0; n < fDatesLen && !matchedRelative; n++) {
            if (fDates[n].string != NULL &&
                    text.compare(startIndex, fDates[n].len, fDates[n].string) == 0) {
                
                UErrorCode status = U_ZERO_ERROR;
                matchedRelative = TRUE;

                
                cal.setTime(Calendar::getNow(),status);
                cal.add(UCAL_DATE,fDates[n].offset, status);

                if(U_FAILURE(status)) { 
                    
                    pos.setErrorIndex(startIndex);
                } else {
                    pos.setIndex(startIndex + fDates[n].len);
                }
            }
        }
        if (!matchedRelative) {
            
            fDateTimeFormatter->applyPattern(fDatePattern);
            fDateTimeFormatter->parse(text,cal,pos);
        }
    } else {
        
        
        UnicodeString modifiedText(text);
        FieldPosition fPos;
        int32_t dateStart = 0, origDateLen = 0, modDateLen = 0;
        UErrorCode status = U_ZERO_ERROR;
        for (int n=0; n < fDatesLen; n++) {
            int32_t relativeStringOffset;
            if (fDates[n].string != NULL &&
                    (relativeStringOffset = modifiedText.indexOf(fDates[n].string, fDates[n].len, startIndex)) >= startIndex) {
                
                UnicodeString dateString;
                Calendar * tempCal = cal.clone();

                
                tempCal->setTime(Calendar::getNow(),status);
                tempCal->add(UCAL_DATE,fDates[n].offset, status);
                if(U_FAILURE(status)) { 
                    pos.setErrorIndex(startIndex);
                    delete tempCal;
                    return;
                }

                fDateTimeFormatter->applyPattern(fDatePattern);
                fDateTimeFormatter->format(*tempCal, dateString, fPos);
                dateStart = relativeStringOffset;
                origDateLen = fDates[n].len;
                modDateLen = dateString.length();
                modifiedText.replace(dateStart, origDateLen, dateString);
                delete tempCal;
                break;
            }
        }
        UnicodeString combinedPattern;
        Formattable timeDatePatterns[] = { fTimePattern, fDatePattern };
        fCombinedFormat->format(timeDatePatterns, 2, combinedPattern, fPos, status); 
        fDateTimeFormatter->applyPattern(combinedPattern);
        fDateTimeFormatter->parse(modifiedText,cal,pos);

        
        UBool noError = (pos.getErrorIndex() < 0);
        int32_t offset = (noError)? pos.getIndex(): pos.getErrorIndex();
        if (offset >= dateStart + modDateLen) {
            
            
            offset -= (modDateLen - origDateLen);
        } else if (offset >= dateStart) {
            
            
            offset = dateStart;
        }
        if (noError) {
            pos.setIndex(offset);
        } else {
            pos.setErrorIndex(offset);
        }
    }
}

UDate
RelativeDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    
    
    return DateFormat::parse(text, pos);
}

UDate
RelativeDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    
    
    return DateFormat::parse(text, status);
}


const UChar *RelativeDateFormat::getStringForDay(int32_t day, int32_t &len, UErrorCode &status) const {
    if(U_FAILURE(status)) {
        return NULL;
    }
    
    
    if(day < fDayMin || day > fDayMax) {
        return NULL; 
    }
    
    
    for(int n=0;n<fDatesLen;n++) {
        if(fDates[n].offset == day) {
            len = fDates[n].len;
            return fDates[n].string;
        }
    }
    
    return NULL;  
}

UnicodeString&
RelativeDateFormat::toPattern(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        if (fDatePattern.isEmpty()) {
            result.setTo(fTimePattern);
        } else if (fTimePattern.isEmpty() || fCombinedFormat == NULL) {
            result.setTo(fDatePattern);
        } else {
            Formattable timeDatePatterns[] = { fTimePattern, fDatePattern };
            FieldPosition pos;
            fCombinedFormat->format(timeDatePatterns, 2, result, pos, status);
        }
    }
    return result;
}

UnicodeString&
RelativeDateFormat::toPatternDate(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        result.setTo(fDatePattern);
    }
    return result;
}

UnicodeString&
RelativeDateFormat::toPatternTime(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        result.setTo(fTimePattern);
    }
    return result;
}

void
RelativeDateFormat::applyPatterns(const UnicodeString& datePattern, const UnicodeString& timePattern, UErrorCode &status)
{
    if (!U_FAILURE(status)) {
        fDatePattern.setTo(datePattern);
        fTimePattern.setTo(timePattern);
    }
}

const DateFormatSymbols*
RelativeDateFormat::getDateFormatSymbols() const
{
    return fDateTimeFormatter->getDateFormatSymbols();
}

void RelativeDateFormat::loadDates(UErrorCode &status) {
    CalendarData calData(fLocale, "gregorian", status);
    
    UErrorCode tempStatus = status;
    UResourceBundle *dateTimePatterns = calData.getByKey(DT_DateTimePatternsTag, tempStatus);
    if(U_SUCCESS(tempStatus)) {
        int32_t patternsSize = ures_getSize(dateTimePatterns);
        if (patternsSize > kDateTime) {
            int32_t resStrLen = 0;

            int32_t glueIndex = kDateTime;
            if (patternsSize >= (DateFormat::kDateTimeOffset + DateFormat::kShort + 1)) {
                
                switch (fDateStyle) { 
                case kFullRelative: 
                case kFull: 
                    glueIndex = kDateTimeOffset + kFull; 
                    break; 
                case kLongRelative: 
                case kLong: 
                    glueIndex = kDateTimeOffset + kLong; 
                    break; 
                case kMediumRelative: 
                case kMedium: 
                    glueIndex = kDateTimeOffset + kMedium; 
                    break;         
                case kShortRelative: 
                case kShort: 
                    glueIndex = kDateTimeOffset + kShort; 
                    break; 
                default: 
                    break; 
                } 
            }

            const UChar *resStr = ures_getStringByIndex(dateTimePatterns, glueIndex, &resStrLen, &tempStatus);
            fCombinedFormat = new MessageFormat(UnicodeString(TRUE, resStr, resStrLen), fLocale, tempStatus);
        }
    }

    UResourceBundle *strings = calData.getByKey3("fields", "day", "relative", status);
    
    fDayMin=-1;
    fDayMax=1;

    if(U_FAILURE(status)) {
        fDatesLen=0;
        return;
    }

    fDatesLen = ures_getSize(strings);
    fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);

    
    int n = 0;

    UResourceBundle *subString = NULL;
    
    while(ures_hasNext(strings) && U_SUCCESS(status)) {  
        subString = ures_getNextResource(strings, subString, &status);
        
        if(U_FAILURE(status) || (subString==NULL)) break;
        
        
        const char *key = ures_getKey(subString);
        
        
        int32_t aLen;
        const UChar* aString = ures_getString(subString, &aLen, &status);
        
        if(U_FAILURE(status) || aString == NULL) break;

        
        int32_t offset = atoi(key);
        
        
        if(offset < fDayMin) {
            fDayMin = offset;
        }
        if(offset > fDayMax) {
            fDayMax = offset;
        }
        
        
        fDates[n].offset = offset;
        fDates[n].string = aString;
        fDates[n].len = aLen; 

        n++;
    }
    ures_close(subString);
    
    
}




Calendar*
RelativeDateFormat::initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status)
{
    if(!U_FAILURE(status)) {
        fCalendar = Calendar::createInstance(adoptZone?adoptZone:TimeZone::createDefault(), locale, status);
    }
    if (U_SUCCESS(status) && fCalendar == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return fCalendar;
}

int32_t RelativeDateFormat::dayDifference(Calendar &cal, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return 0;
    }
    
    Calendar *nowCal = cal.clone();
    nowCal->setTime(Calendar::getNow(), status);

    
    
    
    int32_t dayDiff = cal.get(UCAL_JULIAN_DAY, status) - nowCal->get(UCAL_JULIAN_DAY, status);

    delete nowCal;
    return dayDiff;
}

U_NAMESPACE_END

#endif

