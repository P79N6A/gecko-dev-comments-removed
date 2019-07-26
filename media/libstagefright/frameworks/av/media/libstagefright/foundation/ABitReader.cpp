















#include "ABitReader.h"

#include <log/log.h>
#include <media/stagefright/foundation/ADebug.h>

namespace android {

ABitReader::ABitReader(const uint8_t *data, size_t size)
    : mData(data),
      mSize(size),
      mReservoir(0),
      mNumBitsLeft(0) {
}

void ABitReader::fillReservoir() {
    CHECK_GT(mSize, 0u);

    mReservoir = 0;
    size_t i;
    for (i = 0; mSize > 0 && i < 4; ++i) {
        mReservoir = (mReservoir << 8) | *mData;

        ++mData;
        --mSize;
    }

    mNumBitsLeft = 8 * i;
    mReservoir <<= 32 - mNumBitsLeft;
}

uint32_t ABitReader::getBits(size_t n) {
    CHECK_LE(n, 32u);

    uint32_t result = 0;
    while (n > 0) {
        if (mNumBitsLeft == 0) {
            fillReservoir();
        }

        size_t m = n;
        if (m > mNumBitsLeft) {
            m = mNumBitsLeft;
        }

        result = (result << m) | (mReservoir >> (32 - m));
        mReservoir <<= m;
        mNumBitsLeft -= m;

        n -= m;
    }

    return result;
}

void ABitReader::skipBits(size_t n) {
    while (n > 32) {
        getBits(32);
        n -= 32;
    }

    if (n > 0) {
        getBits(n);
    }
}

void ABitReader::putBits(uint32_t x, size_t n) {
    CHECK_LE(n, 32u);

    while (mNumBitsLeft + n > 32) {
        mNumBitsLeft -= 8;
        --mData;
        ++mSize;
    }

    mReservoir = (mReservoir >> n) | (x << (32 - n));
    mNumBitsLeft += n;
}

size_t ABitReader::numBitsLeft() const {
    return mSize * 8 + mNumBitsLeft;
}

const uint8_t *ABitReader::data() const {
    return mData - (mNumBitsLeft + 7) / 8;
}

}  
