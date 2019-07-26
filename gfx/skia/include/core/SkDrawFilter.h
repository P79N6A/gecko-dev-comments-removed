








#ifndef SkDrawFilter_DEFINED
#define SkDrawFilter_DEFINED

#include "SkRefCnt.h"

class SkCanvas;
class SkPaint;







class SK_API SkDrawFilter : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkDrawFilter)

    enum Type {
        kPaint_Type,
        kPoint_Type,
        kLine_Type,
        kBitmap_Type,
        kRect_Type,
        kOval_Type,
        kPath_Type,
        kText_Type,
    };

    enum {
        kTypeCount = kText_Type + 1
    };

    




    virtual bool filter(SkPaint*, Type) = 0;

private:
    typedef SkRefCnt INHERITED;
};

#endif
