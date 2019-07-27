






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

    static Proc Factory(unsigned flags, SkColorType);

    

    enum Flags32 {
        kGlobalAlpha_Flag32     = 1 << 0,
        kSrcPixelAlpha_Flag32   = 1 << 1
    };

    





    typedef void (*Proc32)(uint32_t* dst,
                         const SkPMColor* src,
                         int count, U8CPU alpha);

    static Proc32 Factory32(unsigned flags32);

   


   typedef void (*ColorProc)(SkPMColor* dst, const SkPMColor* src, int count,
                             SkPMColor color);

    



    static void Color32(SkPMColor dst[], const SkPMColor src[],
                        int count, SkPMColor color);

    
    static ColorProc ColorProcFactory();

    
    typedef void (*ColorRectProc)(SkPMColor* dst, int width, int height,
                                  size_t rowBytes, SkPMColor color);

    
    static void ColorRect32(SkPMColor* dst, int width, int height,
                            size_t rowBytes, SkPMColor color);

    
    static ColorRectProc ColorRectProcFactory();

    





    static Proc32 PlatformProcs32(unsigned flags);
    static Proc PlatformProcs565(unsigned flags);
    static ColorProc PlatformColorProc();

private:
    enum {
        kFlags16_Mask = 7,
        kFlags32_Mask = 3
    };
};

#endif
