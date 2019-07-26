






#include "jsatom.h"

#include "jsapi-tests/tests.h"
#include "vm/StringBuffer.h"

BEGIN_TEST(testStringBuffer_finishString)
{
    JSString *str = JS_NewStringCopyZ(cx, "foopy");
    CHECK(str);

    JS::Rooted<JSAtom*> atom(cx, js::AtomizeString(cx, str));
    CHECK(atom);

    js::StringBuffer buffer(cx);
    CHECK(buffer.append("foopy"));

    JSAtom *finishedAtom = buffer.finishAtom();
    CHECK(finishedAtom);
    CHECK_EQUAL(atom, finishedAtom);
    return true;
}
END_TEST(testStringBuffer_finishString)
