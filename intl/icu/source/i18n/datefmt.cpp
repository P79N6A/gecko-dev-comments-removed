


















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtptngen.h"
#include "reldtfmt.h"

#include "cstring.h"
#include "windtfmt.h"

#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)
#include <stdio.h>
#endif





U_NAMESPACE_BEGIN

DateFormat::DateFormat()
:   fCalendar(0),
    fNumberFormat(0)
{
}



DateFormat::DateFormat(const DateFormat& other)
:   Format(other),
    fCalendar(0),
    fNumberFormat(0)
{
    *this = other;
}



DateFormat& DateFormat::operator=(const DateFormat& other)
{
    if (this != &other)
    {
        delete fCalendar;
        delete fNumberFormat;
        if(other.fCalendar) {
          fCalendar = other.fCalendar->clone();
        } else {
          fCalendar = NULL;
        }
        if(other.fNumberFormat) {
          fNumberFormat = (NumberFormat*)other.fNumberFormat->clone();
        } else {
          fNumberFormat = NULL;
        }
    }
    return *this;
}



DateFormat::~DateFormat()
{
    delete fCalendar;
    delete fNumberFormat;
}



UBool
DateFormat::operator==(const Format& other) const
{
    
    
    

    
    DateFormat* fmt = (DateFormat*)&other;

    return (this == fmt) ||
        (Format::operator==(other) &&
         fCalendar&&(fCalendar->isEquivalentTo(*fmt->fCalendar)) &&
         (fNumberFormat && *fNumberFormat == *fmt->fNumberFormat));
}



UnicodeString&
DateFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPosition& fieldPosition,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    
    UDate date = 0;
    switch (obj.getType())
    {
    case Formattable::kDate:
        date = obj.getDate();
        break;
    case Formattable::kDouble:
        date = (UDate)obj.getDouble();
        break;
    case Formattable::kLong:
        date = (UDate)obj.getLong();
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    
    
    

    return format(date, appendTo, fieldPosition);
}



UnicodeString&
DateFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPositionIterator* posIter,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    
    UDate date = 0;
    switch (obj.getType())
    {
    case Formattable::kDate:
        date = obj.getDate();
        break;
    case Formattable::kDouble:
        date = (UDate)obj.getDouble();
        break;
    case Formattable::kLong:
        date = (UDate)obj.getLong();
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    
    
    

    return format(date, appendTo, posIter, status);
}




UnicodeString&
DateFormat::format(Calendar& ,
                   UnicodeString& appendTo,
                   FieldPositionIterator* ,
                   UErrorCode& status) const {
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return appendTo;
}



UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo, FieldPosition& fieldPosition) const {
    if (fCalendar != NULL) {
        
        Calendar* calClone = fCalendar->clone();
        if (calClone != NULL) {
            UErrorCode ec = U_ZERO_ERROR;
            calClone->setTime(date, ec);
            if (U_SUCCESS(ec)) {
                format(*calClone, appendTo, fieldPosition);
            }
            delete calClone;
        }
    }
    return appendTo;
}



UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo, FieldPositionIterator* posIter,
                   UErrorCode& status) const {
    if (fCalendar != NULL) {
        Calendar* calClone = fCalendar->clone();
        if (calClone != NULL) {
            calClone->setTime(date, status);
            if (U_SUCCESS(status)) {
               format(*calClone, appendTo, posIter, status);
            }
            delete calClone;
        }
    }
    return appendTo;
}



UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo) const
{
    
    
    FieldPosition fpos(0);
    return format(date, appendTo, fpos);
}



UDate
DateFormat::parse(const UnicodeString& text,
                  ParsePosition& pos) const
{
    UDate d = 0; 
    if (fCalendar != NULL) {
        Calendar* calClone = fCalendar->clone();
        if (calClone != NULL) {
            int32_t start = pos.getIndex();
            calClone->clear();
            parse(text, *calClone, pos);
            if (pos.getIndex() != start) {
                UErrorCode ec = U_ZERO_ERROR;
                d = calClone->getTime(ec);
                if (U_FAILURE(ec)) {
                    
                    
                    
                    pos.setIndex(start);
                    pos.setErrorIndex(start);
                    d = 0;
                }
            }
            delete calClone;
        }
    }
    return d;
}



UDate
DateFormat::parse(const UnicodeString& text,
                  UErrorCode& status) const
{
    if (U_FAILURE(status)) return 0;

    ParsePosition pos(0);
    UDate result = parse(text, pos);
    if (pos.getIndex() == 0) {
#if defined (U_DEBUG_CAL)
      fprintf(stderr, "%s:%d - - failed to parse  - err index %d\n"
              , __FILE__, __LINE__, pos.getErrorIndex() );
#endif
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return result;
}



void
DateFormat::parseObject(const UnicodeString& source,
                        Formattable& result,
                        ParsePosition& pos) const
{
    result.setDate(parse(source, pos));
}



DateFormat* U_EXPORT2
DateFormat::createTimeInstance(DateFormat::EStyle style,
                               const Locale& aLocale)
{
    return create(style, kNone, aLocale);
}



DateFormat* U_EXPORT2
DateFormat::createDateInstance(DateFormat::EStyle style,
                               const Locale& aLocale)
{
    
    
    if(style != kNone)
    {
        style = (EStyle) (style + kDateOffset);
    }
    return create(kNone, (EStyle) (style), aLocale);
}



DateFormat* U_EXPORT2
DateFormat::createDateTimeInstance(EStyle dateStyle,
                                   EStyle timeStyle,
                                   const Locale& aLocale)
{
    if(dateStyle != kNone)
    {
        dateStyle = (EStyle) (dateStyle + kDateOffset);
    }
    return create(timeStyle, dateStyle, aLocale);
}



DateFormat* U_EXPORT2
DateFormat::createInstance()
{
    return create(kShort, (EStyle) (kShort + kDateOffset), Locale::getDefault());
}



DateFormat* U_EXPORT2
DateFormat::create(EStyle timeStyle, EStyle dateStyle, const Locale& locale)
{
    UErrorCode status = U_ZERO_ERROR;
#if U_PLATFORM_HAS_WIN32_API
    char buffer[8];
    int32_t count = locale.getKeywordValue("compat", buffer, sizeof(buffer), status);

    
    if (count > 0 && uprv_strcmp(buffer, "host") == 0) {
        Win32DateFormat *f = new Win32DateFormat(timeStyle, dateStyle, locale, status);

        if (U_SUCCESS(status)) {
            return f;
        }

        delete f;
    }
#endif

    
    if(((dateStyle!=kNone)&&((dateStyle-kDateOffset) & UDAT_RELATIVE))) {
        RelativeDateFormat *r = new RelativeDateFormat((UDateFormatStyle)timeStyle, (UDateFormatStyle)(dateStyle-kDateOffset), locale, status);
        if(U_SUCCESS(status)) return r;
        delete r;
        status = U_ZERO_ERROR;
    }

    
    SimpleDateFormat *f = new SimpleDateFormat(timeStyle, dateStyle, locale, status);
    if (U_SUCCESS(status)) return f;
    delete f;

    
    
    status = U_ZERO_ERROR;
    f = new SimpleDateFormat(locale, status);
    if (U_SUCCESS(status)) return f;
    delete f;

    
    
    
    return 0;
}



const Locale* U_EXPORT2
DateFormat::getAvailableLocales(int32_t& count)
{
    
    
    
    return Locale::getAvailableLocales(count);
}



void
DateFormat::adoptCalendar(Calendar* newCalendar)
{
    delete fCalendar;
    fCalendar = newCalendar;
}


void
DateFormat::setCalendar(const Calendar& newCalendar)
{
    Calendar* newCalClone = newCalendar.clone();
    if (newCalClone != NULL) {
        adoptCalendar(newCalClone);
    }
}



const Calendar*
DateFormat::getCalendar() const
{
    return fCalendar;
}



void
DateFormat::adoptNumberFormat(NumberFormat* newNumberFormat)
{
    delete fNumberFormat;
    fNumberFormat = newNumberFormat;
    newNumberFormat->setParseIntegerOnly(TRUE);
}


void
DateFormat::setNumberFormat(const NumberFormat& newNumberFormat)
{
    NumberFormat* newNumFmtClone = (NumberFormat*)newNumberFormat.clone();
    if (newNumFmtClone != NULL) {
        adoptNumberFormat(newNumFmtClone);
    }
}



const NumberFormat*
DateFormat::getNumberFormat() const
{
    return fNumberFormat;
}



void
DateFormat::adoptTimeZone(TimeZone* zone)
{
    if (fCalendar != NULL) {
        fCalendar->adoptTimeZone(zone);
    }
}


void
DateFormat::setTimeZone(const TimeZone& zone)
{
    if (fCalendar != NULL) {
        fCalendar->setTimeZone(zone);
    }
}



const TimeZone&
DateFormat::getTimeZone() const
{
    if (fCalendar != NULL) {
        return fCalendar->getTimeZone();
    }
    
    
    return *(TimeZone::createDefault());
}



void
DateFormat::setLenient(UBool lenient)
{
    if (fCalendar != NULL) {
        fCalendar->setLenient(lenient);
    }
}



UBool
DateFormat::isLenient() const
{
    if (fCalendar != NULL) {
        return fCalendar->isLenient();
    }
    
    return FALSE;
}

U_NAMESPACE_END

#endif 


