






#ifndef SkBlitRow_DEFINED
#define SkBlitRow_DEFINED

#include "SkBitmap.h"
#include "SkColor.h"

class SkBlitRow {
public:
    enum Flags16 {
        
        kGlobalAlpha_Flag   = 0x01,
        
        kSrcPixelAlpha_Flag = 0x02,
        
        kDither_Flag        = 0x04
    };

    









    typedef void (*Proc)(uint16_t* dst,
                         const SkPMColor* src,
                         int count, U8CPU alpha, int x, int y);

   


   typedef void (*ColorProc)(SkPMColor* dst, const SkPMColor* src, int count,
                             SkPMColor color);

    
    static Proc Factory(unsigned flags, SkBitmap::Config);

    

    enum Flags32 {
        kGlobalAlpha_Flag32     = 1 << 0,
        kSrcPixelAlpha_Flag32   = 1 << 1,
    };

    





    typedef void (*Proc32)(uint32_t* dst,
                         const SkPMColor* src,
                         int count, U8CPU alpha);

    static Proc32 Factory32(unsigned flags32);

    



    static void Color32(SkPMColor dst[], const SkPMColor src[],
                        int count, SkPMColor color);

    static ColorProc ColorProcFactory();

    





    static Proc32 PlatformProcs32(unsigned flags);
    static Proc PlatformProcs565(unsigned flags);
    static Proc PlatformProcs4444(unsigned flags);
    static ColorProc PlatformColorProc();

private:
    enum {
        kFlags16_Mask = 7,
        kFlags32_Mask = 3
    };
};

#endif
