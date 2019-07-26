






#include "tests.h"
#include "jsapi.h"
#include "jsstr.h"
#include "js/CharacterEncoding.h"

BEGIN_TEST(testUTF8_badUTF8)
{
    static const char badUTF8[] = "...\xC0...";
    JSString *str = JS_NewStringCopyZ(cx, badUTF8);
    CHECK(str);
    const jschar *chars = JS_GetStringCharsZ(cx, str);
    CHECK(chars);
    CHECK(chars[3] == 0x00C0);
    return true;
}
END_TEST(testUTF8_badUTF8)

BEGIN_TEST(testUTF8_bigUTF8)
{
    static const char bigUTF8[] = "...\xFB\xBF\xBF\xBF\xBF...";
    JSString *str = JS_NewStringCopyZ(cx, bigUTF8);
    CHECK(str);
    const jschar *chars = JS_GetStringCharsZ(cx, str);
    CHECK(chars);
    CHECK(chars[3] == 0x00FB);
    return true;
}
END_TEST(testUTF8_bigUTF8)

BEGIN_TEST(testUTF8_badSurrogate)
{
    static const jschar badSurrogate[] = { 'A', 'B', 'C', 0xDEEE, 'D', 'E', 0 };
    JS::TwoByteChars tbchars(badSurrogate, js_strlen(badSurrogate));
    JS::Latin1CharsZ latin1 = JS::LossyTwoByteCharsToNewLatin1CharsZ(cx, tbchars);
    CHECK(latin1);
    CHECK(latin1[3] == 0x00EE);
    return true;
}
END_TEST(testUTF8_badSurrogate)
