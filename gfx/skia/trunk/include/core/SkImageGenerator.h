






#ifndef SkImageGenerator_DEFINED
#define SkImageGenerator_DEFINED

#include "SkDiscardableMemory.h"
#include "SkImageInfo.h"

class SkBitmap;
class SkData;
class SkImageGenerator;























SK_API bool SkInstallDiscardablePixelRef(SkImageGenerator* generator,
                                         SkBitmap* destination,
                                         SkDiscardableMemory::Factory* factory = NULL);






class SK_API SkImageGenerator {
public:
    



    virtual ~SkImageGenerator() { }

    






    virtual SkData* refEncodedData() { return NULL; }

    








    virtual bool getInfo(SkImageInfo* info) = 0;

    


















    virtual bool getPixels(const SkImageInfo& info,
                           void* pixels,
                           size_t rowBytes) = 0;
};

#endif  
