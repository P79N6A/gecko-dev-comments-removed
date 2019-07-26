



#include "tests.h"
#include "jsatom.h"

#include "vm/String.h"

using mozilla::ArrayLength;

BEGIN_TEST(testAtomizedIsNotInterned)
{
    
    static const char someChars[] = "blah blah blah? blah blah blah";
    js::Rooted<JSAtom*> atom(cx, js::Atomize(cx, someChars, ArrayLength(someChars)));
    CHECK(!JS_StringHasBeenInterned(cx, atom));
    CHECK(JS_InternJSString(cx, atom));
    CHECK(JS_StringHasBeenInterned(cx, atom));
    return true;
}
END_TEST(testAtomizedIsNotInterned)

struct StringWrapper
{
    JSString *str;
    bool     strOk;
} sw;

void
FinalizeCallback(JSFreeOp *fop, JSFinalizeStatus status, JSBool isCompartmentGC)
{
    if (status == JSFINALIZE_GROUP_START)
        sw.strOk = !JS_IsAboutToBeFinalized(sw.str);
}

BEGIN_TEST(testInternAcrossGC)
{
    sw.str = JS_InternString(cx, "wrapped chars that another test shouldn't be using");
    sw.strOk = false;
    CHECK(sw.str);
    JS_SetFinalizeCallback(rt, FinalizeCallback);
    JS_GC(rt);
    CHECK(sw.strOk);
    return true;
}
END_TEST(testInternAcrossGC)
