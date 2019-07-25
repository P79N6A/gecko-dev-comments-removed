








#ifndef SkBitmapProcState_DEFINED
#define SkBitmapProcState_DEFINED

#include "SkBitmap.h"
#include "SkMatrix.h"

class SkPaint;

struct SkBitmapProcState {

    typedef void (*ShaderProc32)(const SkBitmapProcState&, int x, int y,
                                 SkPMColor[], int count);

    typedef void (*ShaderProc16)(const SkBitmapProcState&, int x, int y,
                                 uint16_t[], int count);

    typedef void (*MatrixProc)(const SkBitmapProcState&,
                               uint32_t bitmapXY[],
                               int count,
                               int x, int y);
    
    typedef void (*SampleProc32)(const SkBitmapProcState&,
                                 const uint32_t[],
                                 int count,
                                 SkPMColor colors[]);

    typedef void (*SampleProc16)(const SkBitmapProcState&,
                                 const uint32_t[],
                                 int count,
                                 uint16_t colors[]);
    
    typedef U16CPU (*FixedTileProc)(SkFixed);   
    typedef U16CPU (*IntTileProc)(int value, int count);   

    
    
    ShaderProc32        fShaderProc32;      
    ShaderProc16        fShaderProc16;      
    
    MatrixProc          fMatrixProc;        
    SampleProc32        fSampleProc32;      
    SampleProc16        fSampleProc16;      

    const SkBitmap*     fBitmap;            
    const SkMatrix*     fInvMatrix;         
    SkMatrix::MapXYProc fInvProc;           

    FixedTileProc       fTileProcX;         
    FixedTileProc       fTileProcY;         
    IntTileProc         fIntTileProcY;      
    SkFixed             fFilterOneX;
    SkFixed             fFilterOneY;

    SkPMColor           fPaintPMColor;      
    SkFixed             fInvSx;             
    SkFixed             fInvKy;             
    uint16_t            fAlphaScale;        
    uint8_t             fInvType;           
    uint8_t             fTileModeX;         
    uint8_t             fTileModeY;         
    SkBool8             fDoFilter;          

    












    void platformProcs();

    








    int maxCountForBufferSize(size_t bufferSize) const;

private:
    friend class SkBitmapProcShader;

    SkMatrix            fUnitInvMatrix;     
    SkBitmap            fOrigBitmap;        
    SkBitmap            fMipBitmap;

    MatrixProc chooseMatrixProc(bool trivial_matrix);
    bool chooseProcs(const SkMatrix& inv, const SkPaint&);
};





#ifdef SK_CPU_BENDIAN
    #define PACK_TWO_SHORTS(pri, sec) ((pri) << 16 | (sec))
    #define UNPACK_PRIMARY_SHORT(packed)    ((uint32_t)(packed) >> 16)
    #define UNPACK_SECONDARY_SHORT(packed)  ((packed) & 0xFFFF)
#else
    #define PACK_TWO_SHORTS(pri, sec) ((pri) | ((sec) << 16))
    #define UNPACK_PRIMARY_SHORT(packed)    ((packed) & 0xFFFF)
    #define UNPACK_SECONDARY_SHORT(packed)  ((uint32_t)(packed) >> 16)
#endif

#ifdef SK_DEBUG
    static inline uint32_t pack_two_shorts(U16CPU pri, U16CPU sec) {
        SkASSERT((uint16_t)pri == pri);
        SkASSERT((uint16_t)sec == sec);
        return PACK_TWO_SHORTS(pri, sec);
    }
#else
    #define pack_two_shorts(pri, sec)   PACK_TWO_SHORTS(pri, sec)
#endif



void S32_opaque_D32_filter_DX(const SkBitmapProcState& s, const uint32_t xy[],
                              int count, SkPMColor colors[]);
void S32_alpha_D32_filter_DX(const SkBitmapProcState& s, const uint32_t xy[],
                             int count, SkPMColor colors[]);

#endif
