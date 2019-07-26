








#include "SkPath.h"
#include "SkBuffer.h"
#include "SkMath.h"
#include "SkPathRef.h"
#include "SkThread.h"

SK_DEFINE_INST_COUNT(SkPath);




#define MIN_COUNT_FOR_MEMSET_TO_BE_FAST 16








static void joinNoEmptyChecks(SkRect* dst, const SkRect& src) {
    dst->fLeft = SkMinScalar(dst->fLeft, src.fLeft);
    dst->fTop = SkMinScalar(dst->fTop, src.fTop);
    dst->fRight = SkMaxScalar(dst->fRight, src.fRight);
    dst->fBottom = SkMaxScalar(dst->fBottom, src.fBottom);
}

static bool is_degenerate(const SkPath& path) {
    SkPath::Iter iter(path, false);
    SkPoint pts[4];
    return SkPath::kDone_Verb == iter.next(pts);
}

class SkAutoDisableOvalCheck {
public:
    SkAutoDisableOvalCheck(SkPath* path) : fPath(path) {
        fSaved = fPath->fIsOval;
    }

    ~SkAutoDisableOvalCheck() {
        fPath->fIsOval = fSaved;
    }

private:
    SkPath* fPath;
    bool    fSaved;
};













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
        fPath->setIsConvex(fDegenerate);
        if (fEmpty) {
            fPath->fBounds = fRect;
            fPath->fBoundsIsDirty = false;
            fPath->fIsFinite = fPath->fBounds.isFinite();
        } else if (!fDirty) {
            joinNoEmptyChecks(&fPath->fBounds, fRect);
            fPath->fBoundsIsDirty = false;
            fPath->fIsFinite = fPath->fBounds.isFinite();
        }
    }

private:
    SkPath* fPath;
    SkRect  fRect;
    bool    fDirty;
    bool    fDegenerate;
    bool    fEmpty;

    
    void init(SkPath* path) {
        fPath = path;
        fDirty = SkToBool(path->fBoundsIsDirty);
        fDegenerate = is_degenerate(*path);
        fEmpty = path->isEmpty();
        
        fRect.sort();
    }
};


static bool compute_pt_bounds(SkRect* bounds, const SkPathRef& ref) {
    int count = ref.countPoints();
    if (count <= 1) {  
        bounds->setEmpty();
        return count ? ref.points()->isFinite() : true;
    } else {
        return bounds->setBoundsCheck(ref.points(), count);
    }
}


















#define INITIAL_LASTMOVETOINDEX_VALUE   ~0

SkPath::SkPath()
    : fPathRef(SkPathRef::CreateEmpty())
    , fFillType(kWinding_FillType)
    , fBoundsIsDirty(true) {
    fConvexity = kUnknown_Convexity;
    fSegmentMask = 0;
    fLastMoveToIndex = INITIAL_LASTMOVETOINDEX_VALUE;
    fIsOval = false;
    fIsFinite = false;  
#ifdef SK_BUILD_FOR_ANDROID
    fGenerationID = 0;
    fSourcePath = NULL;
#endif
}

SkPath::SkPath(const SkPath& src) {
    SkDEBUGCODE(src.validate();)
    src.fPathRef.get()->ref();
    fPathRef.reset(src.fPathRef.get());
    fBounds         = src.fBounds;
    fFillType       = src.fFillType;
    fBoundsIsDirty  = src.fBoundsIsDirty;
    fConvexity      = src.fConvexity;
    fIsFinite       = src.fIsFinite;
    fSegmentMask    = src.fSegmentMask;
    fLastMoveToIndex = src.fLastMoveToIndex;
    fIsOval         = src.fIsOval;
#ifdef SK_BUILD_FOR_ANDROID
    fGenerationID = src.fGenerationID;
    fSourcePath = NULL;
#endif
}

SkPath::~SkPath() {
    SkDEBUGCODE(this->validate();)
}

SkPath& SkPath::operator=(const SkPath& src) {
    SkDEBUGCODE(src.validate();)

    if (this != &src) {
        src.fPathRef.get()->ref();
        fPathRef.reset(src.fPathRef.get());
        fBounds         = src.fBounds;
        fFillType       = src.fFillType;
        fBoundsIsDirty  = src.fBoundsIsDirty;
        fConvexity      = src.fConvexity;
        fIsFinite       = src.fIsFinite;
        fSegmentMask    = src.fSegmentMask;
        fLastMoveToIndex = src.fLastMoveToIndex;
        fIsOval         = src.fIsOval;
        GEN_ID_INC;
    }
    SkDEBUGCODE(this->validate();)
    return *this;
}

SK_API bool operator==(const SkPath& a, const SkPath& b) {
    
    

    
    
    

    return &a == &b ||
        (a.fFillType == b.fFillType && a.fSegmentMask == b.fSegmentMask &&
         *a.fPathRef.get() == *b.fPathRef.get());
}

void SkPath::swap(SkPath& other) {
    SkASSERT(&other != NULL);

    if (this != &other) {
        SkTSwap<SkRect>(fBounds, other.fBounds);
        fPathRef.swap(&other.fPathRef);
        SkTSwap<uint8_t>(fFillType, other.fFillType);
        SkTSwap<uint8_t>(fBoundsIsDirty, other.fBoundsIsDirty);
        SkTSwap<uint8_t>(fConvexity, other.fConvexity);
        SkTSwap<uint8_t>(fSegmentMask, other.fSegmentMask);
        SkTSwap<int>(fLastMoveToIndex, other.fLastMoveToIndex);
        SkTSwap<SkBool8>(fIsOval, other.fIsOval);
        SkTSwap<SkBool8>(fIsFinite, other.fIsFinite);
        GEN_ID_INC;
    }
}

#ifdef SK_BUILD_FOR_ANDROID
uint32_t SkPath::getGenerationID() const {
    return fGenerationID;
}

const SkPath* SkPath::getSourcePath() const {
    return fSourcePath;
}

void SkPath::setSourcePath(const SkPath* path) {
    fSourcePath = path;
}
#endif

void SkPath::reset() {
    SkDEBUGCODE(this->validate();)

    fPathRef.reset(SkPathRef::CreateEmpty());
    GEN_ID_INC;
    fBoundsIsDirty = true;
    fConvexity = kUnknown_Convexity;
    fSegmentMask = 0;
    fLastMoveToIndex = INITIAL_LASTMOVETOINDEX_VALUE;
    fIsOval = false;
}

void SkPath::rewind() {
    SkDEBUGCODE(this->validate();)

    SkPathRef::Rewind(&fPathRef);
    GEN_ID_INC;
    fConvexity = kUnknown_Convexity;
    fBoundsIsDirty = true;
    fSegmentMask = 0;
    fLastMoveToIndex = INITIAL_LASTMOVETOINDEX_VALUE;
    fIsOval = false;
}

bool SkPath::isEmpty() const {
    SkDEBUGCODE(this->validate();)
    return 0 == fPathRef->countVerbs();
}

bool SkPath::isLine(SkPoint line[2]) const {
    int verbCount = fPathRef->countVerbs();
    int ptCount = fPathRef->countVerbs();

    if (2 == verbCount && 2 == ptCount) {
        if (kMove_Verb == fPathRef->atVerb(0) &&
            kLine_Verb == fPathRef->atVerb(1)) {
            if (line) {
                const SkPoint* pts = fPathRef->points();
                line[0] = pts[0];
                line[1] = pts[1];
            }
            return true;
        }
    }
    return false;
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
    const SkPoint* pts = fPathRef->points();
    int verbCnt = fPathRef->countVerbs();
    int currVerb = 0;
    while (currVerb < verbCnt) {
        switch (fPathRef->atVerb(currVerb++)) {
            case kClose_Verb:
                pts = fPathRef->points();
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

int SkPath::countPoints() const {
    return fPathRef->countPoints();
}

int SkPath::getPoints(SkPoint dst[], int max) const {
    SkDEBUGCODE(this->validate();)

    SkASSERT(max >= 0);
    SkASSERT(!max || dst);
    int count = SkMin32(max, fPathRef->countPoints());
    memcpy(dst, fPathRef->points(), count * sizeof(SkPoint));
    return fPathRef->countPoints();
}

SkPoint SkPath::getPoint(int index) const {
    if ((unsigned)index < (unsigned)fPathRef->countPoints()) {
        return fPathRef->atPoint(index);
    }
    return SkPoint::Make(0, 0);
}

int SkPath::countVerbs() const {
    return fPathRef->countVerbs();
}

static inline void copy_verbs_reverse(uint8_t* inorderDst,
                                      const uint8_t* reversedSrc,
                                      int count) {
    for (int i = 0; i < count; ++i) {
        inorderDst[i] = reversedSrc[~i];
    }
}

int SkPath::getVerbs(uint8_t dst[], int max) const {
    SkDEBUGCODE(this->validate();)

    SkASSERT(max >= 0);
    SkASSERT(!max || dst);
    int count = SkMin32(max, fPathRef->countVerbs());
    copy_verbs_reverse(dst, fPathRef->verbs(), count);
    return fPathRef->countVerbs();
}

bool SkPath::getLastPt(SkPoint* lastPt) const {
    SkDEBUGCODE(this->validate();)

    int count = fPathRef->countPoints();
    if (count > 0) {
        if (lastPt) {
            *lastPt = fPathRef->atPoint(count - 1);
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

    int count = fPathRef->countPoints();
    if (count == 0) {
        this->moveTo(x, y);
    } else {
        fIsOval = false;
        SkPathRef::Editor ed(&fPathRef);
        ed.atPoint(count-1)->set(x, y);
        GEN_ID_INC;
    }
}

void SkPath::computeBounds() const {
    SkDEBUGCODE(this->validate();)
    SkASSERT(fBoundsIsDirty);

    fIsFinite = compute_pt_bounds(&fBounds, *fPathRef.get());
    fBoundsIsDirty = false;
}

void SkPath::setConvexity(Convexity c) {
    if (fConvexity != c) {
        fConvexity = c;
        GEN_ID_INC;
    }
}




#define DIRTY_AFTER_EDIT                 \
    do {                                 \
        fBoundsIsDirty = true;           \
        fConvexity = kUnknown_Convexity; \
        fIsOval = false;                 \
    } while (0)

#define DIRTY_AFTER_EDIT_NO_CONVEXITY_CHANGE    \
    do {                                        \
        fBoundsIsDirty = true;                  \
    } while (0)

void SkPath::incReserve(U16CPU inc) {
    SkDEBUGCODE(this->validate();)
    SkPathRef::Editor(&fPathRef, inc, inc);
    SkDEBUGCODE(this->validate();)
}

void SkPath::moveTo(SkScalar x, SkScalar y) {
    SkDEBUGCODE(this->validate();)

    SkPathRef::Editor ed(&fPathRef);

    
    fLastMoveToIndex = ed.pathRef()->countPoints();

    ed.growForVerb(kMove_Verb)->set(x, y);

    GEN_ID_INC;
    DIRTY_AFTER_EDIT_NO_CONVEXITY_CHANGE;
}

void SkPath::rMoveTo(SkScalar x, SkScalar y) {
    SkPoint pt;
    this->getLastPt(&pt);
    this->moveTo(pt.fX + x, pt.fY + y);
}

void SkPath::injectMoveToIfNeeded() {
    if (fLastMoveToIndex < 0) {
        SkScalar x, y;
        if (fPathRef->countVerbs() == 0) {
            x = y = 0;
        } else {
            const SkPoint& pt = fPathRef->atPoint(~fLastMoveToIndex);
            x = pt.fX;
            y = pt.fY;
        }
        this->moveTo(x, y);
    }
}

void SkPath::lineTo(SkScalar x, SkScalar y) {
    SkDEBUGCODE(this->validate();)

    this->injectMoveToIfNeeded();

    SkPathRef::Editor ed(&fPathRef);
    ed.growForVerb(kLine_Verb)->set(x, y);
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

    this->injectMoveToIfNeeded();

    SkPathRef::Editor ed(&fPathRef);
    SkPoint* pts = ed.growForVerb(kQuad_Verb);
    pts[0].set(x1, y1);
    pts[1].set(x2, y2);
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

    this->injectMoveToIfNeeded();

    SkPathRef::Editor ed(&fPathRef);
    SkPoint* pts = ed.growForVerb(kCubic_Verb);
    pts[0].set(x1, y1);
    pts[1].set(x2, y2);
    pts[2].set(x3, y3);
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

    int count = fPathRef->countVerbs();
    if (count > 0) {
        switch (fPathRef->atVerb(count - 1)) {
            case kLine_Verb:
            case kQuad_Verb:
            case kCubic_Verb:
            case kMove_Verb: {
                SkPathRef::Editor ed(&fPathRef);
                ed.growForVerb(kClose_Verb);
                GEN_ID_INC;
                break;
            }
            default:
                
                break;
        }
    }

    
#if 0
    if (fLastMoveToIndex >= 0) {
        fLastMoveToIndex = ~fLastMoveToIndex;
    }
#else
    fLastMoveToIndex ^= ~fLastMoveToIndex >> (8 * sizeof(fLastMoveToIndex) - 1);
#endif
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

void SkPath::addPoly(const SkPoint pts[], int count, bool close) {
    SkDEBUGCODE(this->validate();)
    if (count <= 0) {
        return;
    }

    SkPathRef::Editor ed(&fPathRef);
    fLastMoveToIndex = ed.pathRef()->countPoints();
    uint8_t* vb;
    SkPoint* p;
    
    ed.grow(count + close, count, &vb, &p);

    memcpy(p, pts, count * sizeof(SkPoint));
    vb[~0] = kMove_Verb;
    if (count > 1) {
        
        
        if ((unsigned)count >= MIN_COUNT_FOR_MEMSET_TO_BE_FAST) {
            memset(vb - count, kLine_Verb, count - 1);
        } else {
            for (int i = 1; i < count; ++i) {
                vb[~i] = kLine_Verb;
            }
        }
        fSegmentMask |= kLine_SegmentMask;
    }
    if (close) {
        vb[~count] = kClose_Verb;
    }

    GEN_ID_INC;
    DIRTY_AFTER_EDIT;
    SkDEBUGCODE(this->validate();)
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
        default: SkDEBUGFAIL("unexpected startAngle in add_corner_arc");
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

bool SkPath::hasOnlyMoveTos() const {
    int count = fPathRef->countVerbs();
    const uint8_t* verbs = const_cast<const SkPathRef*>(fPathRef.get())->verbsMemBegin();
    for (int i = 0; i < count; ++i) {
        if (*verbs == kLine_Verb ||
            *verbs == kQuad_Verb ||
            *verbs == kCubic_Verb) {
            return false;
        }
        ++verbs;
    }
    return true;
}

void SkPath::addOval(const SkRect& oval, Direction dir) {
    





    fIsOval = hasOnlyMoveTos();

    SkAutoDisableOvalCheck adoc(this);

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

bool SkPath::isOval(SkRect* rect) const {
    if (fIsOval && rect) {
        *rect = getBounds();
    }

    return fIsOval;
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

    if (0 == sweepAngle &&
        (0 == startAngle || SkIntToScalar(360) == startAngle)) {
        
        
        
        pts[0].set(oval.fRight, oval.centerY());
        return 1;
    }

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

    if (fPathRef->countVerbs() == 0) {
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
    SkPathRef::Editor(&fPathRef, path.countVerbs(), path.countPoints());

    fIsOval = false;

    RawIter iter(path);
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
                SkDEBUGFAIL("unknown verb");
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
    int i, vcount = path.fPathRef->countVerbs();
    
    if (vcount < 2) {
        return;
    }

    SkPathRef::Editor(&fPathRef, vcount, path.countPoints());

    fIsOval = false;

    const uint8_t* verbs = path.fPathRef->verbs();
    
    const SkPoint*  pts = path.fPathRef->points() + 1;

    SkASSERT(verbs[~0] == kMove_Verb);
    for (i = 1; i < vcount; i++) {
        switch (verbs[~i]) {
            case kLine_Verb:
                this->lineTo(pts[0].fX, pts[0].fY);
                break;
            case kQuad_Verb:
                this->quadTo(pts[0].fX, pts[0].fY, pts[1].fX, pts[1].fY);
                break;
            case kCubic_Verb:
                this->cubicTo(pts[0].fX, pts[0].fY, pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY);
                break;
            case kClose_Verb:
                return;
        }
        pts += gPtsInVerb[verbs[~i]];
    }
}


void SkPath::reversePathTo(const SkPath& path) {
    int i, vcount = path.fPathRef->countVerbs();
    
    if (vcount < 2) {
        return;
    }

    SkPathRef::Editor(&fPathRef, vcount, path.countPoints());

    fIsOval = false;

    const uint8_t*  verbs = path.fPathRef->verbs();
    const SkPoint*  pts = path.fPathRef->points();

    SkASSERT(verbs[~0] == kMove_Verb);
    for (i = 1; i < vcount; ++i) {
        int n = gPtsInVerb[verbs[~i]];
        if (n == 0) {
            break;
        }
        pts += n;
    }

    while (--i > 0) {
        switch (verbs[~i]) {
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
                SkDEBUGFAIL("bad verb");
                break;
        }
        pts -= gPtsInVerb[verbs[~i]];
    }
}

void SkPath::reverseAddPath(const SkPath& src) {
    SkPathRef::Editor ed(&fPathRef, src.fPathRef->countPoints(), src.fPathRef->countVerbs());

    const SkPoint* pts = src.fPathRef->pointsEnd();
    
    const uint8_t* verbs = src.fPathRef->verbsMemBegin(); 
    const uint8_t* verbsEnd = src.fPathRef->verbs(); 

    fIsOval = false;

    bool needMove = true;
    bool needClose = false;
    while (verbs < verbsEnd) {
        uint8_t v = *(verbs++);
        int n = gPtsInVerb[v];

        if (needMove) {
            --pts;
            this->moveTo(pts->fX, pts->fY);
            needMove = false;
        }
        pts -= n;
        switch (v) {
            case kMove_Verb:
                if (needClose) {
                    this->close();
                    needClose = false;
                }
                needMove = true;
                pts += 1;   
                break;
            case kLine_Verb:
                this->lineTo(pts[0]);
                break;
            case kQuad_Verb:
                this->quadTo(pts[1], pts[0]);
                break;
            case kCubic_Verb:
                this->cubicTo(pts[2], pts[1], pts[0]);
                break;
            case kClose_Verb:
                needClose = true;
                break;
            default:
                SkASSERT(!"unexpected verb");
        }
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

        while ((verb = iter.next(pts, false)) != kDone_Verb) {
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
                    SkDEBUGFAIL("unknown verb");
                    break;
            }
        }

        dst->swap(tmp);
        SkPathRef::Editor ed(&dst->fPathRef);
        matrix.mapPoints(ed.points(), ed.pathRef()->countPoints());
    } else {
        












        if (!fBoundsIsDirty && matrix.rectStaysRect() && fPathRef->countPoints() > 1) {
            dst->fBoundsIsDirty = false;
            if (fIsFinite) {
                matrix.mapRect(&dst->fBounds, fBounds);
                if (!(dst->fIsFinite = dst->fBounds.isFinite())) {
                    dst->fBounds.setEmpty();
                }
            } else {
                dst->fIsFinite = false;
                dst->fBounds.setEmpty();
            }
        } else {
            GEN_ID_PTR_INC(dst);
            dst->fBoundsIsDirty = true;
        }

        SkPathRef::CreateTransformedCopy(&dst->fPathRef, *fPathRef.get(), matrix);

        if (this != dst) {
            dst->fFillType = fFillType;
            dst->fSegmentMask = fSegmentMask;
            dst->fConvexity = fConvexity;
        }

        
        dst->fIsOval = fIsOval && matrix.rectStaysRect();

        SkDEBUGCODE(dst->validate();)
    }
}




enum SegmentState {
    kEmptyContour_SegmentState,   
                                  
                                  
    kAfterMove_SegmentState,      
    kAfterPrimitive_SegmentState  
                                  
};

SkPath::Iter::Iter() {
#ifdef SK_DEBUG
    fPts = NULL;
    fMoveTo.fX = fMoveTo.fY = fLastPt.fX = fLastPt.fY = 0;
    fForceClose = fCloseLine = false;
    fSegmentState = kEmptyContour_SegmentState;
#endif
    
    fVerbs = NULL;
    fVerbStop = NULL;
    fNeedClose = false;
}

SkPath::Iter::Iter(const SkPath& path, bool forceClose) {
    this->setPath(path, forceClose);
}

void SkPath::Iter::setPath(const SkPath& path, bool forceClose) {
    fPts = path.fPathRef->points();
    fVerbs = path.fPathRef->verbs();
    fVerbStop = path.fPathRef->verbsMemBegin();
    fLastPt.fX = fLastPt.fY = 0;
    fMoveTo.fX = fMoveTo.fY = 0;
    fForceClose = SkToU8(forceClose);
    fNeedClose = false;
    fSegmentState = kEmptyContour_SegmentState;
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

    if (kMove_Verb == *(verbs - 1)) {
        verbs -= 1; 
    }

    while (verbs > stop) {
        
        unsigned v = *(--verbs);
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
    SkASSERT(pts);
    if (fLastPt != fMoveTo) {
        
        
        
        if (SkScalarIsNaN(fLastPt.fX) || SkScalarIsNaN(fLastPt.fY) ||
            SkScalarIsNaN(fMoveTo.fX) || SkScalarIsNaN(fMoveTo.fY)) {
            return kClose_Verb;
        }

        pts[0] = fLastPt;
        pts[1] = fMoveTo;
        fLastPt = fMoveTo;
        fCloseLine = true;
        return kLine_Verb;
    } else {
        pts[0] = fMoveTo;
        return kClose_Verb;
    }
}

const SkPoint& SkPath::Iter::cons_moveTo() {
    if (fSegmentState == kAfterMove_SegmentState) {
        
        fSegmentState = kAfterPrimitive_SegmentState;
        return fMoveTo;
    } else {
        SkASSERT(fSegmentState == kAfterPrimitive_SegmentState);
         
        return fPts[-1];
    }
}

void SkPath::Iter::consumeDegenerateSegments() {
    
    
    const uint8_t* lastMoveVerb = 0;
    const SkPoint* lastMovePt = 0;
    SkPoint lastPt = fLastPt;
    while (fVerbs != fVerbStop) {
        unsigned verb = *(fVerbs - 1); 
        switch (verb) {
            case kMove_Verb:
                
                lastMoveVerb = fVerbs;
                lastMovePt = fPts;
                lastPt = fPts[0];
                fVerbs--;
                fPts++;
                break;

            case kClose_Verb:
                
                
                if (fSegmentState == kAfterPrimitive_SegmentState && !lastMoveVerb) {
                    return;
                }
                
                fVerbs--;
                break;

            case kLine_Verb:
                if (!IsLineDegenerate(lastPt, fPts[0])) {
                    if (lastMoveVerb) {
                        fVerbs = lastMoveVerb;
                        fPts = lastMovePt;
                        return;
                    }
                    return;
                }
                
                fVerbs--;
                fPts++;
                break;

            case kQuad_Verb:
                if (!IsQuadDegenerate(lastPt, fPts[0], fPts[1])) {
                    if (lastMoveVerb) {
                        fVerbs = lastMoveVerb;
                        fPts = lastMovePt;
                        return;
                    }
                    return;
                }
                
                fVerbs--;
                fPts += 2;
                break;

            case kCubic_Verb:
                if (!IsCubicDegenerate(lastPt, fPts[0], fPts[1], fPts[2])) {
                    if (lastMoveVerb) {
                        fVerbs = lastMoveVerb;
                        fPts = lastMovePt;
                        return;
                    }
                    return;
                }
                
                fVerbs--;
                fPts += 3;
                break;

            default:
                SkDEBUGFAIL("Should never see kDone_Verb");
        }
    }
}

SkPath::Verb SkPath::Iter::doNext(SkPoint ptsParam[4]) {
    SkASSERT(ptsParam);

    if (fVerbs == fVerbStop) {
        
        if (fNeedClose && fSegmentState == kAfterPrimitive_SegmentState) {
            if (kLine_Verb == this->autoClose(ptsParam)) {
                return kLine_Verb;
            }
            fNeedClose = false;
            return kClose_Verb;
        }
        return kDone_Verb;
    }

    
    unsigned verb = *(--fVerbs);
    const SkPoint* SK_RESTRICT srcPts = fPts;
    SkPoint* SK_RESTRICT       pts = ptsParam;

    switch (verb) {
        case kMove_Verb:
            if (fNeedClose) {
                fVerbs++; 
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
            pts[0] = *srcPts;
            srcPts += 1;
            fSegmentState = kAfterMove_SegmentState;
            fLastPt = fMoveTo;
            fNeedClose = fForceClose;
            break;
        case kLine_Verb:
            pts[0] = this->cons_moveTo();
            pts[1] = srcPts[0];
            fLastPt = srcPts[0];
            fCloseLine = false;
            srcPts += 1;
            break;
        case kQuad_Verb:
            pts[0] = this->cons_moveTo();
            memcpy(&pts[1], srcPts, 2 * sizeof(SkPoint));
            fLastPt = srcPts[1];
            srcPts += 2;
            break;
        case kCubic_Verb:
            pts[0] = this->cons_moveTo();
            memcpy(&pts[1], srcPts, 3 * sizeof(SkPoint));
            fLastPt = srcPts[2];
            srcPts += 3;
            break;
        case kClose_Verb:
            verb = this->autoClose(pts);
            if (verb == kLine_Verb) {
                fVerbs++; 
            } else {
                fNeedClose = false;
                fSegmentState = kEmptyContour_SegmentState;
            }
            fLastPt = fMoveTo;
            break;
    }
    fPts = srcPts;
    return (Verb)verb;
}



SkPath::RawIter::RawIter() {
#ifdef SK_DEBUG
    fPts = NULL;
    fMoveTo.fX = fMoveTo.fY = fLastPt.fX = fLastPt.fY = 0;
#endif
    
    fVerbs = NULL;
    fVerbStop = NULL;
}

SkPath::RawIter::RawIter(const SkPath& path) {
    this->setPath(path);
}

void SkPath::RawIter::setPath(const SkPath& path) {
    fPts = path.fPathRef->points();
    fVerbs = path.fPathRef->verbs();
    fVerbStop = path.fPathRef->verbsMemBegin();
    fMoveTo.fX = fMoveTo.fY = 0;
    fLastPt.fX = fLastPt.fY = 0;
}

SkPath::Verb SkPath::RawIter::next(SkPoint pts[4]) {
    SkASSERT(NULL != pts);
    if (fVerbs == fVerbStop) {
        return kDone_Verb;
    }

    
    unsigned verb = *(--fVerbs);
    const SkPoint* srcPts = fPts;

    switch (verb) {
        case kMove_Verb:
            pts[0] = *srcPts;
            fMoveTo = srcPts[0];
            fLastPt = fMoveTo;
            srcPts += 1;
            break;
        case kLine_Verb:
            pts[0] = fLastPt;
            pts[1] = srcPts[0];
            fLastPt = srcPts[0];
            srcPts += 1;
            break;
        case kQuad_Verb:
            pts[0] = fLastPt;
            memcpy(&pts[1], srcPts, 2 * sizeof(SkPoint));
            fLastPt = srcPts[1];
            srcPts += 2;
            break;
        case kCubic_Verb:
            pts[0] = fLastPt;
            memcpy(&pts[1], srcPts, 3 * sizeof(SkPoint));
            fLastPt = srcPts[2];
            srcPts += 3;
            break;
        case kClose_Verb:
            fLastPt = fMoveTo;
            pts[0] = fMoveTo;
            break;
    }
    fPts = srcPts;
    return (Verb)verb;
}







uint32_t SkPath::writeToMemory(void* storage) const {
    SkDEBUGCODE(this->validate();)

    if (NULL == storage) {
        const int byteCount = sizeof(int32_t)
#if NEW_PICTURE_FORMAT
                      + fPathRef->writeSize()
#else
                      + 2 * sizeof(int32_t)
                      + sizeof(SkPoint) * fPathRef->countPoints()
                      + sizeof(uint8_t) * fPathRef->countVerbs()
#endif
                      + sizeof(SkRect);
        return SkAlign4(byteCount);
    }

    SkWBuffer   buffer(storage);
#if !NEW_PICTURE_FORMAT
    buffer.write32(fPathRef->countPoints());
    buffer.write32(fPathRef->countVerbs());
#endif

    
    
    const SkRect& bounds = this->getBounds();
    SkASSERT(!fBoundsIsDirty);

    int32_t packed = ((fIsFinite & 1) << kIsFinite_SerializationShift) |
                     ((fIsOval & 1) << kIsOval_SerializationShift) |
                     (fConvexity << kConvexity_SerializationShift) |
                     (fFillType << kFillType_SerializationShift) |
                     (fSegmentMask << kSegmentMask_SerializationShift);

    buffer.write32(packed);

    fPathRef->writeToBuffer(&buffer);

    buffer.write(&bounds, sizeof(bounds));

    buffer.padToAlign4();
    return buffer.pos();
}

uint32_t SkPath::readFromMemory(const void* storage) {
    SkRBuffer   buffer(storage);
#if !NEW_PICTURE_FORMAT
    int32_t pcount = buffer.readS32();
    int32_t vcount = buffer.readS32();
#endif

    uint32_t packed = buffer.readS32();
    fIsFinite = (packed >> kIsFinite_SerializationShift) & 1;
    fIsOval = (packed >> kIsOval_SerializationShift) & 1;
    fConvexity = (packed >> kConvexity_SerializationShift) & 0xFF;
    fFillType = (packed >> kFillType_SerializationShift) & 0xFF;
    fSegmentMask = (packed >> kSegmentMask_SerializationShift) & 0xFF;

#if NEW_PICTURE_FORMAT
    fPathRef.reset(SkPathRef::CreateFromBuffer(&buffer));
#else
    fPathRef.reset(SkPathRef::CreateFromBuffer(vcount, pcount, &buffer));
#endif

    buffer.read(&fBounds, sizeof(fBounds));
    fBoundsIsDirty = false;

    buffer.skipToAlign4();

    GEN_ID_INC;

    SkDEBUGCODE(this->validate();)
    return buffer.pos();
}



void SkPath::dump(bool forceClose, const char title[]) const {
    Iter    iter(*this, forceClose);
    SkPoint pts[4];
    Verb    verb;

    SkDebugf("path: forceClose=%s %s\n", forceClose ? "true" : "false",
             title ? title : "");

    while ((verb = iter.next(pts, false)) != kDone_Verb) {
        switch (verb) {
            case kMove_Verb:
                SkDebugf("  path: moveTo [%g %g]\n",
                        SkScalarToFloat(pts[0].fX), SkScalarToFloat(pts[0].fY));
                break;
            case kLine_Verb:
                SkDebugf("  path: lineTo [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY));
                break;
            case kQuad_Verb:
                SkDebugf("  path: quadTo [%g %g] [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY),
                        SkScalarToFloat(pts[2].fX), SkScalarToFloat(pts[2].fY));
                break;
            case kCubic_Verb:
                SkDebugf("  path: cubeTo [%g %g] [%g %g] [%g %g]\n",
                        SkScalarToFloat(pts[1].fX), SkScalarToFloat(pts[1].fY),
                        SkScalarToFloat(pts[2].fX), SkScalarToFloat(pts[2].fY),
                        SkScalarToFloat(pts[3].fX), SkScalarToFloat(pts[3].fY));
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

    if (!fBoundsIsDirty) {
        SkRect bounds;

        bool isFinite = compute_pt_bounds(&bounds, *fPathRef.get());
        SkASSERT(SkToBool(fIsFinite) == isFinite);

        if (fPathRef->countPoints() <= 1) {
            
            
            
            
            SkASSERT(bounds.isEmpty());
            SkASSERT(fBounds.isEmpty());
        } else {
            if (bounds.isEmpty()) {
                SkASSERT(fBounds.isEmpty());
            } else {
                if (!fBounds.isEmpty()) {
                    SkASSERT(fBounds.contains(bounds));
                }
            }
        }
    }

    uint32_t mask = 0;
    const uint8_t* verbs = const_cast<const SkPathRef*>(fPathRef.get())->verbs();
    for (int i = 0; i < fPathRef->countVerbs(); i++) {
        switch (verbs[~i]) {
            case kLine_Verb:
                mask |= kLine_SegmentMask;
                break;
            case kQuad_Verb:
                mask |= kQuad_SegmentMask;
                break;
            case kCubic_Verb:
                mask |= kCubic_SegmentMask;
            case kMove_Verb:  
            case kClose_Verb:
                break;
            case kDone_Verb:
                SkDEBUGFAIL("Done verb shouldn't be recorded.");
                break;
            default:
                SkDEBUGFAIL("Unknown Verb");
                break;
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
                SkDEBUGFAIL("bad verb");
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



class ContourIter {
public:
    ContourIter(const SkPathRef& pathRef);

    bool done() const { return fDone; }
    
    int count() const { return fCurrPtCount; }
    const SkPoint* pts() const { return fCurrPt; }
    void next();

private:
    int fCurrPtCount;
    const SkPoint* fCurrPt;
    const uint8_t* fCurrVerb;
    const uint8_t* fStopVerbs;
    bool fDone;
    SkDEBUGCODE(int fContourCounter;)
};

ContourIter::ContourIter(const SkPathRef& pathRef) {
    fStopVerbs = pathRef.verbsMemBegin();
    fDone = false;
    fCurrPt = pathRef.points();
    fCurrVerb = pathRef.verbs();
    fCurrPtCount = 0;
    SkDEBUGCODE(fContourCounter = 0;)
    this->next();
}

void ContourIter::next() {
    if (fCurrVerb <= fStopVerbs) {
        fDone = true;
    }
    if (fDone) {
        return;
    }

    
    fCurrPt += fCurrPtCount;

    SkASSERT(SkPath::kMove_Verb == fCurrVerb[~0]);
    int ptCount = 1;    
    const uint8_t* verbs = fCurrVerb;

    for (--verbs; verbs > fStopVerbs; --verbs) {
        switch (verbs[~0]) {
            case SkPath::kMove_Verb:
                goto CONTOUR_END;
            case SkPath::kLine_Verb:
                ptCount += 1;
                break;
            case SkPath::kQuad_Verb:
                ptCount += 2;
                break;
            case SkPath::kCubic_Verb:
                ptCount += 3;
                break;
            default:    
                break;
        }
    }
CONTOUR_END:
    fCurrPtCount = ptCount;
    fCurrVerb = verbs;
    SkDEBUGCODE(++fContourCounter;)
}


static SkScalar cross_prod(const SkPoint& p0, const SkPoint& p1, const SkPoint& p2) {
    SkScalar cross = SkPoint::CrossProduct(p1 - p0, p2 - p0);
    
    
    if (0 == cross) {
        double p0x = SkScalarToDouble(p0.fX);
        double p0y = SkScalarToDouble(p0.fY);

        double p1x = SkScalarToDouble(p1.fX);
        double p1y = SkScalarToDouble(p1.fY);

        double p2x = SkScalarToDouble(p2.fX);
        double p2y = SkScalarToDouble(p2.fY);

        cross = SkDoubleToScalar((p1x - p0x) * (p2y - p0y) -
                                 (p1y - p0y) * (p2x - p0x));

    }
    return cross;
}


static int find_max_y(const SkPoint pts[], int count) {
    SkASSERT(count > 0);
    SkScalar max = pts[0].fY;
    int firstIndex = 0;
    for (int i = 1; i < count; ++i) {
        SkScalar y = pts[i].fY;
        if (y > max) {
            max = y;
            firstIndex = i;
        }
    }
    return firstIndex;
}

static int find_diff_pt(const SkPoint pts[], int index, int n, int inc) {
    int i = index;
    for (;;) {
        i = (i + inc) % n;
        if (i == index) {   
            break;
        }
        if (pts[index] != pts[i]) { 
            break;
        }
    }
    return i;
}





static int find_min_max_x_at_y(const SkPoint pts[], int index, int n,
                               int* maxIndexPtr) {
    const SkScalar y = pts[index].fY;
    SkScalar min = pts[index].fX;
    SkScalar max = min;
    int minIndex = index;
    int maxIndex = index;
    for (int i = index + 1; i < n; ++i) {
        if (pts[i].fY != y) {
            break;
        }
        SkScalar x = pts[i].fX;
        if (x < min) {
            min = x;
            minIndex = i;
        } else if (x > max) {
            max = x;
            maxIndex = i;
        }
    }
    *maxIndexPtr = maxIndex;
    return minIndex;
}

static bool crossToDir(SkScalar cross, SkPath::Direction* dir) {
    if (dir) {
        *dir = cross > 0 ? SkPath::kCW_Direction : SkPath::kCCW_Direction;
    }
    return true;
}

#if 0
#include "SkString.h"
#include "../utils/SkParsePath.h"
static void dumpPath(const SkPath& path) {
    SkString str;
    SkParsePath::ToSVGString(path, &str);
    SkDebugf("%s\n", str.c_str());
}
#endif

namespace {

double mul(double a, double b) { return a * b; }
SkScalar mul(SkScalar a, SkScalar b) { return SkScalarMul(a, b); }
double toDouble(SkScalar a) { return SkScalarToDouble(a); }
SkScalar toScalar(SkScalar a) { return a; }



template <typename T, T (CAST_SCALAR)(SkScalar)>
bool convex_dir_test(int n, const SkPoint pts[], SkPath::Direction* dir) {
    
    
    
    
    int i = 0;
    enum { kX = 0, kY = 1 };
    T v0[2];
    while (1) {
        v0[kX] = CAST_SCALAR(pts[i].fX) - CAST_SCALAR(pts[0].fX);
        v0[kY] = CAST_SCALAR(pts[i].fY) - CAST_SCALAR(pts[0].fY);
        if (v0[kX] || v0[kY]) {
            break;
        }
        if (++i == n - 1) {
            return false;
        }
    }
    
    
    
    for (++i; i < n; ++i) {
        T v1[2];
        v1[kX] = CAST_SCALAR(pts[i].fX) - CAST_SCALAR(pts[0].fX);
        v1[kY] = CAST_SCALAR(pts[i].fY) - CAST_SCALAR(pts[0].fY);
        T cross = mul(v0[kX], v1[kY]) - mul(v0[kY], v1[kX]);
        if (0 != cross) {
            *dir = cross > 0 ? SkPath::kCW_Direction : SkPath::kCCW_Direction;
            return true;
        }
    }
    return false;
}
}









bool SkPath::cheapComputeDirection(Direction* dir) const {

    
    
    const Convexity conv = this->getConvexityOrUnknown();

    ContourIter iter(*fPathRef.get());

    
    SkScalar ymax = this->getBounds().fTop;
    SkScalar ymaxCross = 0;

    for (; !iter.done(); iter.next()) {
        int n = iter.count();
        if (n < 3) {
            continue;
        }

        const SkPoint* pts = iter.pts();
        SkScalar cross = 0;
        if (kConvex_Convexity == conv) {
            
            
            
            if (convex_dir_test<SkScalar, toScalar>(n, pts, dir)) {
                return true;
            }
            if (convex_dir_test<double, toDouble>(n, pts, dir)) {
                return true;
            } else {
                return false;
            }
        } else {
            int index = find_max_y(pts, n);
            if (pts[index].fY < ymax) {
                continue;
            }

            
            
            if (pts[(index + 1) % n].fY == pts[index].fY) {
                int maxIndex;
                int minIndex = find_min_max_x_at_y(pts, index, n, &maxIndex);
                if (minIndex == maxIndex) {
                    goto TRY_CROSSPROD;
                }
                SkASSERT(pts[minIndex].fY == pts[index].fY);
                SkASSERT(pts[maxIndex].fY == pts[index].fY);
                SkASSERT(pts[minIndex].fX <= pts[maxIndex].fX);
                
                
                cross = minIndex - maxIndex;
            } else {
                TRY_CROSSPROD:
                
                
                
                
                
                

                
                
                int prev = find_diff_pt(pts, index, n, n - 1);
                if (prev == index) {
                    
                    continue;
                }
                int next = find_diff_pt(pts, index, n, 1);
                SkASSERT(next != index);
                cross = cross_prod(pts[prev], pts[index], pts[next]);
                
                
                if (0 == cross) {
                    
                    cross = pts[index].fX - pts[next].fX;
                }
            }

            if (cross) {
                
                ymax = pts[index].fY;
                ymaxCross = cross;
            }
        }
    }

    return ymaxCross ? crossToDir(ymaxCross, dir) : false;
}



static SkScalar eval_cubic_coeff(SkScalar A, SkScalar B, SkScalar C,
                                 SkScalar D, SkScalar t) {
    return SkScalarMulAdd(SkScalarMulAdd(SkScalarMulAdd(A, t, B), t, C), t, D);
}

static SkScalar eval_cubic_pts(SkScalar c0, SkScalar c1, SkScalar c2, SkScalar c3,
                               SkScalar t) {
    SkScalar A = c3 + 3*(c1 - c2) - c0;
    SkScalar B = 3*(c2 - c1 - c1 + c0);
    SkScalar C = 3*(c1 - c0);
    SkScalar D = c0;
    return eval_cubic_coeff(A, B, C, D, t);
}




static bool chopMonoCubicAt(SkScalar c0, SkScalar c1, SkScalar c2, SkScalar c3,
                            SkScalar target, SkScalar* t) {
    
    SkASSERT(c0 < target && target < c3);

    SkScalar D = c0 - target;
    SkScalar A = c3 + 3*(c1 - c2) - c0;
    SkScalar B = 3*(c2 - c1 - c1 + c0);
    SkScalar C = 3*(c1 - c0);

    const SkScalar TOLERANCE = SK_Scalar1 / 4096;
    SkScalar minT = 0;
    SkScalar maxT = SK_Scalar1;
    SkScalar mid;
    int i;
    for (i = 0; i < 16; i++) {
        mid = SkScalarAve(minT, maxT);
        SkScalar delta = eval_cubic_coeff(A, B, C, D, mid);
        if (delta < 0) {
            minT = mid;
            delta = -delta;
        } else {
            maxT = mid;
        }
        if (delta < TOLERANCE) {
            break;
        }
    }
    *t = mid;
    return true;
}

template <size_t N> static void find_minmax(const SkPoint pts[],
                                            SkScalar* minPtr, SkScalar* maxPtr) {
    SkScalar min, max;
    min = max = pts[0].fX;
    for (size_t i = 1; i < N; ++i) {
        min = SkMinScalar(min, pts[i].fX);
        max = SkMaxScalar(max, pts[i].fX);
    }
    *minPtr = min;
    *maxPtr = max;
}

static int winding_mono_cubic(const SkPoint pts[], SkScalar x, SkScalar y) {
    SkPoint storage[4];

    int dir = 1;
    if (pts[0].fY > pts[3].fY) {
        storage[0] = pts[3];
        storage[1] = pts[2];
        storage[2] = pts[1];
        storage[3] = pts[0];
        pts = storage;
        dir = -1;
    }
    if (y < pts[0].fY || y >= pts[3].fY) {
        return 0;
    }

    
    SkScalar min, max;
    find_minmax<4>(pts, &min, &max);
    if (x < min) {
        return 0;
    }
    if (x > max) {
        return dir;
    }

    
    SkScalar t, xt;
    if (chopMonoCubicAt(pts[0].fY, pts[1].fY, pts[2].fY, pts[3].fY, y, &t)) {
        xt = eval_cubic_pts(pts[0].fX, pts[1].fX, pts[2].fX, pts[3].fX, t);
    } else {
        SkScalar mid = SkScalarAve(pts[0].fY, pts[3].fY);
        xt = y < mid ? pts[0].fX : pts[3].fX;
    }
    return xt < x ? dir : 0;
}

static int winding_cubic(const SkPoint pts[], SkScalar x, SkScalar y) {
    SkPoint dst[10];
    int n = SkChopCubicAtYExtrema(pts, dst);
    int w = 0;
    for (int i = 0; i <= n; ++i) {
        w += winding_mono_cubic(&dst[i * 3], x, y);
    }
    return w;
}

static int winding_mono_quad(const SkPoint pts[], SkScalar x, SkScalar y) {
    SkScalar y0 = pts[0].fY;
    SkScalar y2 = pts[2].fY;

    int dir = 1;
    if (y0 > y2) {
        SkTSwap(y0, y2);
        dir = -1;
    }
    if (y < y0 || y >= y2) {
        return 0;
    }

    
#if 0
    if (pts[0].fX > x && pts[1].fX > x && pts[2].fX > x) {
        return 0;
    }
#endif

    SkScalar roots[2];
    int n = SkFindUnitQuadRoots(pts[0].fY - 2 * pts[1].fY + pts[2].fY,
                                2 * (pts[1].fY - pts[0].fY),
                                pts[0].fY - y,
                                roots);
    SkASSERT(n <= 1);
    SkScalar xt;
    if (0 == n) {
        SkScalar mid = SkScalarAve(y0, y2);
        
        
        xt = y < mid ? pts[1 - dir].fX : pts[dir - 1].fX;
    } else {
        SkScalar t = roots[0];
        SkScalar C = pts[0].fX;
        SkScalar A = pts[2].fX - 2 * pts[1].fX + C;
        SkScalar B = 2 * (pts[1].fX - C);
        xt = SkScalarMulAdd(SkScalarMulAdd(A, t, B), t, C);
    }
    return xt < x ? dir : 0;
}

static bool is_mono_quad(SkScalar y0, SkScalar y1, SkScalar y2) {
    
    if (y0 == y1) {
        return true;
    }
    if (y0 < y1) {
        return y1 <= y2;
    } else {
        return y1 >= y2;
    }
}

static int winding_quad(const SkPoint pts[], SkScalar x, SkScalar y) {
    SkPoint dst[5];
    int     n = 0;

    if (!is_mono_quad(pts[0].fY, pts[1].fY, pts[2].fY)) {
        n = SkChopQuadAtYExtrema(pts, dst);
        pts = dst;
    }
    int w = winding_mono_quad(pts, x, y);
    if (n > 0) {
        w += winding_mono_quad(&pts[2], x, y);
    }
    return w;
}

static int winding_line(const SkPoint pts[], SkScalar x, SkScalar y) {
    SkScalar x0 = pts[0].fX;
    SkScalar y0 = pts[0].fY;
    SkScalar x1 = pts[1].fX;
    SkScalar y1 = pts[1].fY;

    SkScalar dy = y1 - y0;

    int dir = 1;
    if (y0 > y1) {
        SkTSwap(y0, y1);
        dir = -1;
    }
    if (y < y0 || y >= y1) {
        return 0;
    }

    SkScalar cross = SkScalarMul(x1 - x0, y - pts[0].fY) -
    SkScalarMul(dy, x - pts[0].fX);

    if (SkScalarSignAsInt(cross) == dir) {
        dir = 0;
    }
    return dir;
}

bool SkPath::contains(SkScalar x, SkScalar y) const {
    bool isInverse = this->isInverseFillType();
    if (this->isEmpty()) {
        return isInverse;
    }

    const SkRect& bounds = this->getBounds();
    if (!bounds.contains(x, y)) {
        return isInverse;
    }

    SkPath::Iter iter(*this, true);
    bool done = false;
    int w = 0;
    do {
        SkPoint pts[4];
        switch (iter.next(pts, false)) {
            case SkPath::kMove_Verb:
            case SkPath::kClose_Verb:
                break;
            case SkPath::kLine_Verb:
                w += winding_line(pts, x, y);
                break;
            case SkPath::kQuad_Verb:
                w += winding_quad(pts, x, y);
                break;
            case SkPath::kCubic_Verb:
                w += winding_cubic(pts, x, y);
                break;
            case SkPath::kDone_Verb:
                done = true;
                break;
        }
    } while (!done);

    switch (this->getFillType()) {
        case SkPath::kEvenOdd_FillType:
        case SkPath::kInverseEvenOdd_FillType:
            w &= 1;
            break;
        default:
            break;
    }
    return SkToBool(w);
}

