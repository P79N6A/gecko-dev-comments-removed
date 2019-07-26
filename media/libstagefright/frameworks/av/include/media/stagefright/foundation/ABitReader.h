















#ifndef A_BIT_READER_H_

#define A_BIT_READER_H_

#include <media/stagefright/foundation/ABase.h>

#include <sys/types.h>
#include <stdint.h>

namespace android {

struct ABitReader {
    ABitReader(const uint8_t *data, size_t size);

    uint32_t getBits(size_t n);
    void skipBits(size_t n);

    void putBits(uint32_t x, size_t n);

    size_t numBitsLeft() const;

    const uint8_t *data() const;

private:
    const uint8_t *mData;
    size_t mSize;

    uint32_t mReservoir;  
    size_t mNumBitsLeft;

    void fillReservoir();

    DISALLOW_EVIL_CONSTRUCTORS(ABitReader);
};

}  

#endif  
