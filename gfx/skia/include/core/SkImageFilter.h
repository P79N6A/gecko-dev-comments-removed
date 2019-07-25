






#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"

class SkImageFilter : public SkFlattenable {
public:

    












    bool filterImage(const SkBitmap& src, const SkMatrix&,
                     SkBitmap* result, SkPoint* offset);

protected:
    virtual bool onFilterImage(const SkBitmap& src, const SkMatrix&
                               SkBitmap* result, SkPoint* offset) = 0;

private:
    typedef SkFlattenable INHERITED;
};

#endif
