





#include "unicode/unistr.h"
#include "unicode/fmtable.h"


void check(UErrorCode& status, const char* msg);


UnicodeString escape(const UnicodeString &source);


void uprintf(const UnicodeString &str);


UnicodeString formattableToString(const Formattable& f);
