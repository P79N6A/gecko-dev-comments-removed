










#ifndef __RELDATEFMT_H
#define __RELDATEFMT_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/udisplaycontext.h"
#include "unicode/locid.h"






#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_BREAK_ITERATION

#ifndef U_HIDE_DRAFT_API





typedef enum UDateRelativeDateTimeFormatterStyle {

  



  UDAT_STYLE_LONG,

  



  UDAT_STYLE_SHORT,

  



  UDAT_STYLE_NARROW,

  



  UDAT_STYLE_COUNT
} UDateRelativeDateTimeFormatterStyle; 

#endif 






typedef enum UDateRelativeUnit {

    



    UDAT_RELATIVE_SECONDS,

    



    UDAT_RELATIVE_MINUTES,

    



    UDAT_RELATIVE_HOURS,

    



    UDAT_RELATIVE_DAYS,

    



    UDAT_RELATIVE_WEEKS,

    



    UDAT_RELATIVE_MONTHS,

    



    UDAT_RELATIVE_YEARS,

    



    UDAT_RELATIVE_UNIT_COUNT
} UDateRelativeUnit;





typedef enum UDateAbsoluteUnit {

    
    
    



    UDAT_ABSOLUTE_SUNDAY,

    



    UDAT_ABSOLUTE_MONDAY,

    



    UDAT_ABSOLUTE_TUESDAY,

    



    UDAT_ABSOLUTE_WEDNESDAY,

    



    UDAT_ABSOLUTE_THURSDAY,

    



    UDAT_ABSOLUTE_FRIDAY,

    



    UDAT_ABSOLUTE_SATURDAY,

    



    UDAT_ABSOLUTE_DAY,

    



    UDAT_ABSOLUTE_WEEK,

    



    UDAT_ABSOLUTE_MONTH,

    



    UDAT_ABSOLUTE_YEAR,

    



    UDAT_ABSOLUTE_NOW,

    



    UDAT_ABSOLUTE_UNIT_COUNT
} UDateAbsoluteUnit;






typedef enum UDateDirection {

    



    UDAT_DIRECTION_LAST_2,

    



    UDAT_DIRECTION_LAST,

    



    UDAT_DIRECTION_THIS,

    



    UDAT_DIRECTION_NEXT,

    



    UDAT_DIRECTION_NEXT_2,

    



    UDAT_DIRECTION_PLAIN,

    



    UDAT_DIRECTION_COUNT
} UDateDirection;


U_NAMESPACE_BEGIN

class RelativeDateTimeCacheData;
class SharedNumberFormat;
class SharedPluralRules;
class SharedBreakIterator;
class NumberFormat;
class UnicodeString;





































































class U_I18N_API RelativeDateTimeFormatter : public UObject {
public:

    



    RelativeDateTimeFormatter(UErrorCode& status);

    



    RelativeDateTimeFormatter(const Locale& locale, UErrorCode& status);

    









    RelativeDateTimeFormatter(
        const Locale& locale, NumberFormat *nfToAdopt, UErrorCode& status);

#ifndef U_HIDE_DRAFT_API
    














    RelativeDateTimeFormatter(
            const Locale& locale,
            NumberFormat *nfToAdopt,
            UDateRelativeDateTimeFormatterStyle style,
            UDisplayContext capitalizationContext,
            UErrorCode& status);
#endif  

    



    RelativeDateTimeFormatter(const RelativeDateTimeFormatter& other);

    



    RelativeDateTimeFormatter& operator=(
            const RelativeDateTimeFormatter& other);

    



    virtual ~RelativeDateTimeFormatter();

    














    UnicodeString& format(
            double quantity,
            UDateDirection direction,
            UDateRelativeUnit unit,
            UnicodeString& appendTo,
            UErrorCode& status) const;

    











    UnicodeString& format(
            UDateDirection direction,
            UDateAbsoluteUnit unit,
            UnicodeString& appendTo,
            UErrorCode& status) const;

    











    UnicodeString& combineDateAndTime(
            const UnicodeString& relativeDateString,
            const UnicodeString& timeString,
            UnicodeString& appendTo,
            UErrorCode& status) const;

    




    const NumberFormat& getNumberFormat() const;

#ifndef U_HIDE_DRAFT_API
    




    UDisplayContext getCapitalizationContext() const;

    




    UDateRelativeDateTimeFormatterStyle getFormatStyle() const;
#endif  

private:
    const RelativeDateTimeCacheData* fCache;
    const SharedNumberFormat *fNumberFormat;
    const SharedPluralRules *fPluralRules;
    UDateRelativeDateTimeFormatterStyle fStyle;
    UDisplayContext fContext;
    const SharedBreakIterator *fOptBreakIterator;
    Locale fLocale;
    void init(
            NumberFormat *nfToAdopt,
            BreakIterator *brkIter,
            UErrorCode &status);
    void adjustForContext(UnicodeString &) const;
};

U_NAMESPACE_END

#endif 
#endif
