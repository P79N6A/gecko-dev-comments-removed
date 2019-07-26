








#ifndef SkBitmapProcShader_DEFINED
#define SkBitmapProcShader_DEFINED

#include "SkShader.h"
#include "SkBitmapProcState.h"
#include "SkSmallAllocator.h"

class SkBitmapProcShader : public SkShader {
public:
    SkBitmapProcShader(const SkBitmap& src, TileMode tx, TileMode ty);

    
    virtual bool isOpaque() const SK_OVERRIDE;
    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&) SK_OVERRIDE;
    virtual void endContext() SK_OVERRIDE;
    virtual uint32_t getFlags() SK_OVERRIDE { return fFlags; }
    virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) SK_OVERRIDE;
    virtual ShadeProc asAShadeProc(void** ctx) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count) SK_OVERRIDE;
    virtual BitmapType asABitmap(SkBitmap*, SkMatrix*, TileMode*) const SK_OVERRIDE;

    static bool CanDo(const SkBitmap&, TileMode tx, TileMode ty);

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBitmapProcShader)

#if SK_SUPPORT_GPU
    GrEffectRef* asNewEffect(GrContext*, const SkPaint&) const SK_OVERRIDE;
#endif

protected:
    SkBitmapProcShader(SkReadBuffer& );
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    SkBitmap          fRawBitmap;   
    SkBitmapProcState fState;
    uint32_t          fFlags;

private:
    typedef SkShader INHERITED;
};




typedef SkSmallAllocator<2, sizeof(SkBitmapProcShader) + sizeof(void*) * 2> SkTBlitterAllocator;



SkShader* CreateBitmapShader(const SkBitmap& src, SkShader::TileMode, SkShader::TileMode,
                             SkTBlitterAllocator* alloc);

#endif
