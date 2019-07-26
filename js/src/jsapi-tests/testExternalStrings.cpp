



#include "mozilla/PodOperations.h"
#include "mozilla/Util.h"

#include "tests.h"

#include "jsutil.h"

using mozilla::ArrayLength;
using mozilla::PodEqual;

static const jschar arr[] = {
    'h', 'i', ',', 'd', 'o', 'n', '\'', 't', ' ', 'd', 'e', 'l', 'e', 't', 'e', ' ', 'm', 'e', '\0'
};
static const size_t arrlen = ArrayLength(arr) - 1;

static int finalized1 = 0;
static int finalized2 = 0;

static void
finalize_str(const JSStringFinalizer *fin, jschar *chars);

static const JSStringFinalizer finalizer1 = { finalize_str };
static const JSStringFinalizer finalizer2 = { finalize_str };

static void
finalize_str(const JSStringFinalizer *fin, jschar *chars)
{
    if (chars && PodEqual(const_cast<const jschar *>(chars), arr, arrlen)) {
        if (fin == &finalizer1) {
            ++finalized1;
        } else if (fin == &finalizer2) {
            ++finalized2;
        }
    }
}

BEGIN_TEST(testExternalStrings)
{
    const unsigned N = 1000;

    for (unsigned i = 0; i < N; ++i) {
        CHECK(JS_NewExternalString(cx, arr, arrlen, &finalizer1));
        CHECK(JS_NewExternalString(cx, arr, arrlen, &finalizer2));
    }

    
    JS_NewUCStringCopyN(cx, arr, arrlen);

    JS_GC(rt);

    
    const unsigned epsilon = 10;

    CHECK((N - finalized1) < epsilon);
    CHECK((N - finalized2) < epsilon);

    return true;
}
END_TEST(testExternalStrings)
