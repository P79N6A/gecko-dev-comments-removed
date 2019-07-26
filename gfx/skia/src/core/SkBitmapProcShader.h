








#ifndef SkBitmapProcShader_DEFINED
#define SkBitmapProcShader_DEFINED

#include "SkShader.h"
#include "SkBitmapProcState.h"

class SkBitmapProcShader : public SkShader {
public:
    SkBitmapProcShader(const SkBitmap& src, TileMode tx, TileMode ty);

    
    virtual bool isOpaque() const SK_OVERRIDE;
    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&);
    virtual uint32_t getFlags() { return fFlags; }
    virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count);
    virtual void beginSession();
    virtual void endSession();
    virtual BitmapType asABitmap(SkBitmap*, SkMatrix*, TileMode*) const;

    static bool CanDo(const SkBitmap&, TileMode tx, TileMode ty);

    
    virtual bool toDumpString(SkString* str) const;
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBitmapProcShader)

protected:
    SkBitmapProcShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    SkBitmap          fRawBitmap;   
    SkBitmapProcState fState;
    uint32_t          fFlags;

private:
    typedef SkShader INHERITED;
};

#endif
