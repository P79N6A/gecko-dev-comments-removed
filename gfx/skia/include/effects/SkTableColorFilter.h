
#ifndef SkTableColorFilter_DEFINED
#define SkTableColorFilter_DEFINED

#include "SkColorFilter.h"

class SkTableColorFilter {
public:
    










    static SkColorFilter* Create(const uint8_t table[256]);
    
    





    static SkColorFilter* CreateARGB(const uint8_t tableA[256],
                                     const uint8_t tableR[256],
                                     const uint8_t tableG[256],
                                     const uint8_t tableB[256]);
};

#endif
