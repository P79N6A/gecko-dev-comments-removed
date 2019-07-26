








#ifndef SkColorFilter_DEFINED
#define SkColorFilter_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"
#include "SkXfermode.h"

class SkBitmap;

class SK_API SkColorFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkColorFilter)

    




    virtual bool asColorMode(SkColor* color, SkXfermode::Mode* mode);

    




    virtual bool asColorMatrix(SkScalar matrix[20]);

    















    virtual bool asComponentTable(SkBitmap* table);

    






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

    




    static SkColorFilter* CreateLightingFilter(SkColor mul, SkColor add);

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
protected:
    SkColorFilter() {}
    SkColorFilter(SkFlattenableReadBuffer& rb) : INHERITED(rb) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
