








#ifndef SkColorFilter_DEFINED
#define SkColorFilter_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"
#include "SkXfermode.h"

class SK_API SkColorFilter : public SkFlattenable {
public:
    




    virtual bool asColorMode(SkColor* color, SkXfermode::Mode* mode);

    






    virtual void filterSpan(const SkPMColor src[], int count,
                            SkPMColor result[]) = 0;
    






    virtual void filterSpan16(const uint16_t shader[], int count,
                              uint16_t result[]);

    enum Flags {
        


        kAlphaUnchanged_Flag = 0x01,
        


        kHasFilter16_Flag    = 0x02
    };

    


    virtual uint32_t getFlags() { return 0; }

    





    SkColor filterColor(SkColor);


    








    static SkColorFilter* CreateModeFilter(SkColor c, SkXfermode::Mode mode);

    



    static SkColorFilter* CreateProcFilter(SkColor srcColor,
                                           SkXfermodeProc proc,
                                           SkXfermodeProc16 proc16 = NULL);

    




    static SkColorFilter* CreateLightingFilter(SkColor mul, SkColor add);

protected:
    SkColorFilter() {}
    SkColorFilter(SkFlattenableReadBuffer& rb) : INHERITED(rb) {}

private:
    typedef SkFlattenable INHERITED;
};

#include "SkShader.h"

class SkFilterShader : public SkShader {
public:
    SkFilterShader(SkShader* shader, SkColorFilter* filter);
    virtual ~SkFilterShader();

    
    virtual uint32_t getFlags();
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor result[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t result[], int count);
    virtual void beginSession();
    virtual void endSession();

protected:
    SkFilterShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer& ) SK_OVERRIDE;
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }
private:
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkFilterShader, (buffer)); }
    SkShader*       fShader;
    SkColorFilter*  fFilter;

    typedef SkShader INHERITED;
};

#endif
