









#include "SkTypes.h"
#include "SkMD5.h"
#include <string.h>


static void transform(uint32_t state[4], const uint8_t block[64]);


static void encode(uint8_t output[16], const uint32_t input[4]);


static void encode(uint8_t output[8], const uint64_t input);


static const uint32_t* decode(uint32_t storage[16], const uint8_t input[64]);

SkMD5::SkMD5() : byteCount(0) {
    
    this->state[0] = 0x67452301;
    this->state[1] = 0xefcdab89;
    this->state[2] = 0x98badcfe;
    this->state[3] = 0x10325476;
}

void SkMD5::update(const uint8_t* input, size_t inputLength) {
    unsigned int bufferIndex = (unsigned int)(this->byteCount & 0x3F);
    unsigned int bufferAvailable = 64 - bufferIndex;

    unsigned int inputIndex;
    if (inputLength >= bufferAvailable) {
        if (bufferIndex) {
            memcpy(&this->buffer[bufferIndex], input, bufferAvailable);
            transform(this->state, this->buffer);
            inputIndex = bufferAvailable;
        } else {
            inputIndex = 0;
        }

        for (; inputIndex + 63 < inputLength; inputIndex += 64) {
            transform(this->state, &input[inputIndex]);
        }

        bufferIndex = 0;
    } else {
        inputIndex = 0;
    }

    memcpy(&this->buffer[bufferIndex], &input[inputIndex], inputLength - inputIndex);

    this->byteCount += inputLength;
}

void SkMD5::finish(Digest& digest) {
    
    uint8_t bits[8];
    encode(bits, this->byteCount << 3);

    
    unsigned int bufferIndex = (unsigned int)(this->byteCount & 0x3F);
    unsigned int paddingLength = (bufferIndex < 56) ? (56 - bufferIndex) : (120 - bufferIndex);
    static uint8_t PADDING[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    this->update(PADDING, paddingLength);

    
    this->update(bits, 8);

    
    encode(digest.data, this->state);

#if defined(SK_MD5_CLEAR_DATA)
    
    memset(this, 0, sizeof(*this));
#endif
}

struct F { uint32_t operator()(uint32_t x, uint32_t y, uint32_t z) {
    
    return ((y ^ z) & x) ^ z; 
}};

struct G { uint32_t operator()(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & (~z));
    
}};

struct H { uint32_t operator()(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}};

struct I { uint32_t operator()(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | (~z));
}};


static inline uint32_t rotate_left(uint32_t x, uint8_t n) {
    return (x << n) | (x >> (32 - n));
}

template <typename T>
static inline void operation(T operation, uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
                             uint32_t x, uint8_t s, uint32_t t) {
    a = b + rotate_left(a + operation(b, c, d) + x + t, s);
}

static void transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];

    uint32_t storage[16];
    const uint32_t* X = decode(storage, block);

    
    operation(F(), a, b, c, d, X[ 0],  7, 0xd76aa478); 
    operation(F(), d, a, b, c, X[ 1], 12, 0xe8c7b756); 
    operation(F(), c, d, a, b, X[ 2], 17, 0x242070db); 
    operation(F(), b, c, d, a, X[ 3], 22, 0xc1bdceee); 
    operation(F(), a, b, c, d, X[ 4],  7, 0xf57c0faf); 
    operation(F(), d, a, b, c, X[ 5], 12, 0x4787c62a); 
    operation(F(), c, d, a, b, X[ 6], 17, 0xa8304613); 
    operation(F(), b, c, d, a, X[ 7], 22, 0xfd469501); 
    operation(F(), a, b, c, d, X[ 8],  7, 0x698098d8); 
    operation(F(), d, a, b, c, X[ 9], 12, 0x8b44f7af); 
    operation(F(), c, d, a, b, X[10], 17, 0xffff5bb1); 
    operation(F(), b, c, d, a, X[11], 22, 0x895cd7be); 
    operation(F(), a, b, c, d, X[12],  7, 0x6b901122); 
    operation(F(), d, a, b, c, X[13], 12, 0xfd987193); 
    operation(F(), c, d, a, b, X[14], 17, 0xa679438e); 
    operation(F(), b, c, d, a, X[15], 22, 0x49b40821); 

    
    operation(G(), a, b, c, d, X[ 1],  5, 0xf61e2562); 
    operation(G(), d, a, b, c, X[ 6],  9, 0xc040b340); 
    operation(G(), c, d, a, b, X[11], 14, 0x265e5a51); 
    operation(G(), b, c, d, a, X[ 0], 20, 0xe9b6c7aa); 
    operation(G(), a, b, c, d, X[ 5],  5, 0xd62f105d); 
    operation(G(), d, a, b, c, X[10],  9,  0x2441453); 
    operation(G(), c, d, a, b, X[15], 14, 0xd8a1e681); 
    operation(G(), b, c, d, a, X[ 4], 20, 0xe7d3fbc8); 
    operation(G(), a, b, c, d, X[ 9],  5, 0x21e1cde6); 
    operation(G(), d, a, b, c, X[14],  9, 0xc33707d6); 
    operation(G(), c, d, a, b, X[ 3], 14, 0xf4d50d87); 
    operation(G(), b, c, d, a, X[ 8], 20, 0x455a14ed); 
    operation(G(), a, b, c, d, X[13],  5, 0xa9e3e905); 
    operation(G(), d, a, b, c, X[ 2],  9, 0xfcefa3f8); 
    operation(G(), c, d, a, b, X[ 7], 14, 0x676f02d9); 
    operation(G(), b, c, d, a, X[12], 20, 0x8d2a4c8a); 

    
    operation(H(), a, b, c, d, X[ 5],  4, 0xfffa3942); 
    operation(H(), d, a, b, c, X[ 8], 11, 0x8771f681); 
    operation(H(), c, d, a, b, X[11], 16, 0x6d9d6122); 
    operation(H(), b, c, d, a, X[14], 23, 0xfde5380c); 
    operation(H(), a, b, c, d, X[ 1],  4, 0xa4beea44); 
    operation(H(), d, a, b, c, X[ 4], 11, 0x4bdecfa9); 
    operation(H(), c, d, a, b, X[ 7], 16, 0xf6bb4b60); 
    operation(H(), b, c, d, a, X[10], 23, 0xbebfbc70); 
    operation(H(), a, b, c, d, X[13],  4, 0x289b7ec6); 
    operation(H(), d, a, b, c, X[ 0], 11, 0xeaa127fa); 
    operation(H(), c, d, a, b, X[ 3], 16, 0xd4ef3085); 
    operation(H(), b, c, d, a, X[ 6], 23,  0x4881d05); 
    operation(H(), a, b, c, d, X[ 9],  4, 0xd9d4d039); 
    operation(H(), d, a, b, c, X[12], 11, 0xe6db99e5); 
    operation(H(), c, d, a, b, X[15], 16, 0x1fa27cf8); 
    operation(H(), b, c, d, a, X[ 2], 23, 0xc4ac5665); 

    
    operation(I(), a, b, c, d, X[ 0],  6, 0xf4292244); 
    operation(I(), d, a, b, c, X[ 7], 10, 0x432aff97); 
    operation(I(), c, d, a, b, X[14], 15, 0xab9423a7); 
    operation(I(), b, c, d, a, X[ 5], 21, 0xfc93a039); 
    operation(I(), a, b, c, d, X[12],  6, 0x655b59c3); 
    operation(I(), d, a, b, c, X[ 3], 10, 0x8f0ccc92); 
    operation(I(), c, d, a, b, X[10], 15, 0xffeff47d); 
    operation(I(), b, c, d, a, X[ 1], 21, 0x85845dd1); 
    operation(I(), a, b, c, d, X[ 8],  6, 0x6fa87e4f); 
    operation(I(), d, a, b, c, X[15], 10, 0xfe2ce6e0); 
    operation(I(), c, d, a, b, X[ 6], 15, 0xa3014314); 
    operation(I(), b, c, d, a, X[13], 21, 0x4e0811a1); 
    operation(I(), a, b, c, d, X[ 4],  6, 0xf7537e82); 
    operation(I(), d, a, b, c, X[11], 10, 0xbd3af235); 
    operation(I(), c, d, a, b, X[ 2], 15, 0x2ad7d2bb); 
    operation(I(), b, c, d, a, X[ 9], 21, 0xeb86d391); 

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

#if defined(SK_MD5_CLEAR_DATA)
    
    if (X == &storage) {
        memset(storage, 0, sizeof(storage));
    }
#endif
}

static void encode(uint8_t output[16], const uint32_t input[4]) {
    for (size_t i = 0, j = 0; i < 4; i++, j += 4) {
        output[j  ] = (uint8_t) (input[i]        & 0xff);
        output[j+1] = (uint8_t)((input[i] >>  8) & 0xff);
        output[j+2] = (uint8_t)((input[i] >> 16) & 0xff);
        output[j+3] = (uint8_t)((input[i] >> 24) & 0xff);
    }
}

static void encode(uint8_t output[8], const uint64_t input) {
    output[0] = (uint8_t) (input        & 0xff);
    output[1] = (uint8_t)((input >>  8) & 0xff);
    output[2] = (uint8_t)((input >> 16) & 0xff);
    output[3] = (uint8_t)((input >> 24) & 0xff);
    output[4] = (uint8_t)((input >> 32) & 0xff);
    output[5] = (uint8_t)((input >> 40) & 0xff);
    output[6] = (uint8_t)((input >> 48) & 0xff);
    output[7] = (uint8_t)((input >> 56) & 0xff);
}

static inline bool is_aligned(const void *pointer, size_t byte_count) {
    return reinterpret_cast<uintptr_t>(pointer) % byte_count == 0;
}

static const uint32_t* decode(uint32_t storage[16], const uint8_t input[64]) {
#if defined(SK_CPU_LENDIAN) && defined(SK_CPU_FAST_UNALIGNED_ACCESS)
   return reinterpret_cast<const uint32_t*>(input);
#else
#if defined(SK_CPU_LENDIAN)
    if (is_aligned(input, 4)) {
        return reinterpret_cast<const uint32_t*>(input);
    }
#endif
    for (size_t i = 0, j = 0; j < 64; i++, j += 4) {
        storage[i] =  ((uint32_t)input[j  ])        |
                     (((uint32_t)input[j+1]) <<  8) |
                     (((uint32_t)input[j+2]) << 16) |
                     (((uint32_t)input[j+3]) << 24);
    }
    return storage;
#endif
}
