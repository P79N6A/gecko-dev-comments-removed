






#ifndef SkImageGenerator_DEFINED
#define SkImageGenerator_DEFINED

#include "SkImageInfo.h"
#include "SkColor.h"

class SkBitmap;
class SkData;
class SkImageGenerator;



















SK_API bool SkInstallDiscardablePixelRef(SkImageGenerator*, SkBitmap* destination);





SK_API void SkPurgeGlobalDiscardableMemoryPool();






class SK_API SkImageGenerator {
public:
    



    virtual ~SkImageGenerator() { }

#ifdef SK_SUPPORT_LEGACY_IMAGEGENERATORAPI
    virtual SkData* refEncodedData() { return this->onRefEncodedData(); }
    virtual bool getInfo(SkImageInfo* info) { return this->onGetInfo(info); }
    virtual bool getPixels(const SkImageInfo& info, void* pixels, size_t rowBytes) {
        return this->onGetPixels(info, pixels, rowBytes, NULL, NULL);
    }
#else
    






    SkData* refEncodedData() { return this->onRefEncodedData(); }

    








    bool getInfo(SkImageInfo* info);

    

























    bool getPixels(const SkImageInfo& info, void* pixels, size_t rowBytes,
                   SkPMColor ctable[], int* ctableCount);

    


    bool getPixels(const SkImageInfo& info, void* pixels, size_t rowBytes);
#endif

    










    bool getYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]);

protected:
    virtual SkData* onRefEncodedData();
    virtual bool onGetInfo(SkImageInfo* info);
    virtual bool onGetPixels(const SkImageInfo& info,
                             void* pixels, size_t rowBytes,
                             SkPMColor ctable[], int* ctableCount);
    virtual bool onGetYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]);
};

#endif  
