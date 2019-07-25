








#include "SkPath.h"
#include "SkReader32.h"
#include "SkWriter32.h"
#include "SkMath.h"














class SkAutoPathBoundsUpdate {
public:
    SkAutoPathBoundsUpdate(SkPath* path, const SkRect& r) : fRect(r) {
        this->init(path);
    }

    SkAutoPathBoundsUpdate(SkPath* path, SkScalar left, SkScalar top,
                           SkScalar right, SkScalar bottom) {
        fRect.set(left, top, right, bottom);
        this->init(path);
    }

    ~SkAutoPathBoundsUpdate() {
        fPath->setIsConvex(fEmpty);
        if (fEmpty) {
            fPath->fBounds = fRect;
            fPath->fBoundsIsDirty = false;
        } else if (!fDirty) {
            fPath->fBounds.join(fRect);
            fPath->fBoundsIsDirty = false;
        }
    }

private:
    SkPath* fPath;
    SkRect  fRect;
    bool    fDirty;
    bool    fEmpty;

    
    void init(SkPath* path) {
        fPath = path;
        fDirty = SkToBool(path->fBoundsIsDirty);
        fEmpty = path->isEmpty();
        
        fRect.sort();
    }
};

static void compute_pt_bounds(SkRect* bounds, const SkTDArray<SkPoint>& pts) {
    if (pts.count() <= 1) {  
        bounds->set(0, 0, 0, 0);
    } else {
        bounds->set(pts.begin(), pts.count());

    }
}
















SkPath::SkPath() 
    : fFillType(kWinding_FillType)
    , fBoundsIsDirty(true) {
    fConvexity = kUnknown_Convexity;
    fSegmentMask = 0;
#ifdef ANDROID
    fGenerationID = 0;
#endif
}

SkPath::SkPath(const SkPath& src) {
    SkDEBUGCODE(src.validate();)
    *this = src;
#ifdef ANDROID
    
    fGenerationID--;
#endif
}

SkPath::~SkPath() {
    SkDEBUGCODE(this->validate();)
}

SkPath& SkPath::operator=(const SkPath& src) {
    SkDEBUGCODE(src.validate();)

    if (this != &src) {
        fBounds         = src.fBounds;
        fPts            = src.fPts;
        fVerbs          = src.fVerbs;
        fFillType       = src.fFillType;
        fBoundsIsDirty  = src.fBoundsIsDirty;
        fConvexity      = src.fConvexity;
        fSegmentMask    = src.fSegmentMask;
        GEN_ID_INC;
    }
    SkDEBUGCODE(this->validate();)
    return *this;
}

bool operator==(const SkPath& a, const SkPath& b) {
    
    

    
    
    

    return &a == &b ||
        (a.fFillType == b.fFillType && a.fSegmentMask == b.fSegmentMask &&
         a.fVerbs == b.fVerbs && a.fPts == b.fPts);
}

void SkPath::swap(SkPath& other) {
    SkASSERT(&other != NULL);

    if (this != &other) {
        SkTSwap<SkRect>(fBounds, other.fBounds);
        fPts.swap(other.fPts);
        fVerbs.swap(other.fVerbs);
        SkTSwap<uint8_t>(fFillType, other.fFillType);
        SkTSwap<uint8_t>(fBoundsIsDirty, other.fBoundsIsDirty);
        SkTSwap<uint8_t>(fConvexity, other.fConvexity);
        SkTSwap<uint8_t>(fSegmentMask, other.fSegmentMask);
        GEN_ID_INC;
    }
}

#ifdef ANDROID
uint32_t SkPath::getGenerationID() const {
    return fGenerationID;
}
#endif

void SkPath::reset() {
    SkDEBUGCODE(this->validate();)

    fPts.reset();
    fVerbs.reset();
    GEN_ID_INC;
    fBoundsIsDirty = true;
    fConvexity = kUnknown_Convexity;
    fSegmentMask = 0;
}

void SkPath::rewind() {
    SkDEBUGCODE(this->validate();)

    fPts.rewind();
    fVerbs.rewind();
    GEN_ID_INC;
    fBoundsIsDirty = true;
    fConvexity = kUnknown_Convexity;
    fSegmentMask = 0;
}

bool SkPath::isEmpty() const {
    SkDEBUGCODE(this->validate();)

    int count = fVerbs.count();
    return count == 0 || (count == 1 && fVerbs[0] == kMove_Verb);
}




































bool SkPath::isRect(SkRect* rect) const {
    SkDEBUGCODE(this->validate();)

    int corners = 0;
    SkPoint first, last;
    first.set(0, 0);
    last.set(0, 0);
    int firstDirection = 0;
    int lastDirection = 0;
    int nextDirection = 0;
    bool closedOrMoved = false;
    bool autoClose = false;
    const uint8_t* verbs = fVerbs.begin();
    const uint8_t* verbStop = fVerbs.end();
    const SkPoint* pts = fPts.begin();
    while (verbs != verbStop) {
        switch (*verbs++) {
            case kClose_Verb:
                pts = fPts.begin();
                autoClose = true;
            case kLine_Verb: {
                SkScalar left = last.fX;
                SkScalar top = last.fY;
                SkScalar right = pts->fX;
                SkScalar bottom = pts->fY;
                ++pts;
                if (left != right && top != bottom) {
                    return false; 
                }
                if (left == right && top == bottom) {
                    break; 
                }
                nextDirection = (left != right) << 0 |
                    (left < right || top < bottom) << 1;
                if (0 == corners) {
                    firstDirection = nextDirection;
                    first = last;
                    last = pts[-1];
                    corners = 1;
                    closedOrMoved = false;
                    break;
                }
                if (closedOrMoved) {
                    return false; 
                }
                closedOrMoved = autoClose;
                if (lastDirection != nextDirection) {
                    if (++corners > 4) {
                        return false; 
                    }
                }
                last = pts[-1];
                if (lastDirection == nextDirection) {
                    break; 
                }
                
                
                
                int turn = firstDirection ^ (corners - 1);
                int directionCycle = 3 == corners ? 0 : nextDirection ^ turn;
                if ((directionCycle ^ turn) != nextDirection) {
                    return false; 
                }
                break;
            }
            case kQuad_Verb:
            case kCubic_Verb:
                return false; 
            case kMove_Verb:
                last = *pts++;
                closedOrMoved = true;
                break;
        }
        lastDirection = nextDirection;
    }
    
    bool result = 4 == corners && first == last;
    if (result && rect) {
        *rect = getBounds();
    }
    return result;
}

int SkPath::getPoints(SkPoint copy[], int max) const {
    SkDEBUGCODE(this->validate();)

    SkASSERT(max >= 0);
    int count = fPts.count();
    if (copy && max > 0 && count > 0) {
        memcpy(copy, fPts.begin(), sizeof(SkPoint) * SkMin32(max, count));
    }
    return count;
}

SkPoint SkPath::getPoint(int index) const {
    if ((unsigned)index < (unsigned)fPts.count()) {
        return fPts[index];
    }
    return SkPoint::Make(0, 0);
}

bool SkPath::getLastPt(SkPoint* lastPt) const {
    SkDEBUGCODE(this->validate();)

    int count = fPts.count();
    if (count > 0) {
        if (lastPt) {
            *lastPt = fPts[count - 1];
        }
        return true;
    }
    if (lastPt) {
        lastPt->set(0, 0);
    }
    return false;
}

void SkPath::setLastPt(SkScalar x, SkScalar y) {
    SkDEBUGCODE(this->validate();)

    int count = fPts.count();
    if (count == 0) {
        this->moveTo(x, y);
    } else {
        fPts[count - 1].set(x, y);
        GEN_ID_INC;
    }
}

void SkPath::computeBounds() const {
    SkDEBUGCODE(this->validate();)
    SkASSERT(fBoundsIsDirty);

    fBoundsIsDirty = false;
    compute_pt_bounds(&fBounds, fPts);
}

void SkPath::setConvexity(Convexity c) {
    if (fConvexity != c) {
        fConvexity = c;
        GEN_ID_INC;
    }
}




#define DIRTY_AFTER_EDIT                \
    do {                                \
        fBoundsIsDirty = true;          \
        fConvexity = kUnknown_Convexity;\
    } while (0)

void SkPath::incReserve(U16CPU inc) {
    SkDEBUGCODE(this->validate();)

    fVerbs.setReserve(fVerbs.count() + inc);
    fPts.setReserve(fPts.count() + inc);

    SkDEBUGCODE(this->validate();)
}

void SkPath::moveTo(SkScalar x, SkScalar y) {
    SkDEBUGCODE(this->validate();)

    int      vc = fVerbs.count();
    SkPoint* pt;

    if (vc > 0 && fVerbs[vc - 1] == kMove_Verb) {
        pt = &fPts[fPts.count() - 1];
    } else {
        pt = fPts.append();
        *fVerbs.append() = kMove_Verb;
    }
    pt->set(x, y);

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;
}

void SkPath::rMoveTo(SkScalar x, SkScalar y) {
    SkPoint pt;
    this->getLastPt(&pt);
    this->moveTo(pt.fX + x, pt.fY + y);
}

void SkPath::lineTo(SkScalar x, SkScalar y) {
    SkDEBUGCODE(this->validate();)

    if (fVerbs.count() == 0) {
        fPts.append()->set(0, 0);
        *fVerbs.append() = kMove_Verb;
    }
    fPts.append()->set(x, y);
    *fVerbs.append() = kLine_Verb;
    fSegmentMask |= kLine_SegmentMask;

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;
}

void SkPath::rLineTo(SkScalar x, SkScalar y) {
    SkPoint pt;
    this->getLastPt(&pt);
    this->lineTo(pt.fX + x, pt.fY + y);
}

void SkPath::quadTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2) {
    SkDEBUGCODE(this->validate();)

    if (fVerbs.count() == 0) {
        fPts.append()->set(0, 0);
        *fVerbs.append() = kMove_Verb;
    }

    SkPoint* pts = fPts.append(2);
    pts[0].set(x1, y1);
    pts[1].set(x2, y2);
    *fVerbs.append() = kQuad_Verb;
    fSegmentMask |= kQuad_SegmentMask;

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;
}

void SkPath::rQuadTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2) {
    SkPoint pt;
    this->getLastPt(&pt);
    this->quadTo(pt.fX + x1, pt.fY + y1, pt.fX + x2, pt.fY + y2);
}

void SkPath::cubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                     SkScalar x3, SkScalar y3) {
    SkDEBUGCODE(this->validate();)

    if (fVerbs.count() == 0) {
        fPts.append()->set(0, 0);
        *fVerbs.append() = kMove_Verb;
    }
    SkPoint* pts = fPts.append(3);
    pts[0].set(x1, y1);
    pts[1].set(x2, y2);
    pts[2].set(x3, y3);
    *fVerbs.append() = kCubic_Verb;
    fSegmentMask |= kCubic_SegmentMask;

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;
}

void SkPath::rCubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                      SkScalar x3, SkScalar y3) {
    SkPoint pt;
    this->getLastPt(&pt);
    this->cubicTo(pt.fX + x1, pt.fY + y1, pt.fX + x2, pt.fY + y2,
                  pt.fX + x3, pt.fY + y3);
}

void SkPath::close() {
    SkDEBUGCODE(this->validate();)

    int count = fVerbs.count();
    if (count > 0) {
        switch (fVerbs[count - 1]) {
            case kLine_Verb:
            case kQuad_Verb:
            case kCubic_Verb:
                *fVerbs.append() = kClose_Verb;
                GEN_ID_INC;
                break;
            default:
                
                break;
        }
    }
}



void SkPath::addRect(const SkRect& rect, Direction dir) {
    this->addRect(rect.fLeft, rect.fTop, rect.fRight, rect.fBottom, dir);
}

void SkPath::addRect(SkScalar left, SkScalar top, SkScalar right,
                     SkScalar bottom, Direction dir) {
    SkAutoPathBoundsUpdate apbu(this, left, top, right, bottom);

    this->incReserve(5);

    this->moveTo(left, top);
    if (dir == kCCW_Direction) {
        this->lineTo(left, bottom);
        this->lineTo(right, bottom);
        this->lineTo(right, top);
    } else {
        this->lineTo(right, top);
        this->lineTo(right, bottom);
        this->lineTo(left, bottom);
    }
    this->close();
}

#define CUBIC_ARC_FACTOR    ((SK_ScalarSqrt2 - SK_Scalar1) * 4 / 3)

void SkPath::addRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry,
                          Direction dir) {
    SkScalar    w = rect.width();
    SkScalar    halfW = SkScalarHalf(w);
    SkScalar    h = rect.height();
    SkScalar    halfH = SkScalarHalf(h);

    if (halfW <= 0 || halfH <= 0) {
        return;
    }

    bool skip_hori = rx >= halfW;
    bool skip_vert = ry >= halfH;

    if (skip_hori && skip_vert) {
        this->addOval(rect, dir);
        return;
    }

    SkAutoPathBoundsUpdate apbu(this, rect);

    if (skip_hori) {
        rx = halfW;
    } else if (skip_vert) {
        ry = halfH;
    }

    SkScalar    sx = SkScalarMul(rx, CUBIC_ARC_FACTOR);
    SkScalar    sy = SkScalarMul(ry, CUBIC_ARC_FACTOR);

    this->incReserve(17);
    this->moveTo(rect.fRight - rx, rect.fTop);
    if (dir == kCCW_Direction) {
        if (!skip_hori) {
            this->lineTo(rect.fLeft + rx, rect.fTop);       
        }
        this->cubicTo(rect.fLeft + rx - sx, rect.fTop,
                      rect.fLeft, rect.fTop + ry - sy,
                      rect.fLeft, rect.fTop + ry);          
        if (!skip_vert) {
            this->lineTo(rect.fLeft, rect.fBottom - ry);        
        }
        this->cubicTo(rect.fLeft, rect.fBottom - ry + sy,
                      rect.fLeft + rx - sx, rect.fBottom,
                      rect.fLeft + rx, rect.fBottom);       
        if (!skip_hori) {
            this->lineTo(rect.fRight - rx, rect.fBottom);   
        }
        this->cubicTo(rect.fRight - rx + sx, rect.fBottom,
                      rect.fRight, rect.fBottom - ry + sy,
                      rect.fRight, rect.fBottom - ry);      
        if (!skip_vert) {
            this->lineTo(rect.fRight, rect.fTop + ry);
        }
        this->cubicTo(rect.fRight, rect.fTop + ry - sy,
                      rect.fRight - rx + sx, rect.fTop,
                      rect.fRight - rx, rect.fTop);         
    } else {
        this->cubicTo(rect.fRight - rx + sx, rect.fTop,
                      rect.fRight, rect.fTop + ry - sy,
                      rect.fRight, rect.fTop + ry);         
        if (!skip_vert) {
            this->lineTo(rect.fRight, rect.fBottom - ry);
        }
        this->cubicTo(rect.fRight, rect.fBottom - ry + sy,
                      rect.fRight - rx + sx, rect.fBottom,
                      rect.fRight - rx, rect.fBottom);      
        if (!skip_hori) {
            this->lineTo(rect.fLeft + rx, rect.fBottom);    
        }
        this->cubicTo(rect.fLeft + rx - sx, rect.fBottom,
                      rect.fLeft, rect.fBottom - ry + sy,
                      rect.fLeft, rect.fBottom - ry);       
        if (!skip_vert) {
            this->lineTo(rect.fLeft, rect.fTop + ry);       
        }
        this->cubicTo(rect.fLeft, rect.fTop + ry - sy,
                      rect.fLeft + rx - sx, rect.fTop,
                      rect.fLeft + rx, rect.fTop);          
        if (!skip_hori) {
            this->lineTo(rect.fRight - rx, rect.fTop);      
        }
    }
    this->close();
}

static void add_corner_arc(SkPath* path, const SkRect& rect,
                           SkScalar rx, SkScalar ry, int startAngle,
                           SkPath::Direction dir, bool forceMoveTo) {
    rx = SkMinScalar(SkScalarHalf(rect.width()), rx);
    ry = SkMinScalar(SkScalarHalf(rect.height()), ry);

    SkRect   r;
    r.set(-rx, -ry, rx, ry);

    switch (startAngle) {
        case   0:
            r.offset(rect.fRight - r.fRight, rect.fBottom - r.fBottom);
            break;
        case  90:
            r.offset(rect.fLeft - r.fLeft,   rect.fBottom - r.fBottom);
            break;
        case 180: r.offset(rect.fLeft - r.fLeft,   rect.fTop - r.fTop); break;
        case 270: r.offset(rect.fRight - r.fRight, rect.fTop - r.fTop); break;
        default: SkASSERT(!"unexpected startAngle in add_corner_arc");
    }

    SkScalar start = SkIntToScalar(startAngle);
    SkScalar sweep = SkIntToScalar(90);
    if (SkPath::kCCW_Direction == dir) {
        start += sweep;
        sweep = -sweep;
    }

    path->arcTo(r, start, sweep, forceMoveTo);
}

void SkPath::addRoundRect(const SkRect& rect, const SkScalar rad[],
                          Direction dir) {
    
    if (rect.isEmpty()) {
        return;
    }

    SkAutoPathBoundsUpdate apbu(this, rect);

    if (kCW_Direction == dir) {
        add_corner_arc(this, rect, rad[0], rad[1], 180, dir, true);
        add_corner_arc(this, rect, rad[2], rad[3], 270, dir, false);
        add_corner_arc(this, rect, rad[4], rad[5],   0, dir, false);
        add_corner_arc(this, rect, rad[6], rad[7],  90, dir, false);
    } else {
        add_corner_arc(this, rect, rad[0], rad[1], 180, dir, true);
        add_corner_arc(this, rect, rad[6], rad[7],  90, dir, false);
        add_corner_arc(this, rect, rad[4], rad[5],   0, dir, false);
        add_corner_arc(this, rect, rad[2], rad[3], 270, dir, false);
    }
    this->close();
}

void SkPath::addOval(const SkRect& oval, Direction dir) {
    SkAutoPathBoundsUpdate apbu(this, oval);

    SkScalar    cx = oval.centerX();
    SkScalar    cy = oval.centerY();
    SkScalar    rx = SkScalarHalf(oval.width());
    SkScalar    ry = SkScalarHalf(oval.height());
#if 0   
    SkScalar    sx = SkScalarMul(rx, CUBIC_ARC_FACTOR);
    SkScalar    sy = SkScalarMul(ry, CUBIC_ARC_FACTOR);

    this->incReserve(13);
    this->moveTo(cx + rx, cy);
    if (dir == kCCW_Direction) {
        this->cubicTo(cx + rx, cy - sy, cx + sx, cy - ry, cx, cy - ry);
        this->cubicTo(cx - sx, cy - ry, cx - rx, cy - sy, cx - rx, cy);
        this->cubicTo(cx - rx, cy + sy, cx - sx, cy + ry, cx, cy + ry);
        this->cubicTo(cx + sx, cy + ry, cx + rx, cy + sy, cx + rx, cy);
    } else {
        this->cubicTo(cx + rx, cy + sy, cx + sx, cy + ry, cx, cy + ry);
        this->cubicTo(cx - sx, cy + ry, cx - rx, cy + sy, cx - rx, cy);
        this->cubicTo(cx - rx, cy - sy, cx - sx, cy - ry, cx, cy - ry);
        this->cubicTo(cx + sx, cy - ry, cx + rx, cy - sy, cx + rx, cy);
    }
#else
    SkScalar    sx = SkScalarMul(rx, SK_ScalarTanPIOver8);
    SkScalar    sy = SkScalarMul(ry, SK_ScalarTanPIOver8);
    SkScalar    mx = SkScalarMul(rx, SK_ScalarRoot2Over2);
    SkScalar    my = SkScalarMul(ry, SK_ScalarRoot2Over2);

    





    const SkScalar L = oval.fLeft;      
    const SkScalar T = oval.fTop;       
    const SkScalar R = oval.fRight;     
    const SkScalar B = oval.fBottom;    

    this->incReserve(17);   
    this->moveTo(R, cy);
    if (dir == kCCW_Direction) {
        this->quadTo(      R, cy - sy, cx + mx, cy - my);
        this->quadTo(cx + sx,       T, cx     ,       T);
        this->quadTo(cx - sx,       T, cx - mx, cy - my);
        this->quadTo(      L, cy - sy,       L, cy     );
        this->quadTo(      L, cy + sy, cx - mx, cy + my);
        this->quadTo(cx - sx,       B, cx     ,       B);
        this->quadTo(cx + sx,       B, cx + mx, cy + my);
        this->quadTo(      R, cy + sy,       R, cy     );
    } else {
        this->quadTo(      R, cy + sy, cx + mx, cy + my);
        this->quadTo(cx + sx,       B, cx     ,       B);
        this->quadTo(cx - sx,       B, cx - mx, cy + my);
        this->quadTo(      L, cy + sy,       L, cy     );
        this->quadTo(      L, cy - sy, cx - mx, cy - my);
        this->quadTo(cx - sx,       T, cx     ,       T);
        this->quadTo(cx + sx,       T, cx + mx, cy - my);
        this->quadTo(      R, cy - sy,       R, cy     );
    }
#endif
    this->close();
}

void SkPath::addCircle(SkScalar x, SkScalar y, SkScalar r, Direction dir) {
    if (r > 0) {
        SkRect  rect;
        rect.set(x - r, y - r, x + r, y + r);
        this->addOval(rect, dir);
    }
}

#include "SkGeometry.h"

static int build_arc_points(const SkRect& oval, SkScalar startAngle,
                            SkScalar sweepAngle,
                            SkPoint pts[kSkBuildQuadArcStorage]) {
    SkVector start, stop;

    start.fY = SkScalarSinCos(SkDegreesToRadians(startAngle), &start.fX);
    stop.fY = SkScalarSinCos(SkDegreesToRadians(startAngle + sweepAngle),
                             &stop.fX);

    







    if (start == stop) {
        SkScalar sw = SkScalarAbs(sweepAngle);
        if (sw < SkIntToScalar(360) && sw > SkIntToScalar(359)) {
            SkScalar stopRad = SkDegreesToRadians(startAngle + sweepAngle);
            
            SkScalar deltaRad = SkScalarCopySign(SK_Scalar1/512, sweepAngle);
            
            do {
                stopRad -= deltaRad;
                stop.fY = SkScalarSinCos(stopRad, &stop.fX);
            } while (start == stop);
        }
    }

    SkMatrix    matrix;

    matrix.setScale(SkScalarHalf(oval.width()), SkScalarHalf(oval.height()));
    matrix.postTranslate(oval.centerX(), oval.centerY());

    return SkBuildQuadArc(start, stop,
          sweepAngle > 0 ? kCW_SkRotationDirection : kCCW_SkRotationDirection,
                          &matrix, pts);
}

void SkPath::arcTo(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle,
                   bool forceMoveTo) {
    if (oval.width() < 0 || oval.height() < 0) {
        return;
    }

    SkPoint pts[kSkBuildQuadArcStorage];
    int count = build_arc_points(oval, startAngle, sweepAngle, pts);
    SkASSERT((count & 1) == 1);

    if (fVerbs.count() == 0) {
        forceMoveTo = true;
    }
    this->incReserve(count);
    forceMoveTo ? this->moveTo(pts[0]) : this->lineTo(pts[0]);
    for (int i = 1; i < count; i += 2) {
        this->quadTo(pts[i], pts[i+1]);
    }
}

void SkPath::addArc(const SkRect& oval, SkScalar startAngle,
                    SkScalar sweepAngle) {
    if (oval.isEmpty() || 0 == sweepAngle) {
        return;
    }

    const SkScalar kFullCircleAngle = SkIntToScalar(360);

    if (sweepAngle >= kFullCircleAngle || sweepAngle <= -kFullCircleAngle) {
        this->addOval(oval, sweepAngle > 0 ? kCW_Direction : kCCW_Direction);
        return;
    }

    SkPoint pts[kSkBuildQuadArcStorage];
    int count = build_arc_points(oval, startAngle, sweepAngle, pts);

    this->incReserve(count);
    this->moveTo(pts[0]);
    for (int i = 1; i < count; i += 2) {
        this->quadTo(pts[i], pts[i+1]);
    }
}





void SkPath::arcTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                   SkScalar radius) {
    SkVector    before, after;

    
    {
        SkPoint start;
        this->getLastPt(&start);
        
        
        if ((x1 == start.fX && y1 == start.fY) ||
            (x1 == x2 && y1 == y2) ||
            radius == 0) {
            this->lineTo(x1, y1);
            return;
        }
        before.setNormalize(x1 - start.fX, y1 - start.fY);
        after.setNormalize(x2 - x1, y2 - y1);
    }

    SkScalar cosh = SkPoint::DotProduct(before, after);
    SkScalar sinh = SkPoint::CrossProduct(before, after);

    if (SkScalarNearlyZero(sinh)) {   
        this->lineTo(x1, y1);
        return;
    }

    SkScalar dist = SkScalarMulDiv(radius, SK_Scalar1 - cosh, sinh);
    if (dist < 0) {
        dist = -dist;
    }

    SkScalar xx = x1 - SkScalarMul(dist, before.fX);
    SkScalar yy = y1 - SkScalarMul(dist, before.fY);
    SkRotationDirection arcDir;

    
    if (sinh > 0) {
        before.rotateCCW();
        after.rotateCCW();
        arcDir = kCW_SkRotationDirection;
    } else {
        before.rotateCW();
        after.rotateCW();
        arcDir = kCCW_SkRotationDirection;
    }

    SkMatrix    matrix;
    SkPoint     pts[kSkBuildQuadArcStorage];

    matrix.setScale(radius, radius);
    matrix.postTranslate(xx - SkScalarMul(radius, before.fX),
                         yy - SkScalarMul(radius, before.fY));

    int count = SkBuildQuadArc(before, after, arcDir, &matrix, pts);

    this->incReserve(count);
    
    this->lineTo(xx, yy);
    for (int i = 1; i < count; i += 2) {
        this->quadTo(pts[i], pts[i+1]);
    }
}



void SkPath::addPath(const SkPath& path, SkScalar dx, SkScalar dy) {
    SkMatrix matrix;

    matrix.setTranslate(dx, dy);
    this->addPath(path, matrix);
}

void SkPath::addPath(const SkPath& path, const SkMatrix& matrix) {
    this->incReserve(path.fPts.count());

    Iter    iter(path, false);
    SkPoint pts[4];
    Verb    verb;

    SkMatrix::MapPtsProc proc = matrix.getMapPtsProc();

    while ((verb = iter.next(pts)) != kDone_Verb) {
        switch (verb) {
            case kMove_Verb:
                proc(matrix, &pts[0], &pts[0], 1);
                this->moveTo(pts[0]);
                break;
            case kLine_Verb:
                proc(matrix, &pts[1], &pts[1], 1);
                this->lineTo(pts[1]);
                break;
            case kQuad_Verb:
                proc(matrix, &pts[1], &pts[1], 2);
                this->quadTo(pts[1], pts[2]);
                break;
            case kCubic_Verb:
                proc(matrix, &pts[1], &pts[1], 3);
                this->cubicTo(pts[1], pts[2], pts[3]);
                break;
            case kClose_Verb:
                this->close();
                break;
            default:
                SkASSERT(!"unknown verb");
        }
    }
}



static const uint8_t gPtsInVerb[] = {
    1,  
    1,  
    2,  
    3,  
    0,  
    0   
};


void SkPath::pathTo(const SkPath& path) {
    int i, vcount = path.fVerbs.count();
    if (vcount == 0) {
        return;
    }

    this->incReserve(vcount);

    const uint8_t*  verbs = path.fVerbs.begin();
    const SkPoint*  pts = path.fPts.begin() + 1;    

    SkASSERT(verbs[0] == kMove_Verb);
    for (i = 1; i < vcount; i++) {
        switch (verbs[i]) {
            case kLine_Verb:
                this->lineTo(pts[0].fX, pts[0].fY);
                break;
            case kQuad_Verb:
                this->quadTo(pts[0].fX, pts[0].fY, pts[1].fX, pts[1].fY);
                break;
            case kCubic_Verb:
                this->cubicTo(pts[0].fX, pts[0].fY, pts[1].fX, pts[1].fY,
                              pts[2].fX, pts[2].fY);
                break;
            case kClose_Verb:
                return;
        }
        pts += gPtsInVerb[verbs[i]];
    }
}


void SkPath::reversePathTo(const SkPath& path) {
    int i, vcount = path.fVerbs.count();
    if (vcount == 0) {
        return;
    }

    this->incReserve(vcount);

    const uint8_t*  verbs = path.fVerbs.begin();
    const SkPoint*  pts = path.fPts.begin();

    SkASSERT(verbs[0] == kMove_Verb);
    for (i = 1; i < vcount; i++) {
        int n = gPtsInVerb[verbs[i]];
        if (n == 0) {
            break;
        }
        pts += n;
    }

    while (--i > 0) {
        switch (verbs[i]) {
            case kLine_Verb:
                this->lineTo(pts[-1].fX, pts[-1].fY);
                break;
            case kQuad_Verb:
                this->quadTo(pts[-1].fX, pts[-1].fY, pts[-2].fX, pts[-2].fY);
                break;
            case kCubic_Verb:
                this->cubicTo(pts[-1].fX, pts[-1].fY, pts[-2].fX, pts[-2].fY,
                              pts[-3].fX, pts[-3].fY);
                break;
            default:
                SkASSERT(!"bad verb");
                break;
        }
        pts -= gPtsInVerb[verbs[i]];
    }
}



void SkPath::offset(SkScalar dx, SkScalar dy, SkPath* dst) const {
    SkMatrix    matrix;

    matrix.setTranslate(dx, dy);
    this->transform(matrix, dst);
}

#include "SkGeometry.h"

static void subdivide_quad_to(SkPath* path, const SkPoint pts[3],
                              int level = 2) {
    if (--level >= 0) {
        SkPoint tmp[5];

        SkChopQuadAtHalf(pts, tmp);
        subdivide_quad_to(path, &tmp[0], level);
        subdivide_quad_to(path, &tmp[2], level);
    } else {
        path->quadTo(pts[1], pts[2]);
    }
}

static void subdivide_cubic_to(SkPath* path, const SkPoint pts[4],
                               int level = 2) {
    if (--level >= 0) {
        SkPoint tmp[7];

        SkChopCubicAtHalf(pts, tmp);
        subdivide_cubic_to(path, &tmp[0], level);
        subdivide_cubic_to(path, &tmp[3], level);
    } else {
        path->cubicTo(pts[1], pts[2], pts[3]);
    }
}

void SkPath::transform(const SkMatrix& matrix, SkPath* dst) const {
    SkDEBUGCODE(this->validate();)
    if (dst == NULL) {
        dst = (SkPath*)this;
    }

    if (matrix.hasPerspective()) {
        SkPath  tmp;
        tmp.fFillType = fFillType;

        SkPath::Iter    iter(*this, false);
        SkPoint         pts[4];
        SkPath::Verb    verb;

        while ((verb = iter.next(pts)) != kDone_Verb) {
            switch (verb) {
                case kMove_Verb:
                    tmp.moveTo(pts[0]);
                    break;
                case kLine_Verb:
                    tmp.lineTo(pts[1]);
                    break;
                case kQuad_Verb:
                    subdivide_quad_to(&tmp, pts);
                    break;
                case kCubic_Verb:
                    subdivide_cubic_to(&tmp, pts);
                    break;
                case kClose_Verb:
                    tmp.close();
                    break;
                default:
                    SkASSERT(!"unknown verb");
                    break;
            }
        }

        dst->swap(tmp);
        matrix.mapPoints(dst->fPts.begin(), dst->fPts.count());
    } else {
        
        
        if (!fBoundsIsDirty && matrix.rectStaysRect() && fPts.count() > 1) {
            
            matrix.mapRect(&dst->fBounds, fBounds);
            dst->fBoundsIsDirty = false;
        } else {
            GEN_ID_PTR_INC(dst);
            dst->fBoundsIsDirty = true;
        }

        if (this != dst) {
            dst->fVerbs = fVerbs;
            dst->fPts.setCount(fPts.count());
            dst->fFillType = fFillType;
            dst->fSegmentMask = fSegmentMask;
            dst->fConvexity = fConvexity;
        }
        matrix.mapPoints(dst->fPts.begin(), fPts.begin(), fPts.count());
        SkDEBUGCODE(dst->validate();)
    }
}




enum NeedMoveToState {
    kAfterClose_NeedMoveToState,
    kAfterCons_NeedMoveToState,
    kAfterPrefix_NeedMoveToState
};

SkPath::Iter::Iter() {
#ifdef SK_DEBUG
    fPts = NULL;
    fMoveTo.fX = fMoveTo.fY = fLastPt.fX = fLastPt.fY = 0;
    fForceClose = fNeedMoveTo = fCloseLine = false;
#endif
    
    fVerbs = NULL;
    fVerbStop = NULL;
    fNeedClose = false;
}

SkPath::Iter::Iter(const SkPath& path, bool forceClose) {
    this->setPath(path, forceClose);
}

void SkPath::Iter::setPath(const SkPath& path, bool forceClose) {
    fPts = path.fPts.begin();
    fVerbs = path.fVerbs.begin();
    fVerbStop = path.fVerbs.end();
    fForceClose = SkToU8(forceClose);
    fNeedClose = false;
    fNeedMoveTo = kAfterPrefix_NeedMoveToState;
}

bool SkPath::Iter::isClosedContour() const {
    if (fVerbs == NULL || fVerbs == fVerbStop) {
        return false;
    }
    if (fForceClose) {
        return true;
    }

    const uint8_t* verbs = fVerbs;
    const uint8_t* stop = fVerbStop;

    if (kMove_Verb == *verbs) {
        verbs += 1; 
    }

    while (verbs < stop) {
        unsigned v = *verbs++;
        if (kMove_Verb == v) {
            break;
        }
        if (kClose_Verb == v) {
            return true;
        }
    }
    return false;
}

SkPath::Verb SkPath::Iter::autoClose(SkPoint pts[2]) {
    if (fLastPt != fMoveTo) {
        
        
        
        if (SkScalarIsNaN(fLastPt.fX) || SkScalarIsNaN(fLastPt.fY) ||
            SkScalarIsNaN(fMoveTo.fX) || SkScalarIsNaN(fMoveTo.fY)) {
            return kClose_Verb;
        }

        if (pts) {
            pts[0] = fLastPt;
            pts[1] = fMoveTo;
        }
        fLastPt = fMoveTo;
        fCloseLine = true;
        return kLine_Verb;
    } else {
        pts[0] = fMoveTo;
        return kClose_Verb;
    }
}

bool SkPath::Iter::cons_moveTo(SkPoint pts[1]) {
    if (fNeedMoveTo == kAfterClose_NeedMoveToState) {
        if (pts) {
            *pts = fMoveTo;
        }
        fNeedClose = fForceClose;
        fNeedMoveTo = kAfterCons_NeedMoveToState;
        fVerbs -= 1;
        return true;
    }

    if (fNeedMoveTo == kAfterCons_NeedMoveToState) {
        if (pts) {
            *pts = fMoveTo;
        }
        fNeedMoveTo = kAfterPrefix_NeedMoveToState;
    } else {
        SkASSERT(fNeedMoveTo == kAfterPrefix_NeedMoveToState);
        if (pts) {
            *pts = fPts[-1];
        }
    }
    return false;
}

SkPath::Verb SkPath::Iter::next(SkPoint pts[4]) {
    if (fVerbs == fVerbStop) {
        if (fNeedClose) {
            if (kLine_Verb == this->autoClose(pts)) {
                return kLine_Verb;
            }
            fNeedClose = false;
            return kClose_Verb;
        }
        return kDone_Verb;
    }

    unsigned        verb = *fVerbs++;
    const SkPoint*  srcPts = fPts;

    switch (verb) {
        case kMove_Verb:
            if (fNeedClose) {
                fVerbs -= 1;
                verb = this->autoClose(pts);
                if (verb == kClose_Verb) {
                    fNeedClose = false;
                }
                return (Verb)verb;
            }
            if (fVerbs == fVerbStop) {    
                return kDone_Verb;
            }
            fMoveTo = *srcPts;
            if (pts) {
                pts[0] = *srcPts;
            }
            srcPts += 1;
            fNeedMoveTo = kAfterCons_NeedMoveToState;
            fNeedClose = fForceClose;
            break;
        case kLine_Verb:
            if (this->cons_moveTo(pts)) {
                return kMove_Verb;
            }
            if (pts) {
                pts[1] = srcPts[0];
            }
            fLastPt = srcPts[0];
            fCloseLine = false;
            srcPts += 1;
            break;
        case kQuad_Verb:
            if (this->cons_moveTo(pts)) {
                return kMove_Verb;
            }
            if (pts) {
                memcpy(&pts[1], srcPts, 2 * sizeof(SkPoint));
            }
            fLastPt = srcPts[1];
            srcPts += 2;
            break;
        case kCubic_Verb:
            if (this->cons_moveTo(pts)) {
                return kMove_Verb;
            }
            if (pts) {
                memcpy(&pts[1], srcPts, 3 * sizeof(SkPoint));
            }
            fLastPt = srcPts[2];
            srcPts += 3;
            break;
        case kClose_Verb:
            verb = this->autoClose(pts);
            if (verb == kLine_Verb) {
                fVerbs -= 1;
            } else {
                fNeedClose = false;
            }
            fNeedMoveTo = kAfterClose_NeedMoveToState;
            break;
    }
    fPts = srcPts;
    return (Verb)verb;
}







void SkPath::flatten(SkWriter32& buffer) const {
    SkDEBUGCODE(this->validate();)

    buffer.write32(fPts.count());
    buffer.write32(fVerbs.count());
    buffer.write32((fFillType << 8) | fSegmentMask);
    buffer.writeMul4(fPts.begin(), sizeof(SkPoint) * fPts.count());
    buffer.writePad(fVerbs.begin(), fVerbs.count());
}

void SkPath::unflatten(SkReader32& buffer) {
    fPts.setCount(buffer.readS32());
    fVerbs.setCount(buffer.readS32());
    uint32_t packed = buffer.readS32();
    fFillType = packed >> 8;
    fSegmentMask = packed & 0xFF;
    buffer.read(fPts.begin(), sizeof(SkPoint) * fPts.count());
    buffer.read(fVerbs.begin(), fVerbs.count());

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;

    SkDEBUGCODE(this->validate();)
}




void SkPath::dump(bool forceClose, const char title[]) const {
    Iter    iter(*this, forceClose);
    SkPoint pts[4];
    Verb    verb;

    SkDebugf("path: forceClose=%s %s\n", forceClose ? "true" : "false",
             title ? title : "");

    while ((verb = iter.next(pts)) != kDone_Verb) {
        switch (verb) {
            case kMove_Verb:
#ifdef SK_CAN_USE_FLOAT
                SkDebugf("  path: moveTo [%g %g]\n",
                        SkScalarToFloat(pts[0].fX), SkScalarToFloat(pts[0].fY));
#else
                SkDebugf("  path: moveTo [%x %x]\n", pts[0].fX, pts[0].fY);
#endif
                break;
            case kLine_Verb:
#ifdef SK_CAN_USE_FLOAT
                SkDebugf("  path: lineTo [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY));
#else
                SkDebugf("  path: lineTo [%x %x]\n", pts[1].fX, pts[1].fY);
#endif
                break;
            case kQuad_Verb:
#ifdef SK_CAN_USE_FLOAT
                SkDebugf("  path: quadTo [%g %g] [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY),
                        SkScalarToFloat(pts[2].fX), SkScalarToFloat(pts[2].fY));
#else
                SkDebugf("  path: quadTo [%x %x] [%x %x]\n",
                         pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY);
#endif
                break;
            case kCubic_Verb:
#ifdef SK_CAN_USE_FLOAT
                SkDebugf("  path: cubeTo [%g %g] [%g %g] [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY),
                        SkScalarToFloat(pts[2].fX), SkScalarToFloat(pts[2].fY),
                        SkScalarToFloat(pts[3].fX), SkScalarToFloat(pts[3].fY));
#else
                SkDebugf("  path: cubeTo [%x %x] [%x %x] [%x %x]\n",
                         pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY,
                         pts[3].fX, pts[3].fY);
#endif
                break;
            case kClose_Verb:
                SkDebugf("  path: close\n");
                break;
            default:
                SkDebugf("  path: UNKNOWN VERB %d, aborting dump...\n", verb);
                verb = kDone_Verb;  
                break;
        }
    }
    SkDebugf("path: done %s\n", title ? title : "");
}

void SkPath::dump() const {
    this->dump(false);
}

#ifdef SK_DEBUG
void SkPath::validate() const {
    SkASSERT(this != NULL);
    SkASSERT((fFillType & ~3) == 0);
    fPts.validate();
    fVerbs.validate();

    if (!fBoundsIsDirty) {
        SkRect bounds;
        compute_pt_bounds(&bounds, fPts);
        if (fPts.count() <= 1) {
            
            
            
            
            SkASSERT(bounds.isEmpty());
            SkASSERT(fBounds.isEmpty());
        } else {
            fBounds.contains(bounds);
        }
    }

    uint32_t mask = 0;
    for (int i = 0; i < fVerbs.count(); i++) {
        switch (fVerbs[i]) {
            case kLine_Verb:
                mask |= kLine_SegmentMask;
                break;
            case kQuad_Verb:
                mask |= kQuad_SegmentMask;
                break;
            case kCubic_Verb:
                mask |= kCubic_SegmentMask;
        }
    }
    SkASSERT(mask == fSegmentMask);
}
#endif



static int sign(SkScalar x) { return x < 0; }
#define kValueNeverReturnedBySign   2

static int CrossProductSign(const SkVector& a, const SkVector& b) {
    return SkScalarSignAsInt(SkPoint::CrossProduct(a, b));
}


struct Convexicator {
    Convexicator() : fPtCount(0), fConvexity(SkPath::kConvex_Convexity) {
        fSign = 0;
        
        fCurrPt.set(0, 0);
        fVec0.set(0, 0);
        fVec1.set(0, 0);
        fFirstVec.set(0, 0);

        fDx = fDy = 0;
        fSx = fSy = kValueNeverReturnedBySign;
    }

    SkPath::Convexity getConvexity() const { return fConvexity; }

    void addPt(const SkPoint& pt) {
        if (SkPath::kConcave_Convexity == fConvexity) {
            return;
        }

        if (0 == fPtCount) {
            fCurrPt = pt;
            ++fPtCount;
        } else {
            SkVector vec = pt - fCurrPt;
            if (vec.fX || vec.fY) {
                fCurrPt = pt;
                if (++fPtCount == 2) {
                    fFirstVec = fVec1 = vec;
                } else {
                    SkASSERT(fPtCount > 2);
                    this->addVec(vec);
                }
                
                int sx = sign(vec.fX);
                int sy = sign(vec.fY);
                fDx += (sx != fSx);
                fDy += (sy != fSy);
                fSx = sx;
                fSy = sy;
                
                if (fDx > 3 || fDy > 3) {
                    fConvexity = SkPath::kConcave_Convexity;
                }
            }
        }
    }

    void close() {
        if (fPtCount > 2) {
            this->addVec(fFirstVec);
        }
    }

private:
    void addVec(const SkVector& vec) {
        SkASSERT(vec.fX || vec.fY);
        fVec0 = fVec1;
        fVec1 = vec;
        int sign = CrossProductSign(fVec0, fVec1);
        if (0 == fSign) {
            fSign = sign;
        } else if (sign) {
            if (fSign != sign) {
                fConvexity = SkPath::kConcave_Convexity;
            }
        }
    }

    SkPoint             fCurrPt;
    SkVector            fVec0, fVec1, fFirstVec;
    int                 fPtCount;   
    int                 fSign;
    SkPath::Convexity   fConvexity;
    int                 fDx, fDy, fSx, fSy;
};

SkPath::Convexity SkPath::ComputeConvexity(const SkPath& path) {
    SkPoint         pts[4];
    SkPath::Verb    verb;
    SkPath::Iter    iter(path, true);

    int             contourCount = 0;
    int             count;
    Convexicator    state;

    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        switch (verb) {
            case kMove_Verb:
                if (++contourCount > 1) {
                    return kConcave_Convexity;
                }
                pts[1] = pts[0];
                count = 1;
                break;
            case kLine_Verb: count = 1; break;
            case kQuad_Verb: count = 2; break;
            case kCubic_Verb: count = 3; break;
            case kClose_Verb:
                state.close();
                count = 0;
                break;
            default:
                SkASSERT(!"bad verb");
                return kConcave_Convexity;
        }

        for (int i = 1; i <= count; i++) {
            state.addPt(pts[i]);
        }
        
        if (kConcave_Convexity == state.getConvexity()) {
            return kConcave_Convexity;
        }
    }
    return state.getConvexity();
}
