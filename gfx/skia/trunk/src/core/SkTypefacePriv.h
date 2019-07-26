






#ifndef SkTypefacePriv_DEFINED
#define SkTypefacePriv_DEFINED

#include "SkTypeface.h"







static inline SkTypeface* ref_or_default(SkTypeface* face) {
    return face ? SkRef(face) : SkTypeface::RefDefault();
}





class SkAutoResolveDefaultTypeface : public SkAutoTUnref<SkTypeface> {
public:
    SkAutoResolveDefaultTypeface() : INHERITED(SkTypeface::RefDefault()) {}

    SkAutoResolveDefaultTypeface(SkTypeface* face)
        : INHERITED(ref_or_default(face)) {}

private:
    typedef SkAutoTUnref<SkTypeface> INHERITED;
};

#endif
