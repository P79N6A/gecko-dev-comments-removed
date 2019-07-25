






































#ifndef jsstr_h___
#define jsstr_h___

#include <ctype.h>
#include "jsapi.h"
#include "jsprvtd.h"
#include "jshashtable.h"
#include "jslock.h"
#include "jsobj.h"
#include "jscell.h"

#include "vm/Unicode.h"

namespace js {


class StringBuffer;








class StringSegmentRange;
class MutatingRopeSegmentRange;




class RopeBuilder;

}  

extern JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *s1, JSString *s2);

extern JSString * JS_FASTCALL
js_toLowerCase(JSContext *cx, JSString *str);

extern JSString * JS_FASTCALL
js_toUpperCase(JSContext *cx, JSString *str);

struct JSSubString {
    size_t          length;
    const jschar    *chars;
};

extern jschar      js_empty_ucstr[];
extern JSSubString js_EmptySubString;





#define JS7_ISDEC(c)    ((((unsigned)(c)) - '0') <= 9)
#define JS7_UNDEC(c)    ((c) - '0')
#define JS7_ISHEX(c)    ((c) < 128 && isxdigit(c))
#define JS7_UNHEX(c)    (uintN)(JS7_ISDEC(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define JS7_ISLET(c)    ((c) < 128 && isalpha(c))


extern JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj);

extern const char js_escape_str[];
extern const char js_unescape_str[];
extern const char js_uneval_str[];
extern const char js_decodeURI_str[];
extern const char js_encodeURI_str[];
extern const char js_decodeURIComponent_str[];
extern const char js_encodeURIComponent_str[];


extern JSFixedString *
js_NewString(JSContext *cx, jschar *chars, size_t length);

extern JSLinearString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start, size_t length);


extern JSFixedString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);

extern JSFixedString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n);


extern JSFixedString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);

extern JSFixedString *
js_NewStringCopyZ(JSContext *cx, const char *s);




extern const char *
js_ValueToPrintable(JSContext *cx, const js::Value &,
                    JSAutoByteString *bytes, bool asSource = false);





extern JSString *
js_ValueToString(JSContext *cx, const js::Value &v);

namespace js {






static JS_ALWAYS_INLINE JSString *
ValueToString_TestForStringInline(JSContext *cx, const Value &v)
{
    if (v.isString())
        return v.toString();
    return js_ValueToString(cx, v);
}






inline bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);

} 





extern JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const js::Value &v);

namespace js {





inline uint32
HashChars(const jschar *chars, size_t length)
{
    uint32 h = 0;
    for (; length; chars++, length--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *chars;
    return h;
}





extern bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, JSBool *result);


extern bool
EqualStrings(JSLinearString *str1, JSLinearString *str2);





extern bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32 *result);




extern bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes);

} 

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

#define js_strncpy(t, s, n)     memcpy((t), (s), (n) * sizeof(jschar))

namespace js {


















enum FlationCoding
{
    NormalEncoding,
    CESU8Encoding
};






extern jschar *
InflateString(JSContext *cx, const char *bytes, size_t *length,
              FlationCoding fc = NormalEncoding);

extern char *
DeflateString(JSContext *cx, const jschar *chars, size_t length);








extern bool
InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                      jschar *chars, size_t *charsLength);

extern bool
InflateUTF8StringToBuffer(JSContext *cx, const char *bytes, size_t length,
                          jschar *chars, size_t *charsLength,
                          FlationCoding fc = NormalEncoding);


extern size_t
GetDeflatedStringLength(JSContext *cx, const jschar *chars, size_t charsLength);


extern size_t
GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                            size_t charsLength,
                            FlationCoding fc = NormalEncoding);







extern bool
DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                      size_t charsLength, char *bytes, size_t *length);




extern bool
DeflateStringToUTF8Buffer(JSContext *cx, const jschar *chars,
                          size_t charsLength, char *bytes, size_t *length,
                          FlationCoding fc = NormalEncoding);

} 





namespace js {
extern JSBool
str_replace(JSContext *cx, uintN argc, js::Value *vp);
}

extern JSBool
js_str_toString(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_str_charAt(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_str_charCodeAt(JSContext *cx, uintN argc, js::Value *vp);





extern int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char);

namespace js {

extern size_t
PutEscapedStringImpl(char *buffer, size_t size, FILE *fp, JSLinearString *str, uint32 quote);










inline size_t
PutEscapedString(char *buffer, size_t size, JSLinearString *str, uint32 quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, NULL, str, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE *fp, JSLinearString *str, uint32 quote)
{
    return PutEscapedStringImpl(NULL, 0, fp, str, quote) != size_t(-1);
}

} 

extern JSBool
js_String(JSContext *cx, uintN argc, js::Value *vp);

#endif 
