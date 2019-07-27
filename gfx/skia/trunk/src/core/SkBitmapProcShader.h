








#ifndef SkBitmapProcShader_DEFINED
#define SkBitmapProcShader_DEFINED

#include "SkShader.h"
#include "SkBitmapProcState.h"
#include "SkSmallAllocator.h"

class SkBitmapProcShader : public SkShader {
public:
    SkBitmapProcShader(const SkBitmap& src, TileMode tx, TileMode ty,
                       const SkMatrix* localMatrix = NULL);

    
    virtual bool isOpaque() const SK_OVERRIDE;
    virtual BitmapType asABitmap(SkBitmap*, SkMatrix*, TileMode*) const SK_OVERRIDE;

    virtual size_t contextSize() const SK_OVERRIDE;

    static bool CanDo(const SkBitmap&, TileMode tx, TileMode ty);

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBitmapProcShader)


    bool asNewEffect(GrContext*, const SkPaint&, const SkMatrix*, GrColor*, GrEffect**)
            const SK_OVERRIDE;

    class BitmapProcShaderContext : public SkShader::Context {
    public:
        
        
        BitmapProcShaderContext(const SkBitmapProcShader&, const ContextRec&, SkBitmapProcState*);
        virtual ~BitmapProcShaderContext();

        virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) SK_OVERRIDE;
        virtual ShadeProc asAShadeProc(void** ctx) SK_OVERRIDE;
        virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count) SK_OVERRIDE;

        virtual uint32_t getFlags() const SK_OVERRIDE { return fFlags; }

    private:
        SkBitmapProcState*  fState;
        uint32_t            fFlags;

        typedef SkShader::Context INHERITED;
    };

protected:
    SkBitmapProcShader(SkReadBuffer& );
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual Context* onCreateContext(const ContextRec&, void* storage) const SK_OVERRIDE;

    SkBitmap    fRawBitmap;   
    uint8_t     fTileModeX, fTileModeY;

private:
    typedef SkShader INHERITED;
};






typedef SkSmallAllocator<3, 768> SkTBlitterAllocator;



SkShader* CreateBitmapShader(const SkBitmap& src, SkShader::TileMode, SkShader::TileMode,
                             const SkMatrix* localMatrix, SkTBlitterAllocator* alloc);

#endif
