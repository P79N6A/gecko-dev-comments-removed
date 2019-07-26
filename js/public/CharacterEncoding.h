





#ifndef js_CharacterEncoding_h
#define js_CharacterEncoding_h

#include "mozilla/NullPtr.h"
#include "mozilla/Range.h"

#include "js/TypeDecls.h"
#include "js/Utility.h"

namespace js {
struct ThreadSafeContext;
}

namespace JS {







class Latin1Chars : public mozilla::Range<unsigned char>
{
    typedef mozilla::Range<unsigned char> Base;

  public:
    Latin1Chars() : Base() {}
    Latin1Chars(char *aBytes, size_t aLength) : Base(reinterpret_cast<unsigned char *>(aBytes), aLength) {}
    Latin1Chars(const char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(const_cast<char *>(aBytes)), aLength)
    {}
};




class Latin1CharsZ : public mozilla::RangedPtr<unsigned char>
{
    typedef mozilla::RangedPtr<unsigned char> Base;

  public:
    Latin1CharsZ() : Base(nullptr, 0) {}

    Latin1CharsZ(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(aBytes), aLength)
    {
        MOZ_ASSERT(aBytes[aLength] == '\0');
    }

    Latin1CharsZ(unsigned char *aBytes, size_t aLength)
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









class TwoByteChars : public mozilla::Range<jschar>
{
    typedef mozilla::Range<jschar> Base;

  public:
    TwoByteChars() : Base() {}
    TwoByteChars(jschar *aChars, size_t aLength) : Base(aChars, aLength) {}
    TwoByteChars(const jschar *aChars, size_t aLength) : Base(const_cast<jschar *>(aChars), aLength) {}
};




class TwoByteCharsZ : public mozilla::RangedPtr<jschar>
{
    typedef mozilla::RangedPtr<jschar> Base;

  public:
    TwoByteCharsZ() : Base(nullptr, 0) {}

    TwoByteCharsZ(jschar *chars, size_t length)
      : Base(chars, length)
    {
        MOZ_ASSERT(chars[length] == '\0');
    }

    using Base::operator=;
};

typedef mozilla::RangedPtr<const jschar> ConstCharPtr;




class ConstTwoByteChars : public mozilla::RangedPtr<const jschar>
{
  public:
    ConstTwoByteChars(const ConstTwoByteChars &s) : ConstCharPtr(s) {}
    MOZ_IMPLICIT ConstTwoByteChars(const mozilla::RangedPtr<const jschar> &s) : ConstCharPtr(s) {}
    ConstTwoByteChars(const jschar *s, size_t len) : ConstCharPtr(s, len) {}
    ConstTwoByteChars(const jschar *pos, const jschar *start, size_t len)
      : ConstCharPtr(pos, start, len)
    {}

    using ConstCharPtr::operator=;
};












extern Latin1CharsZ
LossyTwoByteCharsToNewLatin1CharsZ(js::ThreadSafeContext *cx, TwoByteChars tbchars);

extern UTF8CharsZ
TwoByteCharsToNewUTF8CharsZ(js::ThreadSafeContext *cx, TwoByteChars tbchars);

uint32_t
Utf8ToOneUcs4Char(const uint8_t *utf8Buffer, int utf8Length);







extern TwoByteCharsZ
UTF8CharsToNewTwoByteCharsZ(JSContext *cx, const UTF8Chars utf8, size_t *outlen);






extern TwoByteCharsZ
LossyUTF8CharsToNewTwoByteCharsZ(JSContext *cx, const UTF8Chars utf8, size_t *outlen);

} 

inline void JS_free(JS::Latin1CharsZ &ptr) { js_free((void*)ptr.get()); }
inline void JS_free(JS::UTF8CharsZ &ptr) { js_free((void*)ptr.get()); }

#endif 
