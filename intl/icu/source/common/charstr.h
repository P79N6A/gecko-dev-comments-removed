










#ifndef CHARSTRING_H
#define CHARSTRING_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/uobject.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN



#if !U_PLATFORM_IS_DARWIN_BASED
template class U_COMMON_API MaybeStackArray<char, 40>;
#endif












class U_COMMON_API CharString : public UMemory {
public:
    CharString() : len(0) { buffer[0]=0; }
    CharString(const StringPiece &s, UErrorCode &errorCode) : len(0) {
        buffer[0]=0;
        append(s, errorCode);
    }
    CharString(const CharString &s, UErrorCode &errorCode) : len(0) {
        buffer[0]=0;
        append(s, errorCode);
    }
    CharString(const char *s, int32_t sLength, UErrorCode &errorCode) : len(0) {
        buffer[0]=0;
        append(s, sLength, errorCode);
    }
    ~CharString() {}

    





    CharString &copyFrom(const CharString &other, UErrorCode &errorCode);

    UBool isEmpty() const { return len==0; }
    int32_t length() const { return len; }
    char operator[](int32_t index) const { return buffer[index]; }
    StringPiece toStringPiece() const { return StringPiece(buffer.getAlias(), len); }

    const char *data() const { return buffer.getAlias(); }
    char *data() { return buffer.getAlias(); }

    CharString &clear() { len=0; buffer[0]=0; return *this; }
    CharString &truncate(int32_t newLength);

    CharString &append(char c, UErrorCode &errorCode);
    CharString &append(const StringPiece &s, UErrorCode &errorCode) {
        return append(s.data(), s.length(), errorCode);
    }
    CharString &append(const CharString &s, UErrorCode &errorCode) {
        return append(s.data(), s.length(), errorCode);
    }
    CharString &append(const char *s, int32_t sLength, UErrorCode &status);
    



















    char *getAppendBuffer(int32_t minCapacity,
                          int32_t desiredCapacityHint,
                          int32_t &resultCapacity,
                          UErrorCode &errorCode);

    CharString &appendInvariantChars(const UnicodeString &s, UErrorCode &errorCode);

    




    CharString &appendPathPart(const StringPiece &s, UErrorCode &errorCode);

private:
    MaybeStackArray<char, 40> buffer;
    int32_t len;

    UBool ensureCapacity(int32_t capacity, int32_t desiredCapacityHint, UErrorCode &errorCode);

    CharString(const CharString &other); 
    CharString &operator=(const CharString &other); 
};

U_NAMESPACE_END

#endif

