













#ifndef _REGEXTXT_H
#define _REGEXTXT_H

#include "unicode/utypes.h"
#include "unicode/utext.h"

U_NAMESPACE_BEGIN

#define UTEXT_USES_U16(ut) (NULL==((ut)->pFuncs->mapNativeIndexToUTF16))

#if 0
#define REGEX_DISABLE_CHUNK_MODE 1
#endif

#ifdef REGEX_DISABLE_CHUNK_MODE
#  define UTEXT_FULL_TEXT_IN_CHUNK(ut,len) (FALSE)
#else
#  define UTEXT_FULL_TEXT_IN_CHUNK(ut,len) ((0==((ut)->chunkNativeStart))&&((len)==((ut)->chunkNativeLimit))&&((len)==((ut)->nativeIndexingLimit)))
#endif

struct URegexUTextUnescapeCharContext {
    UText *text;
    int32_t lastOffset;
};
#define U_REGEX_UTEXT_UNESCAPE_CONTEXT(text) { (text), -1 }

U_CFUNC UChar U_CALLCONV
uregex_utext_unescape_charAt(int32_t offset, void *  context);
U_CFUNC UChar U_CALLCONV
uregex_ucstr_unescape_charAt(int32_t offset, void *  context);

U_NAMESPACE_END

#endif
