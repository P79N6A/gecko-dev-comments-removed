








#ifndef SkFlipPixelRef_DEFINED
#define SkFlipPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkPageFlipper.h"
#include "SkPixelRef.h"
#include "SkThread.h"

class SkRegion;

class SkFlipPixelRef : public SkPixelRef {
public:
            SkFlipPixelRef(SkBitmap::Config, int width, int height);
    virtual ~SkFlipPixelRef();
    
    bool isDirty() const { return fFlipper.isDirty(); }
    const SkRegion& dirtyRgn() const { return fFlipper.dirtyRgn(); }

    void inval() { fFlipper.inval(); }
    void inval(const SkIRect& rect) { fFlipper.inval(rect); }
    void inval(const SkRegion& rgn) { fFlipper.inval(rgn); }
    void inval(const SkRect& r, bool doAA) { fFlipper.inval(r, doAA); }

    const SkRegion& beginUpdate(SkBitmap* device);
    void endUpdate();
    
private:
    void getFrontBack(const void** front, void** back) const {
        if (front) {
            *front = fPage0;
        }
        if (back) {
            *back = fPage1;
        }
    }

    void    swapPages();

    
    
    static void CopyBitsFromAddr(const SkBitmap& dst, const SkRegion& clip,
                                 const void* srcAddr);

    

public:
    virtual Factory getFactory() const { return Create; }
    virtual void flatten(SkFlattenableWriteBuffer&) const;
    static SkPixelRef* Create(SkFlattenableReadBuffer& buffer);
    
protected:
    virtual void* onLockPixels(SkColorTable**);
    virtual void onUnlockPixels();

    SkFlipPixelRef(SkFlattenableReadBuffer&);

private:
    SkMutex         fMutex;
    SkPageFlipper   fFlipper;
    
    void*           fStorage;
    void*           fPage0; 
    void*           fPage1; 
    size_t          fSize;  
    SkBitmap::Config fConfig;

    typedef SkPixelRef INHERITED;
};

class SkAutoFlipUpdate : SkNoncopyable {
public:
    SkAutoFlipUpdate(SkFlipPixelRef* ref) : fRef(ref) {
        fDirty = &ref->beginUpdate(&fBitmap);
    }
    ~SkAutoFlipUpdate() {
        if (fRef) {
            fRef->endUpdate();
        }
    }
    
    const SkBitmap& bitmap() const { return fBitmap; }
    const SkRegion& dirty() const { return *fDirty; }
    
    
    void endUpdate() {
        if (fRef) {
            fRef->endUpdate();
            fRef = NULL;
        }
    }

private:
    SkFlipPixelRef* fRef;
    SkBitmap        fBitmap;
    const SkRegion* fDirty;
};

#endif
