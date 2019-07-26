






#ifndef SkSHA1_DEFINED
#define SkSHA1_DEFINED

#include "SkTypes.h"
#include "SkEndian.h"
#include "SkStream.h"






class SkSHA1 : public SkWStream {
public:
    SkSHA1();

    


    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE {
        update(reinterpret_cast<const uint8_t*>(buffer), size);
        return true;
    }

    virtual size_t bytesWritten() const SK_OVERRIDE { return SkToSizeT(this->byteCount); }

    
    void update(const uint8_t* input, size_t length);

    struct Digest {
        uint8_t data[20];
    };

    
    void finish(Digest& digest);

private:
    
    uint64_t byteCount;

    
    uint32_t state[5];

    
    uint8_t buffer[64];
};

#endif
