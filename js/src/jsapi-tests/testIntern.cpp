



#include "jsatom.h"

#include "gc/Marking.h"
#include "jsapi-tests/tests.h"
#include "vm/String.h"

using mozilla::ArrayLength;

BEGIN_TEST(testAtomizedIsNotPinned)
{
    
    static const char someChars[] = "blah blah blah? blah blah blah";
    JS::Rooted<JSAtom*> atom(cx, js::Atomize(cx, someChars, ArrayLength(someChars)));
    CHECK(!JS_StringHasBeenPinned(cx, atom));
    CHECK(JS_AtomizeAndPinJSString(cx, atom));
    CHECK(JS_StringHasBeenPinned(cx, atom));
    return true;
}
END_TEST(testAtomizedIsNotPinned)

struct StringWrapperStruct
{
    JSString* str;
    bool     strOk;
} sw;

BEGIN_TEST(testPinAcrossGC)
{
    sw.str = JS_AtomizeAndPinString(cx, "wrapped chars that another test shouldn't be using");
    sw.strOk = false;
    CHECK(sw.str);
    JS_AddFinalizeCallback(rt, FinalizeCallback, nullptr);
    JS_GC(rt);
    CHECK(sw.strOk);
    return true;
}

static void
FinalizeCallback(JSFreeOp* fop, JSFinalizeStatus status, bool isCompartmentGC, void* data)
{
    if (status == JSFINALIZE_GROUP_START)
        sw.strOk = js::gc::IsMarkedUnbarriered(&sw.str);
}
END_TEST(testPinAcrossGC)
