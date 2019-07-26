








#ifndef SkFlate_DEFINED
#define SkFlate_DEFINED

#include "SkTypes.h"

class SkData;
class SkWStream;
class SkStream;




class SkFlate {
public:
    

    static bool HaveFlate();

    



    static bool Deflate(SkStream* src, SkWStream* dst);

    



    static bool Deflate(const void* src, size_t len, SkWStream* dst);

    



    static bool Deflate(const SkData*, SkWStream* dst);

    


    static bool Inflate(SkStream* src, SkWStream* dst);
};

#endif
