





#include "unicode/unistr.h"
#include "unicode/msgfmt.h"
#include "unicode/calendar.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"



static UnicodeString PATTERN(
    "Received {0,choice,0#no arguments|1#one argument|2#{0,number,integer} arguments}"
    " on {1,date,long}."
);

int main(int argc, char **argv) {

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    FieldPosition pos;

    
    MessageFormat msg(PATTERN, status);
    check(status, "MessageFormat::ct");

    
    Formattable msgArgs[2];
    msgArgs[0].setLong(argc-1);
    msgArgs[1].setDate(Calendar::getNow());

    
    msg.format(msgArgs, 2, str, pos, status);
    check(status, "MessageFormat::format");

    printf("Message: ");
    uprintf(str);
    printf("\n");

    printf("Exiting successfully\n");
    return 0;
}
