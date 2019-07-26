






#include "SkReader32.h"
#include "SkString.h"
#include "SkWriter32.h"





const char* SkReader32::readString(size_t* outLen) {
    size_t len = this->readInt();
    const void* ptr = this->peek();

    
    size_t alignedSize = SkAlign4(len + 1);
    this->skip(alignedSize);

    if (outLen) {
        *outLen = len;
    }
    return (const char*)ptr;
}

size_t SkReader32::readIntoString(SkString* copy) {
    size_t len;
    const char* ptr = this->readString(&len);
    if (copy) {
        copy->set(ptr, len);
    }
    return len;
}

void SkWriter32::writeString(const char str[], size_t len) {
    if (NULL == str) {
        str = "";
        len = 0;
    }
    if ((long)len < 0) {
        len = strlen(str);
    }

    
    uint32_t* ptr = this->reservePad(sizeof(uint32_t) + len + 1);
    *ptr = len;
    char* chars = (char*)(ptr + 1);
    memcpy(chars, str, len);
    chars[len] = '\0';
}

size_t SkWriter32::WriteStringSize(const char* str, size_t len) {
    if ((long)len < 0) {
        SkASSERT(str);
        len = strlen(str);
    }
    const size_t lenBytes = 4;    
    
    return SkAlign4(lenBytes + len + 1);
}

const size_t kMinBufferBytes = 4096;

void SkWriter32::growToAtLeast(size_t size) {
    const bool wasExternal = (fExternal != NULL) && (fData == fExternal);
    const size_t minCapacity = kMinBufferBytes +
        SkTMax(size, fCapacity + (fCapacity >> 1));

    
    fInternal.setCountExact(minCapacity);
    fData = fInternal.begin();
    fCapacity = fInternal.reserved();
    if (wasExternal) {
        
        memcpy(fData, fExternal, fUsed);
    }
}
