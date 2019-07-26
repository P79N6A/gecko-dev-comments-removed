








#ifndef SkBase64_DEFINED
#define SkBase64_DEFINED

#include "SkTypes.h"

struct SkBase64 {
public:
    enum Error {
        kNoError,
        kPadError,
        kBadCharError
    };

    SkBase64();
    Error decode(const char* src, size_t length);
    char* getData() { return fData; }
    




    static size_t Encode(const void* src, size_t length, void* dest, const char* encode = NULL);

#ifdef SK_SUPPORT_UNITTEST
    static void UnitTest();
#endif
private:
    Error decode(const void* srcPtr, size_t length, bool writeDestination);

    size_t fLength;
    char* fData;
    friend class SkImageBaseBitmap;
};

#endif 
