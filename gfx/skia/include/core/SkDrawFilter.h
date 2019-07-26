








#ifndef SkDrawFilter_DEFINED
#define SkDrawFilter_DEFINED

#include "SkRefCnt.h"

class SkCanvas;
class SkPaint;







class SkDrawFilter : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkDrawFilter)

    enum Type {
        kPaint_Type,
        kPoint_Type,
        kLine_Type,
        kBitmap_Type,
        kRect_Type,
        kPath_Type,
        kText_Type
    };

    



    virtual void filter(SkPaint*, Type) = 0;

private:
    typedef SkRefCnt INHERITED;
};

#endif
