






#ifndef SkBitmapFactory_DEFINED
#define SkBitmapFactory_DEFINED

#include "SkImage.h"
#include "SkTypes.h"

class SkBitmap;
class SkData;
class SkImageCache;




class SkBitmapFactory {

public:
    


    struct Target {
        


        void*  fAddr;

        


        size_t fRowBytes;
    };

    


    typedef bool (*DecodeProc)(const void* data, size_t length, SkImage::Info*, const Target*);

    



    SkBitmapFactory(DecodeProc);

    ~SkBitmapFactory();

    



    void setImageCache(SkImageCache* cache);

    








    bool installPixelRef(SkData*, SkBitmap*);

    


    class CacheSelector : public SkRefCnt {

    public:
        




        virtual SkImageCache* selectCache(const SkImage::Info&) = 0;
    };

    



    void setCacheSelector(CacheSelector*);

private:
    DecodeProc     fDecodeProc;
    SkImageCache*  fImageCache;
    CacheSelector* fCacheSelector;
};

#endif 
