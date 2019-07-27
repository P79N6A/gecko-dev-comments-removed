








#ifndef SkGraphics_DEFINED
#define SkGraphics_DEFINED

#include "SkTypes.h"

class SK_API SkGraphics {
public:
    




    static void Init();

    


    static void Term();

    



    static void GetVersion(int32_t* major, int32_t* minor, int32_t* patch);

    




    static size_t GetFontCacheLimit();

    






    static size_t SetFontCacheLimit(size_t bytes);

    


    static size_t GetFontCacheUsed();

    



    static int GetFontCacheCountUsed();

    



    static int GetFontCacheCountLimit();

    




    static int SetFontCacheCountLimit(int count);

    




    static void PurgeFontCache();

    






    static size_t GetImageCacheTotalBytesUsed();
    




    static size_t GetImageCacheTotalByteLimit();
    static size_t SetImageCacheTotalByteLimit(size_t newLimit);

    
    static size_t GetImageCacheBytesUsed() {
        return GetImageCacheTotalBytesUsed();
    }
    
    static size_t GetImageCacheByteLimit() {
        return GetImageCacheTotalByteLimit();
    }
    
    static size_t SetImageCacheByteLimit(size_t newLimit) {
        return SetImageCacheTotalByteLimit(newLimit);
    }

    









    static size_t GetImageCacheSingleAllocationByteLimit();
    static size_t SetImageCacheSingleAllocationByteLimit(size_t newLimit);

    







    static void SetFlags(const char* flags);

    










    static size_t GetTLSFontCacheLimit();

    




    static void SetTLSFontCacheLimit(size_t bytes);

private:
    



    static void InstallNewHandler();
};

class SkAutoGraphics {
public:
    SkAutoGraphics() {
        SkGraphics::Init();
    }
    ~SkAutoGraphics() {
        SkGraphics::Term();
    }
};

#endif
