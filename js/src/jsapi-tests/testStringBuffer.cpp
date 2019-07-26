







#include "tests.h"

#include "jsatom.h"

#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

BEGIN_TEST(testStringBuffer_finishString)
{
    JSString *str = JS_NewStringCopyZ(cx, "foopy");
    CHECK(str);

    js::Rooted<JSAtom*> atom(cx, js::AtomizeString<js::CanGC>(cx, str));
    CHECK(atom);

    js::StringBuffer buffer(cx);
    CHECK(buffer.append("foopy"));

    JSAtom *finishedAtom = buffer.finishAtom();
    CHECK(finishedAtom);
    CHECK_EQUAL(atom, finishedAtom);
    return true;
}
END_TEST(testStringBuffer_finishString)
