








#ifndef SkPageFlipper_DEFINED
#define SkPageFlipper_DEFINED

#include "SkRegion.h"










class SkPageFlipper {
public:
    SkPageFlipper();
    SkPageFlipper(int width, int height);
    
    int width() const { return fWidth; }
    int height() const { return fHeight; }

    void resize(int width, int height);

    bool isDirty() const { return !fDirty1->isEmpty(); }
    const SkRegion& dirtyRgn() const { return *fDirty1; }

    void inval();
    void inval(const SkIRect&);
    void inval(const SkRegion&);
    void inval(const SkRect&, bool antialias);

    








    const SkRegion& update(SkRegion* copyBits);

private:
    SkRegion*   fDirty0;
    SkRegion*   fDirty1;
    SkRegion    fDirty0Storage;
    SkRegion    fDirty1Storage;
    int         fWidth;
    int         fHeight;
};

#endif

