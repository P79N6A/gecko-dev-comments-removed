





#ifndef jsstr_h
#define jsstr_h

#include "mozilla/HashFunctions.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include "jsutil.h"
#include "NamespaceImports.h"

#include "gc/Rooting.h"
#include "js/RootingAPI.h"
#include "vm/Unicode.h"

class JSAutoByteString;
class JSLinearString;

namespace js {

class StringBuffer;

template <AllowGC allowGC>
extern JSString*
ConcatStrings(ExclusiveContext* cx,
              typename MaybeRooted<JSString*, allowGC>::HandleType left,
              typename MaybeRooted<JSString*, allowGC>::HandleType right);


template <typename CharT>
static inline const CharT*
SkipSpace(const CharT* s, const CharT* end)
{
    MOZ_ASSERT(s <= end);

    while (s < end && unicode::IsSpace(*s))
        s++;

    return s;
}



template <typename Char1, typename Char2>
inline int32_t
CompareChars(const Char1* s1, size_t len1, const Char2* s2, size_t len2)
{
    size_t n = Min(len1, len2);
    for (size_t i = 0; i < n; i++) {
        if (int32_t cmp = s1[i] - s2[i])
            return cmp;
    }

    return int32_t(len1 - len2);
}

extern int32_t
CompareChars(const char16_t* s1, size_t len1, JSLinearString* s2);

}  

struct JSSubString {
    JSLinearString* base;
    size_t          offset;
    size_t          length;

    JSSubString() { mozilla::PodZero(this); }

    void initEmpty(JSLinearString* base) {
        this->base = base;
        offset = length = 0;
    }
    void init(JSLinearString* base, size_t offset, size_t length) {
        this->base = base;
        this->offset = offset;
        this->length = length;
    }
};





#define JS7_ISDEC(c)    ((((unsigned)(c)) - '0') <= 9)
#define JS7_UNDEC(c)    ((c) - '0')
#define JS7_ISOCT(c)    ((((unsigned)(c)) - '0') <= 7)
#define JS7_UNOCT(c)    (JS7_UNDEC(c))
#define JS7_ISHEX(c)    ((c) < 128 && isxdigit(c))
#define JS7_UNHEX(c)    (unsigned)(JS7_ISDEC(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define JS7_ISLET(c)    ((c) < 128 && isalpha(c))

extern size_t
js_strlen(const char16_t* s);

extern int32_t
js_strcmp(const char16_t* lhs, const char16_t* rhs);

template <typename CharT>
extern const CharT*
js_strchr_limit(const CharT* s, char16_t c, const CharT* limit);

static MOZ_ALWAYS_INLINE void
js_strncpy(char16_t* dst, const char16_t* src, size_t nelem)
{
    return mozilla::PodCopy(dst, src, nelem);
}

namespace js {


extern JSObject*
InitStringClass(JSContext* cx, HandleObject obj);




extern const char*
ValueToPrintable(JSContext* cx, const Value&, JSAutoByteString* bytes, bool asSource = false);

extern mozilla::UniquePtr<char[], JS::FreePolicy>
DuplicateString(ExclusiveContext* cx, const char* s);

extern mozilla::UniquePtr<char16_t[], JS::FreePolicy>
DuplicateString(ExclusiveContext* cx, const char16_t* s);





template <AllowGC allowGC>
extern JSString*
ToStringSlow(ExclusiveContext* cx, typename MaybeRooted<Value, allowGC>::HandleType arg);






template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSString*
ToString(JSContext* cx, JS::HandleValue v)
{
    if (v.isString())
        return v.toString();
    return ToStringSlow<allowGC>(cx, v);
}






inline bool
ValueToStringBuffer(JSContext* cx, const Value& v, StringBuffer& sb);





extern JSString*
ValueToSource(JSContext* cx, HandleValue v);






extern JSString*
StringToSource(JSContext* cx, JSString* str);





extern bool
EqualStrings(JSContext* cx, JSString* str1, JSString* str2, bool* result);


extern bool
EqualStrings(JSContext* cx, JSLinearString* str1, JSLinearString* str2, bool* result) = delete;


extern bool
EqualStrings(JSLinearString* str1, JSLinearString* str2);

extern bool
EqualChars(JSLinearString* str1, JSLinearString* str2);





extern bool
CompareStrings(JSContext* cx, JSString* str1, JSString* str2, int32_t* result);





extern int32_t
CompareAtoms(JSAtom* atom1, JSAtom* atom2);




extern bool
StringEqualsAscii(JSLinearString* str, const char* asciiBytes);


extern bool
StringHasPattern(JSLinearString* text, const char16_t* pat, uint32_t patlen);

extern int
StringFindPattern(JSLinearString* text, JSLinearString* pat, size_t start);

template <typename CharT>
extern bool
HasRegExpMetaChars(const CharT* chars, size_t length);

extern bool
StringHasRegExpMetaChars(JSLinearString* str);

template <typename Char1, typename Char2>
inline bool
EqualChars(const Char1* s1, const Char2* s2, size_t len);

template <typename Char1>
inline bool
EqualChars(const Char1* s1, const Char1* s2, size_t len)
{
    return mozilla::PodEqual(s1, s2, len);
}

template <typename Char1, typename Char2>
inline bool
EqualChars(const Char1* s1, const Char2* s2, size_t len)
{
    for (const Char1* s1end = s1 + len; s1 < s1end; s1++, s2++) {
        if (*s1 != *s2)
            return false;
    }
    return true;
}






JSString*
SubstringKernel(JSContext* cx, HandleString str, int32_t beginInt, int32_t lengthInt);







extern char16_t*
InflateString(ExclusiveContext* cx, const char* bytes, size_t* length);





inline void
CopyAndInflateChars(char16_t* dst, const char* src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i++)
        dst[i] = (unsigned char) src[i];
}

inline void
CopyAndInflateChars(char16_t* dst, const JS::Latin1Char* src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i++)
        dst[i] = src[i];
}







template <typename CharT>
extern bool
DeflateStringToBuffer(JSContext* maybecx, const CharT* chars,
                      size_t charsLength, char* bytes, size_t* length);





extern bool
str_replace(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
str_fromCharCode(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_fromCharCode_one_arg(JSContext* cx, HandleValue code, MutableHandleValue rval);



extern bool
str_indexOf(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_lastIndexOf(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_startsWith(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_toLowerCase(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_toUpperCase(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_toString(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_charAt(JSContext* cx, unsigned argc, Value* vp);

extern bool
str_charCodeAt_impl(JSContext* cx, HandleString string, HandleValue index, MutableHandleValue res);

extern bool
str_charCodeAt(JSContext* cx, unsigned argc, Value* vp);




extern int
OneUcs4ToUtf8Char(uint8_t* utf8Buffer, uint32_t ucs4Char);

extern size_t
PutEscapedStringImpl(char* buffer, size_t size, FILE* fp, JSLinearString* str, uint32_t quote);

template <typename CharT>
extern size_t
PutEscapedStringImpl(char* buffer, size_t bufferSize, FILE* fp, const CharT* chars,
                     size_t length, uint32_t quote);










inline size_t
PutEscapedString(char* buffer, size_t size, JSLinearString* str, uint32_t quote)
{
    size_t n = PutEscapedStringImpl(buffer, size, nullptr, str, quote);

    
    MOZ_ASSERT(n != size_t(-1));
    return n;
}

template <typename CharT>
inline size_t
PutEscapedString(char* buffer, size_t bufferSize, const CharT* chars, size_t length, uint32_t quote)
{
    size_t n = PutEscapedStringImpl(buffer, bufferSize, nullptr, chars, length, quote);

    
    MOZ_ASSERT(n != size_t(-1));
    return n;
}






inline bool
FileEscapedString(FILE* fp, JSLinearString* str, uint32_t quote)
{
    return PutEscapedStringImpl(nullptr, 0, fp, str, quote) != size_t(-1);
}

inline bool
FileEscapedString(FILE* fp, const char* chars, size_t length, uint32_t quote)
{
    return PutEscapedStringImpl(nullptr, 0, fp, chars, length, quote) != size_t(-1);
}

bool
str_match(JSContext* cx, unsigned argc, Value* vp);

bool
str_search(JSContext* cx, unsigned argc, Value* vp);

bool
str_split(JSContext* cx, unsigned argc, Value* vp);

JSObject*
str_split_string(JSContext* cx, HandleObjectGroup group, HandleString str, HandleString sep);

bool
str_replace_regexp_raw(JSContext* cx, HandleString string, HandleObject regexp,
                       HandleString replacement, MutableHandleValue rval);

bool
str_replace_string_raw(JSContext* cx, HandleString string, HandleString pattern,
                       HandleString replacement, MutableHandleValue rval);

extern bool
StringConstructor(JSContext* cx, unsigned argc, Value* vp);

} 

#endif 
