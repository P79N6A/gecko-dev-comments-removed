








#ifndef SkPackBits_DEFINED
#define SkPackBits_DEFINED

#include "SkTypes.h"

class SkPackBits {
public:
    


    static size_t ComputeMaxSize16(int count);

    


    static size_t ComputeMaxSize8(int count);

    







    static size_t Pack16(const uint16_t src[], int count, uint8_t dst[]);

    







    static size_t Pack8(const uint8_t src[], int count, uint8_t dst[]);

    






    static int Unpack16(const uint8_t src[], size_t srcSize, uint16_t dst[]);

    






    static int Unpack8(const uint8_t src[], size_t srcSize, uint8_t dst[]);

    









    static void Unpack8(uint8_t dst[], size_t dstSkip, size_t dstWrite,
                        const uint8_t src[]);
};

#endif
