





#ifndef js_CharacterEncoding_h
#define js_CharacterEncoding_h

#include "mozilla/Range.h"

#include "jspubtd.h"

#include "js/Utility.h"

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
    Latin1CharsZ() : Base(NULL, 0) {}

    Latin1CharsZ(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(aBytes), aLength)
    {
        JS_ASSERT(aBytes[aLength] == '\0');
    }

    Latin1CharsZ(unsigned char *aBytes, size_t aLength)
      : Base(aBytes, aLength)
    {
        JS_ASSERT(aBytes[aLength] == '\0');
    }

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
    UTF8CharsZ() : Base(NULL, 0) {}

    UTF8CharsZ(char *aBytes, size_t aLength)
      : Base(reinterpret_cast<unsigned char *>(aBytes), aLength)
    {
        JS_ASSERT(aBytes[aLength] == '\0');
    }

    UTF8CharsZ(unsigned char *aBytes, size_t aLength)
      : Base(aBytes, aLength)
    {
        JS_ASSERT(aBytes[aLength] == '\0');
    }

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






class StableTwoByteChars : public mozilla::Range<jschar>
{
    typedef mozilla::Range<jschar> Base;

  public:
    StableTwoByteChars() : Base() {}
    StableTwoByteChars(jschar *aChars, size_t aLength) : Base(aChars, aLength) {}
    StableTwoByteChars(const jschar *aChars, size_t aLength) : Base(const_cast<jschar *>(aChars), aLength) {}
};




class TwoByteCharsZ : public mozilla::RangedPtr<jschar>
{
    typedef mozilla::RangedPtr<jschar> Base;

  public:
    TwoByteCharsZ() : Base(NULL, 0) {}

    TwoByteCharsZ(jschar *chars, size_t length)
      : Base(chars, length)
    {
        JS_ASSERT(chars[length] == '\0');
    }
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
