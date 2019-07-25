








#ifndef SkPorterDuff_DEFINED
#define SkPorterDuff_DEFINED

#include "SkColor.h"
#include "SkXfermode.h"

class SkXfermode;



class SkPorterDuff {
public:
    







    enum Mode {
        kClear_Mode,    
        kSrc_Mode,      
        kDst_Mode,      
        kSrcOver_Mode,  
        kDstOver_Mode,  
        kSrcIn_Mode,    
        kDstIn_Mode,    
        kSrcOut_Mode,   
        kDstOut_Mode,   
        kSrcATop_Mode,  
        kDstATop_Mode,  
        kXor_Mode,      
        kDarken_Mode,   
        kLighten_Mode,  
        kMultiply_Mode, 
        kScreen_Mode,   
        kAdd_Mode,      
#ifdef ANDROID
        kOverlay_Mode,
#endif

        kModeCount
    };

    

    static SkXfermode* CreateXfermode(Mode mode);
    
    


    static SkXfermodeProc GetXfermodeProc(Mode mode);
    
    




    static SkXfermodeProc16 GetXfermodeProc16(Mode mode, SkColor srcColor);
    
    




    static bool IsMode(SkXfermode*, Mode* mode);

    

    static SkXfermode::Mode ToXfermodeMode(Mode);
};

#endif

