






#ifndef GrTextureAccess_DEFINED
#define GrTextureAccess_DEFINED

#include "GrNoncopyable.h"
#include "SkRefCnt.h"

class GrTexture;







class GrTextureAccess : GrNoncopyable {
public:
    



    GrTextureAccess();

    



    GrTextureAccess(GrTexture*, const char* swizzle);

    


    GrTextureAccess(GrTexture*);

    void reset(GrTexture*, const char* swizzle);
    void reset(GrTexture*);

    GrTexture* getTexture() const { return fTexture.get(); }

    


    const char* getSwizzle() const { return fSwizzle; }

    enum {
        kR_SwizzleFlag = 0x1,
        kG_SwizzleFlag = 0x2,
        kB_SwizzleFlag = 0x4,
        kA_SwizzleFlag = 0x8,

        kRGB_SwizzleMask = (kR_SwizzleFlag |  kG_SwizzleFlag | kB_SwizzleFlag),
    };

    
    uint32_t swizzleMask() const { return fSwizzleMask; }

private:
    SkAutoTUnref<GrTexture> fTexture;
    uint32_t                fSwizzleMask;
    char                    fSwizzle[5];
};

#endif
