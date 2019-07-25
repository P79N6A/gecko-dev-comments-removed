








#ifndef SkSpriteBlitter_DEFINED
#define SkSpriteBlitter_DEFINED

#include "SkBlitter.h"
#include "SkBitmap.h"

class SkPaint;

class SkSpriteBlitter : public SkBlitter {
public:
            SkSpriteBlitter(const SkBitmap& source);
    virtual ~SkSpriteBlitter();

    virtual void setup(const SkBitmap& device, int left, int top,
                       const SkPaint& paint);

    
#ifdef SK_DEBUG
    virtual void    blitH(int x, int y, int width);
    virtual void    blitAntiH(int x, int y, const SkAlpha antialias[], const int16_t runs[]);
    virtual void    blitV(int x, int y, int height, SkAlpha alpha);
    virtual void    blitMask(const SkMask&, const SkIRect& clip);
#endif

    static SkSpriteBlitter* ChooseD16(const SkBitmap& source, const SkPaint&,
                                      void* storage, size_t storageSize);
    static SkSpriteBlitter* ChooseD32(const SkBitmap& source, const SkPaint&,
                                      void* storage, size_t storageSize);

protected:
    const SkBitmap* fDevice;
    const SkBitmap* fSource;
    int             fLeft, fTop;
    const SkPaint*  fPaint;
};

#endif

