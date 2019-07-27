








#ifndef SkColorFilter_DEFINED
#define SkColorFilter_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"
#include "SkXfermode.h"

class SkBitmap;
class GrEffect;
class GrContext;









class SK_API SkColorFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkColorFilter)

    




    virtual bool asColorMode(SkColor* color, SkXfermode::Mode* mode) const;

    




    virtual bool asColorMatrix(SkScalar matrix[20]) const;

    















    virtual bool asComponentTable(SkBitmap* table) const;

    






    virtual void filterSpan(const SkPMColor src[], int count,
                            SkPMColor result[]) const = 0;
    






    virtual void filterSpan16(const uint16_t shader[], int count,
                              uint16_t result[]) const;

    enum Flags {
        


        kAlphaUnchanged_Flag = 0x01,
        


        kHasFilter16_Flag    = 0x02
    };

    


    virtual uint32_t getFlags() const { return 0; }

    





    SkColor filterColor(SkColor) const;

    








    static SkColorFilter* CreateModeFilter(SkColor c, SkXfermode::Mode mode);

    




    static SkColorFilter* CreateLightingFilter(SkColor mul, SkColor add);

    


    virtual GrEffect* asNewEffect(GrContext*) const;

    SK_TO_STRING_PUREVIRT()

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
    SK_DEFINE_FLATTENABLE_TYPE(SkColorFilter)

protected:
    SkColorFilter() {}
    SkColorFilter(SkReadBuffer& rb) : INHERITED(rb) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
