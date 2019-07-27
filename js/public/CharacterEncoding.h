





#ifndef js_CharacterEncoding_h
#define js_CharacterEncoding_h

#include "mozilla/NullPtr.h"
#include "mozilla/Range.h"

#include "js/TypeDecls.h"
#include "js/Utility.h"

namespace js {
struct ThreadSafeContext;
}

class JSFlatString;

namespace JS {







class Latin1Chars : public mozilla::Range<Latin1Char>
{
    typedef mozilla::Range<Latin1Char> Base;

  public:
    Latin1Chars() : Base() {}
    Latin1Chars(char *aBytes, size_t aLength) : Base(reinterpret_cast<Latin1Char *>(aBytes), aLength) {}
    Latin1Chars(const Latin1Char *aBytes, size_t aLength)
      : Base(const_cast<Latin1Char *>(aBytes), aLength)
    {}
    Latin1Chars(const char *aBytes, size_t aLength)
      : Base(reinterpret_cast<Latin1Char *>(const_cast<char *>(aBytes)), aLength)
    {}
};




class Latin1CharsZ : public mozilla::RangedPtr<Latin1Char>
{
    typedef mozilla::RangedPtr<Latin1Char> Base;

  public:
    Latin1CharsZ() : Base(nullptr, 0) {}

    Latin1CharsZ(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<Latin1Char *>(aBytes), aLength)
    {
        MOZ_ASSERT(aBytes[aLength] == '\0');
    }

    Latin1CharsZ(Latin1Char *aBytes, size_t aLength)
      : Base(aBytes, aLength)
    {
        MOZ_ASSERT(aBytes[aLength] == '\0');
    }

    using Base::operator=;

    char *c_str() { return reinterpret_cast<char *>(get()); }
};

class UTF8Chars : public mozilla::Range<unsigned char>
{
    typedef mozilla::Range<unsigned char> Base;

  public:
    UTF8Chars() : Base() {}
    UTF8Chars(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(aBytes), aLength)
    {}
    UTF8Chars(const char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(const_cast<char *>(aBytes)), aLength)
    {}
};




class UTF8CharsZ : public mozilla::RangedPtr<unsigned char>
{
    typedef mozilla::RangedPtr<unsigned char> Base;

  public:
    UTF8CharsZ() : Base(nullptr, 0) {}

    UTF8CharsZ(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(aBytes), aLength)
    {
        MOZ_ASSERT(aBytes[aLength] == '\0');
    }

    UTF8CharsZ(unsigned char *aBytes, size_t aLength)
      : Base(aBytes, aLength)
    {
        MOZ_ASSERT(aBytes[aLength] == '\0');
    }

    using Base::operator=;

    char *c_str() { return reinterpret_cast<char *>(get()); }
};









class TwoByteChars : public mozilla::Range<char16_t>
{
    typedef mozilla::Range<char16_t> Base;

  public:
    TwoByteChars() : Base() {}
    TwoByteChars(char16_t *aChars, size_t aLength) : Base(aChars, aLength) {}
    TwoByteChars(const char16_t *aChars, size_t aLength) : Base(const_cast<char16_t *>(aChars), aLength) {}
};




class TwoByteCharsZ : public mozilla::RangedPtr<char16_t>
{
    typedef mozilla::RangedPtr<char16_t> Base;

  public:
    TwoByteCharsZ() : Base(nullptr, 0) {}

    TwoByteCharsZ(char16_t *chars, size_t length)
      : Base(chars, length)
    {
        MOZ_ASSERT(chars[length] == '\0');
    }

    using Base::operator=;
};

typedef mozilla::RangedPtr<const char16_t> ConstCharPtr;




class ConstTwoByteChars : public mozilla::RangedPtr<const char16_t>
{
  public:
    ConstTwoByteChars(const ConstTwoByteChars &s) : ConstCharPtr(s) {}
    MOZ_IMPLICIT ConstTwoByteChars(const mozilla::RangedPtr<const char16_t> &s) : ConstCharPtr(s) {}
    ConstTwoByteChars(const char16_t *s, size_t len) : ConstCharPtr(s, len) {}
    ConstTwoByteChars(const char16_t *pos, const char16_t *start, size_t len)
      : ConstCharPtr(pos, start, len)
    {}

    using ConstCharPtr::operator=;
};











extern Latin1CharsZ
LossyTwoByteCharsToNewLatin1CharsZ(js::ThreadSafeContext *cx,
                                   const mozilla::Range<const char16_t> tbchars);

template <typename CharT>
extern UTF8CharsZ
CharsToNewUTF8CharsZ(js::ThreadSafeContext *cx, const mozilla::Range<const CharT> chars);

uint32_t
Utf8ToOneUcs4Char(const uint8_t *utf8Buffer, int utf8Length);







extern TwoByteCharsZ
UTF8CharsToNewTwoByteCharsZ(JSContext *cx, const UTF8Chars utf8, size_t *outlen);






extern TwoByteCharsZ
LossyUTF8CharsToNewTwoByteCharsZ(JSContext *cx, const UTF8Chars utf8, size_t *outlen);





JS_PUBLIC_API(size_t)
GetDeflatedUTF8StringLength(JSFlatString *s);





JS_PUBLIC_API(void)
DeflateStringToUTF8Buffer(JSFlatString *src, mozilla::RangedPtr<char> dst);

} 

inline void JS_free(JS::Latin1CharsZ &ptr) { js_free((void*)ptr.get()); }
inline void JS_free(JS::UTF8CharsZ &ptr) { js_free((void*)ptr.get()); }

#endif 
