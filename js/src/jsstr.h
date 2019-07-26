





#ifndef jsstr_h
#define jsstr_h

#include "mozilla/HashFunctions.h"
#include "mozilla/PodOperations.h"

#include "jsutil.h"
#include "NamespaceImports.h"

#include "gc/Rooting.h"
#include "js/RootingAPI.h"
#include "vm/Unicode.h"

class JSAutoByteString;
class JSFlatString;
class JSLinearString;

namespace js {

class StringBuffer;

class MutatingRopeSegmentRange;

template <AllowGC allowGC>
extern JSString *
ConcatStrings(ThreadSafeContext *cx,
              typename MaybeRooted<JSString*, allowGC>::HandleType left,
              typename MaybeRooted<JSString*, allowGC>::HandleType right);


static inline const jschar *
SkipSpace(const jschar *s, const jschar *end)
{
    JS_ASSERT(s <= end);

    while (s < end && unicode::IsSpace(*s))
        s++;

    return s;
}



inline int32_t
CompareChars(const jschar *s1, size_t l1, const jschar *s2, size_t l2)
{
    size_t n = Min(l1, l2);
    for (size_t i = 0; i < n; i++) {
        if (int32_t cmp = s1[i] - s2[i])
            return cmp;
    }

    return (int32_t)(l1 - l2);
}

}  

extern JSString * JS_FASTCALL
js_toLowerCase(JSContext *cx, JSString *str);

extern JSString * JS_FASTCALL
js_toUpperCase(JSContext *cx, JSString *str);

struct JSSubString {
    size_t          length;
    const jschar    *chars;
};

extern const jschar js_empty_ucstr[];
extern const JSSubString js_EmptySubString;





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
extern JSFlatString *
js_NewString(js::ThreadSafeContext *cx, jschar *chars, size_t length);

extern JSLinearString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start, size_t length);


template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyN(js::ExclusiveContext *cx, const jschar *s, size_t n);

template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyN(js::ThreadSafeContext *cx, const char *s, size_t n);


template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyZ(js::ExclusiveContext *cx, const jschar *s);

template <js::AllowGC allowGC>
extern JSFlatString *
js_NewStringCopyZ(js::ThreadSafeContext *cx, const char *s);




extern const char *
js_ValueToPrintable(JSContext *cx, const js::Value &,
                    JSAutoByteString *bytes, bool asSource = false);

namespace js {





template <AllowGC allowGC>
extern JSString *
ToStringSlow(ExclusiveContext *cx, typename MaybeRooted<Value, allowGC>::HandleType arg);






template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSString *
ToString(JSContext *cx, JS::HandleValue v)
{
    if (v.isString())
        return v.toString();
    return ToStringSlow<allowGC>(cx, v);
}






inline bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);





extern JSString *
ValueToSource(JSContext *cx, HandleValue v);






extern JSString *
StringToSource(JSContext *cx, JSString *str);





extern bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, bool *result);


extern bool
EqualStrings(JSContext *cx, JSLinearString *str1, JSLinearString *str2, bool *result) MOZ_DELETE;


extern bool
EqualStrings(JSLinearString *str1, JSLinearString *str2);





extern bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result);

extern int32_t
CompareAtoms(JSAtom *atom1, JSAtom *atom2);




extern bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes);


extern bool
StringHasPattern(const jschar *text, uint32_t textlen,
                 const jschar *pat, uint32_t patlen);

extern int
StringFindPattern(const jschar *text, uint32_t textlen,
                  const jschar *pat, uint32_t patlen);

extern bool
StringHasRegExpMetaChars(const jschar *chars, size_t length);

} 

extern size_t
js_strlen(const jschar *s);

extern int32_t
js_strcmp(const jschar *lhs, const jschar *rhs);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

static MOZ_ALWAYS_INLINE void
js_strncpy(jschar *dst, const jschar *src, size_t nelem)
{
    return mozilla::PodCopy(dst, src, nelem);
}

extern jschar *
js_strdup(js::ThreadSafeContext *cx, const jschar *s);

namespace js {







extern jschar *
InflateString(ThreadSafeContext *cx, const char *bytes, size_t *length);





inline void
CopyAndInflateChars(jschar *dst, const char *src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i++)
        dst[i] = (unsigned char) src[i];
}







extern bool
DeflateStringToBuffer(JSContext *maybecx, const jschar *chars,
                      size_t charsLength, char *bytes, size_t *length);





extern bool
str_replace(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
str_fromCharCode(JSContext *cx, unsigned argc, Value *vp);

} 

extern bool
js_str_toString(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_str_charAt(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_str_charCodeAt(JSContext *cx, unsigned argc, js::Value *vp);





extern int
js_OneUcs4ToUtf8Char(uint8_t *utf8Buffer, uint32_t ucs4Char);

namespace js {

extern size_t
PutEscapedStringImpl(char *buffer, size_t size, FILE *fp, JSLinearString *str, uint32_t quote);

extern size_t
PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, const jschar *chars,
                     size_t length, uint32_t quote);










inline size_t
PutEscapedString(char *buffer, size_t size, JSLinearString *str, uint32_t quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, nullptr, str, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}

inline size_t
PutEscapedString(char *buffer, size_t bufferSize, const jschar *chars, size_t length, uint32_t quote)
{
    size_t n = PutEscapedStringImpl(buffer, bufferSize, nullptr, chars, length, quote);

    
    JS_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE *fp, JSLinearString *str, uint32_t quote)
{
    return PutEscapedStringImpl(nullptr, 0, fp, str, quote) != size_t(-1);
}

bool
str_match(JSContext *cx, unsigned argc, Value *vp);

bool
str_search(JSContext *cx, unsigned argc, Value *vp);

bool
str_split(JSContext *cx, unsigned argc, Value *vp);

JSObject *
str_split_string(JSContext *cx, HandleTypeObject type, HandleString str, HandleString sep);

bool
str_resolve(JSContext *cx, HandleObject obj, HandleId id, MutableHandleObject objp);

bool
str_replace_regexp_raw(JSContext *cx, HandleString string, HandleObject regexp,
                       HandleString replacement, MutableHandleValue rval);

bool
str_replace_string_raw(JSContext *cx, HandleString string, HandleString pattern,
                       HandleString replacement, MutableHandleValue rval);

} 

extern bool
js_String(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
