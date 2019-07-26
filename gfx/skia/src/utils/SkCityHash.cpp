











#include "SkCityHash.h"
#include "SkTypes.h"
#include "city.h"

uint32_t SkCityHash::Compute32(const char *data, size_t size) {
    return CityHash32(data, size);
}

uint64_t SkCityHash::Compute64(const char *data, size_t size) {
    return CityHash64(data, size);
}
