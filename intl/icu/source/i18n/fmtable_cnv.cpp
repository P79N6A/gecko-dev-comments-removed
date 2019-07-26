














#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_CONVERSION

#include "unicode/fmtable.h"





U_NAMESPACE_BEGIN






Formattable::Formattable(const char* stringToCopy)
{
    init();
    fType = kString;
    fValue.fString = new UnicodeString(stringToCopy);
}

U_NAMESPACE_END

#endif 


