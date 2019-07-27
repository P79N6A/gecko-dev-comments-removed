






#ifndef SkUtils_DEFINED
#define SkUtils_DEFINED

#include "SkTypes.h"








void sk_memset16(uint16_t dst[], uint16_t value, int count);
typedef void (*SkMemset16Proc)(uint16_t dst[], uint16_t value, int count);
SkMemset16Proc SkMemset16GetPlatformProc();






void sk_memset32(uint32_t dst[], uint32_t value, int count);
typedef void (*SkMemset32Proc)(uint32_t dst[], uint32_t value, int count);
SkMemset32Proc SkMemset32GetPlatformProc();






void sk_memcpy32(uint32_t dst[], const uint32_t src[], int count);
typedef void (*SkMemcpy32Proc)(uint32_t dst[], const uint32_t src[], int count);
SkMemcpy32Proc SkMemcpy32GetPlatformProc();



#define kMaxBytesInUTF8Sequence     4

#ifdef SK_DEBUG
    int SkUTF8_LeadByteToCount(unsigned c);
#else
    #define SkUTF8_LeadByteToCount(c)   ((((0xE5 << 24) >> ((unsigned)c >> 4 << 1)) & 3) + 1)
#endif

inline int SkUTF8_CountUTF8Bytes(const char utf8[]) {
    SkASSERT(utf8);
    return SkUTF8_LeadByteToCount(*(const uint8_t*)utf8);
}

int         SkUTF8_CountUnichars(const char utf8[]);
int         SkUTF8_CountUnichars(const char utf8[], size_t byteLength);
SkUnichar   SkUTF8_ToUnichar(const char utf8[]);
SkUnichar   SkUTF8_NextUnichar(const char**);
SkUnichar   SkUTF8_PrevUnichar(const char**);





size_t      SkUTF8_FromUnichar(SkUnichar uni, char utf8[] = NULL);



#define SkUTF16_IsHighSurrogate(c)  (((c) & 0xFC00) == 0xD800)
#define SkUTF16_IsLowSurrogate(c)   (((c) & 0xFC00) == 0xDC00)

int SkUTF16_CountUnichars(const uint16_t utf16[]);
int SkUTF16_CountUnichars(const uint16_t utf16[], int numberOf16BitValues);

SkUnichar SkUTF16_NextUnichar(const uint16_t**);

SkUnichar SkUTF16_PrevUnichar(const uint16_t**);
size_t SkUTF16_FromUnichar(SkUnichar uni, uint16_t utf16[] = NULL);

size_t SkUTF16_ToUTF8(const uint16_t utf16[], int numberOf16BitValues,
                      char utf8[] = NULL);

inline bool SkUnichar_IsVariationSelector(SkUnichar uni) {





    if (uni < 0x180B || uni > 0xE01EF) {
        return false;
    }
    if ((uni > 0x180D && uni < 0xFE00) || (uni > 0xFE0F && uni < 0xE0100)) {
        return false;
    }
    return true;
}



class SkAutoTrace {
public:
    


    SkAutoTrace(const char label[]) : fLabel(label) {
        SkDebugf("--- trace: %s Enter\n", fLabel);
    }
    ~SkAutoTrace() {
        SkDebugf("--- trace: %s Leave\n", fLabel);
    }
private:
    const char* fLabel;
};
#define SkAutoTrace(...) SK_REQUIRE_LOCAL_VAR(SkAutoTrace)

#endif
