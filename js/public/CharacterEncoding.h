






#ifndef js_CharacterEncoding_h___
#define js_CharacterEncoding_h___

#include "mozilla/Range.h"

#include "js/Utility.h"

#include "jspubtd.h"

namespace JS {







class Latin1Chars : public mozilla::Range<unsigned char>
{
    typedef mozilla::Range<unsigned char> Base;

  public:
    Latin1Chars() : Base() {}
    Latin1Chars(char *bytes, size_t length) : Base(reinterpret_cast<unsigned char *>(bytes), length) {}
};




class Latin1CharsZ : public mozilla::RangedPtr<unsigned char>
{
    typedef mozilla::RangedPtr<unsigned char> Base;

  public:
    Latin1CharsZ() : Base(NULL, 0) {}

    Latin1CharsZ(char *bytes, size_t length)
      : Base(reinterpret_cast<unsigned char *>(bytes), length)
    {
        JS_ASSERT(bytes[length] == '\0');
    }

    Latin1CharsZ(unsigned char *bytes, size_t length)
      : Base(bytes, length)
    {
        JS_ASSERT(bytes[length] == '\0');
    }

    char *c_str() { return reinterpret_cast<char *>(get()); }
};




class UTF8CharsZ : public mozilla::RangedPtr<unsigned char>
{
    typedef mozilla::RangedPtr<unsigned char> Base;

  public:
    UTF8CharsZ() : Base(NULL, 0) {}

    UTF8CharsZ(char *bytes, size_t length)
      : Base(reinterpret_cast<unsigned char *>(bytes), length)
    {
        JS_ASSERT(bytes[length] == '\0');
    }
};









class TwoByteChars : public mozilla::Range<jschar>
{
    typedef mozilla::Range<jschar> Base;

  public:
    TwoByteChars() : Base() {}
    TwoByteChars(jschar *chars, size_t length) : Base(chars, length) {}
    TwoByteChars(const jschar *chars, size_t length) : Base(const_cast<jschar *>(chars), length) {}
};




class TwoByteCharsZ : public mozilla::RangedPtr<jschar>
{
    typedef mozilla::RangedPtr<jschar> Base;

  public:
    TwoByteCharsZ(jschar *chars, size_t length)
      : Base(chars, length)
    {
        JS_ASSERT(chars[length] = '\0');
    }
};











extern Latin1CharsZ
LossyTwoByteCharsToNewLatin1CharsZ(JSContext *cx, TwoByteChars tbchars);

} 

inline void JS_free(JS::Latin1CharsZ &ptr) { js_free((void*)ptr.get()); }

#endif 
