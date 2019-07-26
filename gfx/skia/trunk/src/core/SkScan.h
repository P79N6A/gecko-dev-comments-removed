








#ifndef SkScan_DEFINED
#define SkScan_DEFINED

#include "SkRect.h"

class SkRasterClip;
class SkRegion;
class SkBlitter;
class SkPath;




typedef SkIRect SkXRect;

class SkScan {
public:
    static void FillPath(const SkPath&, const SkIRect&, SkBlitter*);

    
    

    static void FillIRect(const SkIRect&, const SkRasterClip&, SkBlitter*);
    static void FillXRect(const SkXRect&, const SkRasterClip&, SkBlitter*);
    static void FillRect(const SkRect&, const SkRasterClip&, SkBlitter*);
    static void AntiFillRect(const SkRect&, const SkRasterClip&, SkBlitter*);
    static void AntiFillXRect(const SkXRect&, const SkRasterClip&, SkBlitter*);
    static void FillPath(const SkPath&, const SkRasterClip&, SkBlitter*);
    static void AntiFillPath(const SkPath&, const SkRasterClip&, SkBlitter*);
    static void FrameRect(const SkRect&, const SkPoint& strokeSize,
                          const SkRasterClip&, SkBlitter*);
    static void AntiFrameRect(const SkRect&, const SkPoint& strokeSize,
                              const SkRasterClip&, SkBlitter*);
    static void FillTriangle(const SkPoint pts[], const SkRasterClip&, SkBlitter*);
    static void HairLine(const SkPoint&, const SkPoint&, const SkRasterClip&,
                         SkBlitter*);
    static void AntiHairLine(const SkPoint&, const SkPoint&, const SkRasterClip&,
                             SkBlitter*);
    static void HairRect(const SkRect&, const SkRasterClip&, SkBlitter*);
    static void AntiHairRect(const SkRect&, const SkRasterClip&, SkBlitter*);
    static void HairPath(const SkPath&, const SkRasterClip&, SkBlitter*);
    static void AntiHairPath(const SkPath&, const SkRasterClip&, SkBlitter*);

private:
    friend class SkAAClip;
    friend class SkRegion;

    static void FillIRect(const SkIRect&, const SkRegion* clip, SkBlitter*);
    static void FillXRect(const SkXRect&, const SkRegion* clip, SkBlitter*);
    static void FillRect(const SkRect&, const SkRegion* clip, SkBlitter*);
    static void AntiFillRect(const SkRect&, const SkRegion* clip, SkBlitter*);
    static void AntiFillXRect(const SkXRect&, const SkRegion*, SkBlitter*);
    static void FillPath(const SkPath&, const SkRegion& clip, SkBlitter*);
    static void AntiFillPath(const SkPath&, const SkRegion& clip, SkBlitter*,
                             bool forceRLE = false);
    static void FillTriangle(const SkPoint pts[], const SkRegion*, SkBlitter*);

    static void AntiFrameRect(const SkRect&, const SkPoint& strokeSize,
                              const SkRegion*, SkBlitter*);
    static void HairLineRgn(const SkPoint&, const SkPoint&, const SkRegion*,
                         SkBlitter*);
    static void AntiHairLineRgn(const SkPoint&, const SkPoint&, const SkRegion*,
                             SkBlitter*);
};





static inline void XRect_set(SkXRect* xr, const SkIRect& src) {
    xr->fLeft = SkIntToFixed(src.fLeft);
    xr->fTop = SkIntToFixed(src.fTop);
    xr->fRight = SkIntToFixed(src.fRight);
    xr->fBottom = SkIntToFixed(src.fBottom);
}





static inline void XRect_set(SkXRect* xr, const SkRect& src) {
    xr->fLeft = SkScalarToFixed(src.fLeft);
    xr->fTop = SkScalarToFixed(src.fTop);
    xr->fRight = SkScalarToFixed(src.fRight);
    xr->fBottom = SkScalarToFixed(src.fBottom);
}



static inline void XRect_round(const SkXRect& xr, SkIRect* dst) {
    dst->fLeft = SkFixedRoundToInt(xr.fLeft);
    dst->fTop = SkFixedRoundToInt(xr.fTop);
    dst->fRight = SkFixedRoundToInt(xr.fRight);
    dst->fBottom = SkFixedRoundToInt(xr.fBottom);
}




static inline void XRect_roundOut(const SkXRect& xr, SkIRect* dst) {
    dst->fLeft = SkFixedFloorToInt(xr.fLeft);
    dst->fTop = SkFixedFloorToInt(xr.fTop);
    dst->fRight = SkFixedCeilToInt(xr.fRight);
    dst->fBottom = SkFixedCeilToInt(xr.fBottom);
}

#endif
