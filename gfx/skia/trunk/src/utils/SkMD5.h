






#ifndef SkMD5_DEFINED
#define SkMD5_DEFINED

#include "SkTypes.h"
#include "SkEndian.h"
#include "SkStream.h"






class SkMD5 : public SkWStream {
public:
    SkMD5();

    


    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE {
        this->update(reinterpret_cast<const uint8_t*>(buffer), size);
        return true;
    }

    virtual size_t bytesWritten() const SK_OVERRIDE { return SkToSizeT(this->byteCount); }

    
    void update(const uint8_t* input, size_t length);

    struct Digest {
        uint8_t data[16];
    };

    
    void finish(Digest& digest);

private:
    
    uint64_t byteCount;

    
    uint32_t state[4];

    
    uint8_t buffer[64];
};

#endif
