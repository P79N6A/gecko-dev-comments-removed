






#ifndef SkMD5_DEFINED
#define SkMD5_DEFINED

#include "SkTypes.h"
#include "SkEndian.h"
#include "SkStream.h"






class SkMD5 : SkWStream {
public:
    SkMD5();

    


    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE {
        update(reinterpret_cast<const uint8_t*>(buffer), size);
        return true;
    }

    
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
