





#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"







TimeZone* createZone(const UnicodeString& id) {
    UnicodeString str;
    TimeZone* zone = TimeZone::createTimeZone(id);
    if (zone->getID(str) != id) {
        delete zone;
        printf("Error: TimeZone::createTimeZone(");
        uprintf(id);
        printf(") returned zone with ID ");
        uprintf(str);
        printf("\n");
        exit(1);
    }
    return zone;
}

int main(int argc, char **argv) {

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;

    
    static char* LANGUAGE[] = {
        "en", "de", "fr"
    };
    static const int32_t N_LANGUAGE = sizeof(LANGUAGE)/sizeof(LANGUAGE[0]);

    
    static char* TIMEZONE[] = {
        "America/Los_Angeles",
        "America/New_York",
        "Europe/Paris",
        "Europe/Berlin"
    };
    static const int32_t N_TIMEZONE = sizeof(TIMEZONE)/sizeof(TIMEZONE[0]);

    for (int32_t i=0; i<N_LANGUAGE; ++i) {
        Locale loc(LANGUAGE[i]);

        
        printf("Date (%s)\n", LANGUAGE[i]);
    }

    printf("Exiting successfully\n");
    return 0;
}
