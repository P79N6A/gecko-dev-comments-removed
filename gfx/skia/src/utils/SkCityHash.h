



















#ifndef SkCityHash_DEFINED
#define SkCityHash_DEFINED

#include "SkTypes.h"

class SkCityHash : SkNoncopyable {
public:
    






    static uint32_t Compute32(const char *data, size_t size);

    






    static uint64_t Compute64(const char *data, size_t size);
};

#endif
