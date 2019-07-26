










#include "unicode/utf.h"
#include "regextxt.h"

U_NAMESPACE_BEGIN

U_CFUNC UChar U_CALLCONV
uregex_utext_unescape_charAt(int32_t offset, void *ct) {
    struct URegexUTextUnescapeCharContext *context = (struct URegexUTextUnescapeCharContext *)ct;
    UChar32 c;
    if (offset == context->lastOffset + 1) {
        c = UTEXT_NEXT32(context->text);
        context->lastOffset++;
    } else if (offset == context->lastOffset) {
        c = UTEXT_PREVIOUS32(context->text);
        UTEXT_NEXT32(context->text);
    } else {
        utext_moveIndex32(context->text, offset - context->lastOffset - 1);
        c = UTEXT_NEXT32(context->text);
        context->lastOffset = offset;
    }

    
    if (U_IS_BMP(c)) {
        return (UChar)c;
    } else {
        return 0;
    }
}

U_CFUNC UChar U_CALLCONV
uregex_ucstr_unescape_charAt(int32_t offset, void *context) {
    return ((UChar *)context)[offset];
}

U_NAMESPACE_END
