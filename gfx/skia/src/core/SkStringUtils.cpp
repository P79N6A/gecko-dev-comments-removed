






#include "SkString.h"
#include "SkStringUtils.h"

void SkAddFlagToString(SkString* string, bool flag, const char* flagStr, bool* needSeparator) {
    if (flag) {
        if (*needSeparator) {
            string->append("|");
        }
        string->append(flagStr);
        *needSeparator = true;
    }
}
