

























#include "jit/arm64/vixl/Utils-vixl.h"

#include "mozilla/MathAlgorithms.h"

#include <stdio.h>

namespace vixl {

uint32_t
float_to_rawbits(float value)
{
    uint32_t bits = 0;
    memcpy(&bits, &value, 4);
    return bits;
}


uint64_t
double_to_rawbits(double value)
{
    uint64_t bits = 0;
    memcpy(&bits, &value, 8);
    return bits;
}

float
rawbits_to_float(uint32_t bits)
{
    float value = 0.0;
    memcpy(&value, &bits, 4);
    return value;
}

double
rawbits_to_double(uint64_t bits)
{
    double value = 0.0;
    memcpy(&value, &bits, 8);
    return value;
}

int
CountLeadingZeros(uint64_t value, int width)
{
    MOZ_ASSERT((width == 32) || (width == 64));
    int count = 0;
    uint64_t bit_test = UINT64_C(1) << (width - 1);
    while ((count < width) && ((bit_test & value) == 0)) {
        count++;
        bit_test >>= 1;
    }
    return count;
}

int
CountLeadingSignBits(int64_t value, int width)
{
    MOZ_ASSERT((width == 32) || (width == 64));
    if (value >= 0)
        return CountLeadingZeros(value, width) - 1;
    return CountLeadingZeros(~value, width) - 1;
}

int
CountTrailingZeros(uint64_t value, int width)
{
    MOZ_ASSERT((width == 32) || (width == 64));
    int count = 0;
    while ((count < width) && (((value >> count) & 1) == 0))
        count++;
    return count;
}

int
CountSetBits(uint64_t value, int width)
{
    MOZ_ASSERT((width == 32) || (width == 64));

    if (width == 32)
        return mozilla::CountPopulation32(uint32_t(value));
    return mozilla::CountPopulation64(value);
}

uint64_t LowestSetBit(uint64_t value) {
  return value & -value;
}

} 
