






#ifndef jsstr_h___
#define jsstr_h___

#include <ctype.h>
#include "jsapi.h"
#include "jsatom.h"
#include "jslock.h"
#include "jsutil.h"

#include "js/HashTable.h"
#include "vm/Unicode.h"

class JSFlatString;
class JSStableString;

namespace js {


class StringBuffer;








class StringSegmentRange;
class MutatingRopeSegmentRange;




class RopeBuilder;

template <AllowGC allowGC>
extern JSString *
ConcatStrings(JSContext *cx,
              typename MaybeRooted<JSString*, allowGC>::HandleType left,
              typename MaybeRooted<JSString*, allowGC>::HandleType right);

}  

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
#define JS7_UNHEX(c)    (unsigned)(JS7_ISDEC(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define JS7_ISLET(c)    ((c) < 128 && isalpha(c))


extern JSObject *
js_InitStringClass(JSContext *cx, js::HandleObject obj);

extern const char js_escape_str[];
extern const char js_unescape_str[];
extern const char js_uneval_str[];
extern const char js_decodeURI_str[];
extern const char js_encodeURI_str[];
extern const char js_decodeURIComponent_str[];
extern const char js_encodeURIComponent_str[];


template <js::AllowGC allowGC>
extern JSStableString *
js_NewString(JSContext *cx, jschar *chars, size_t length);

extern JSLinearString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start, size_t length);


template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n);

template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n);


template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyZ(JSContext *cx, const jschar *s);

template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyZ(JSContext *cx, const char *s);




extern const char *
js_ValueToPrintable(JSContext *cx, const js::Value &,
                    JSAutoByteString *bytes, bool asSource = false);

namespace js {





template <AllowGC allowGC>
extern JSString *
ToStringSlow(JSContext *cx, const Value &v);






template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSString *
ToString(JSContext *cx, const js::Value &v)
{
#ifdef DEBUG
    if (allowGC) {
        SkipRoot skip(cx, &v);
        MaybeCheckStackRoots(cx);
    }
#endif

    if (v.isString())
        return v.toString();
    return ToStringSlow<allowGC>(cx, v);
}






inline bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);

} 

namespace js {




extern JSString *
ValueToSource(JSContext *cx, const js::Value &v);





extern bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, bool *result);


extern bool
EqualStrings(JSContext *cx, JSLinearString *str1, JSLinearString *str2, bool *result) MOZ_DELETE;


extern bool
EqualStrings(JSLinearString *str1, JSLinearString *str2);





extern bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result);




extern bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes);

} 

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

static JS_ALWAYS_INLINE void
js_strncpy(jschar *dst, const jschar *src, size_t nelem)
{
    return js::PodCopy(dst, src, nelem);
}

extern jschar *
js_strdup(JSContext *cx, const jschar *s);

namespace js {






extern jschar *
InflateString(JSContext *cx, const char *bytes, size_t *length);






extern jschar *
InflateUTF8String(JSContext *cx, const char *bytes, size_t *length);








extern bool
InflateStringToBuffer(JSContext *cx, const char *bytes, size_t length,
                      jschar *chars, size_t *charsLength);

extern bool
InflateUTF8StringToBuffer(JSContext *cx, const char *bytes, size_t length,
                          jschar *chars, size_t *charsLength);






extern bool
InflateUTF8StringToBufferReplaceInvalid(JSContext *cx, const char *bytes,
                                        size_t length, jschar *chars,
                                        size_t *charsLength);







extern bool
DeflateStringToBuffer(JSContext *cx, const jschar *chars,
                      size_t charsLength, char *bytes, size_t *length);





extern JSBool
str_replace(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
str_fromCharCode(JSContext *cx, unsigned argc, Value *vp);

} 

extern JSBool
js_str_toString(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_str_charAt(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_str_charCodeAt(JSContext *cx, unsigned argc, js::Value *vp);





extern int
js_OneUcs4ToUtf8Char(uint8_t *utf8Buffer, uint32_t ucs4Char);

namespace js {

extern size_t
PutEscapedStringImpl(char *buffer, size_t size, FILE *fp, JSLinearString *str, uint32_t quote);










inline size_t
PutEscapedString(char *buffer, size_t size, JSLinearString *str, uint32_t quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, NULL, str, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE *fp, JSLinearString *str, uint32_t quote)
{
    return PutEscapedStringImpl(NULL, 0, fp, str, quote) != size_t(-1);
}

JSBool
str_match(JSContext *cx, unsigned argc, Value *vp);

JSBool
str_search(JSContext *cx, unsigned argc, Value *vp);

JSBool
str_split(JSContext *cx, unsigned argc, Value *vp);

} 

extern JSBool
js_String(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
