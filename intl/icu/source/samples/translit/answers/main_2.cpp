





#include "unicode/translit.h"
#include "unicode/rbt.h"
#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "unaccent.h"

int main(int argc, char **argv) {

    Calendar *cal;
    DateFormat *fmt;
    DateFormat *defFmt;
    Transliterator *greek_latin;
    UErrorCode status = U_ZERO_ERROR;
    Locale greece("el", "GR");
    UnicodeString str, str2;

    
    cal = Calendar::createInstance(greece, status);
    check(status, "Calendar::createInstance");

    
    fmt = DateFormat::createDateInstance(DateFormat::kFull, greece);
    fmt->setCalendar(*cal);

    
    defFmt = DateFormat::createDateInstance(DateFormat::kFull);
    defFmt->setCalendar(*cal);

    
    greek_latin = Transliterator::createInstance("Greek-Latin");
    if (greek_latin == 0) {
        printf("ERROR: Transliterator::createInstance() failed\n");
        exit(1);
    }

    
    for (int32_t month = Calendar::JANUARY;
         month <= Calendar::DECEMBER;
         ++month) {

        
        cal->clear();
        cal->set(1999, month, 4);
        
        
        str.remove();
        defFmt->format(cal->getTime(status), str, status);
        check(status, "DateFormat::format");
        printf("Date: ");
        uprintf(escape(str));
        printf("\n");
        
        
        str.remove();
        fmt->format(cal->getTime(status), str, status);
        check(status, "DateFormat::format");
        printf("Greek formatted date: ");
        uprintf(escape(str));
        printf("\n");
        
        
        greek_latin->transliterate(str);
        printf("Transliterated via Greek-Latin: ");
        uprintf(escape(str));
        printf("\n\n");
    }

    
    delete fmt;
    delete cal;
    delete greek_latin;

    printf("Exiting successfully\n");
    return 0;
}
