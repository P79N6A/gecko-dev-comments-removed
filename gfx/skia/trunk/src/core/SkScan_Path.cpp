






#include "SkScanPriv.h"
#include "SkBlitter.h"
#include "SkEdge.h"
#include "SkEdgeBuilder.h"
#include "SkGeometry.h"
#include "SkPath.h"
#include "SkQuadClipper.h"
#include "SkRasterClip.h"
#include "SkRegion.h"
#include "SkTemplates.h"
#include "SkTSort.h"

#ifdef SK_USE_LEGACY_AA_COVERAGE
    #define SK_USE_STD_SORT_FOR_EDGES
#endif

#define kEDGE_HEAD_Y    SK_MinS32
#define kEDGE_TAIL_Y    SK_MaxS32

#ifdef SK_DEBUG
    static void validate_sort(const SkEdge* edge) {
        int y = kEDGE_HEAD_Y;

        while (edge->fFirstY != SK_MaxS32) {
            edge->validate();
            SkASSERT(y <= edge->fFirstY);

            y = edge->fFirstY;
            edge = edge->fNext;
        }
    }
#else
    #define validate_sort(edge)
#endif

static inline void remove_edge(SkEdge* edge) {
    edge->fPrev->fNext = edge->fNext;
    edge->fNext->fPrev = edge->fPrev;
}

static inline void swap_edges(SkEdge* prev, SkEdge* next) {
    SkASSERT(prev->fNext == next && next->fPrev == prev);

    
    prev->fPrev->fNext = next;
    next->fPrev = prev->fPrev;

    
    prev->fNext = next->fNext;
    next->fNext->fPrev = prev;
    next->fNext = prev;
    prev->fPrev = next;
}

static void backward_insert_edge_based_on_x(SkEdge* edge SkDECLAREPARAM(int, curr_y)) {
    SkFixed x = edge->fX;

    for (;;) {
        SkEdge* prev = edge->fPrev;

        
        
        SkASSERT(prev && prev->fFirstY <= curr_y + 1);

        if (prev->fX <= x) {
            break;
        }
        swap_edges(prev, edge);
    }
}

static void insert_new_edges(SkEdge* newEdge, int curr_y) {
    SkASSERT(newEdge->fFirstY >= curr_y);

    while (newEdge->fFirstY == curr_y) {
        SkEdge* next = newEdge->fNext;
        backward_insert_edge_based_on_x(newEdge  SkPARAM(curr_y));
        newEdge = next;
    }
}

#ifdef SK_DEBUG
static void validate_edges_for_y(const SkEdge* edge, int curr_y) {
    while (edge->fFirstY <= curr_y) {
        SkASSERT(edge->fPrev && edge->fNext);
        SkASSERT(edge->fPrev->fNext == edge);
        SkASSERT(edge->fNext->fPrev == edge);
        SkASSERT(edge->fFirstY <= edge->fLastY);

        SkASSERT(edge->fPrev->fX <= edge->fX);
        edge = edge->fNext;
    }
}
#else
    #define validate_edges_for_y(edge, curr_y)
#endif

#if defined _WIN32 && _MSC_VER >= 1300  
#pragma warning ( push )
#pragma warning ( disable : 4701 )
#endif

typedef void (*PrePostProc)(SkBlitter* blitter, int y, bool isStartOfScanline);
#define PREPOST_START   true
#define PREPOST_END     false

static void walk_edges(SkEdge* prevHead, SkPath::FillType fillType,
                       SkBlitter* blitter, int start_y, int stop_y,
                       PrePostProc proc) {
    validate_sort(prevHead->fNext);

    int curr_y = start_y;
    
    int windingMask = (fillType & 1) ? 1 : -1;

    for (;;) {
        int     w = 0;
        int     left SK_INIT_TO_AVOID_WARNING;
        bool    in_interval = false;
        SkEdge* currE = prevHead->fNext;
        SkFixed prevX = prevHead->fX;

        validate_edges_for_y(currE, curr_y);

        if (proc) {
            proc(blitter, curr_y, PREPOST_START);    
        }

        while (currE->fFirstY <= curr_y) {
            SkASSERT(currE->fLastY >= curr_y);

            int x = SkFixedRoundToInt(currE->fX);
            w += currE->fWinding;
            if ((w & windingMask) == 0) { 
                SkASSERT(in_interval);
                int width = x - left;
                SkASSERT(width >= 0);
                if (width)
                    blitter->blitH(left, curr_y, width);
                in_interval = false;
            } else if (!in_interval) {
                left = x;
                in_interval = true;
            }

            SkEdge* next = currE->fNext;
            SkFixed newX;

            if (currE->fLastY == curr_y) {    
                if (currE->fCurveCount < 0) {
                    if (((SkCubicEdge*)currE)->updateCubic()) {
                        SkASSERT(currE->fFirstY == curr_y + 1);

                        newX = currE->fX;
                        goto NEXT_X;
                    }
                } else if (currE->fCurveCount > 0) {
                    if (((SkQuadraticEdge*)currE)->updateQuadratic()) {
                        newX = currE->fX;
                        goto NEXT_X;
                    }
                }
                remove_edge(currE);
            } else {
                SkASSERT(currE->fLastY > curr_y);
                newX = currE->fX + currE->fDX;
                currE->fX = newX;
            NEXT_X:
                if (newX < prevX) { 
                    backward_insert_edge_based_on_x(currE  SkPARAM(curr_y));
                } else {
                    prevX = newX;
                }
            }
            currE = next;
            SkASSERT(currE);
        }

        if (proc) {
            proc(blitter, curr_y, PREPOST_END);    
        }

        curr_y += 1;
        if (curr_y >= stop_y) {
            break;
        }
        
        insert_new_edges(currE, curr_y);
    }
}


static bool update_edge(SkEdge* edge, int last_y) {
    SkASSERT(edge->fLastY >= last_y);
    if (last_y == edge->fLastY) {
        if (edge->fCurveCount < 0) {
            if (((SkCubicEdge*)edge)->updateCubic()) {
                SkASSERT(edge->fFirstY == last_y + 1);
                return false;
            }
        } else if (edge->fCurveCount > 0) {
            if (((SkQuadraticEdge*)edge)->updateQuadratic()) {
                SkASSERT(edge->fFirstY == last_y + 1);
                return false;
            }
        }
        return true;
    }
    return false;
}

static void walk_convex_edges(SkEdge* prevHead, SkPath::FillType,
                              SkBlitter* blitter, int start_y, int stop_y,
                              PrePostProc proc) {
    validate_sort(prevHead->fNext);

    SkEdge* leftE = prevHead->fNext;
    SkEdge* riteE = leftE->fNext;
    SkEdge* currE = riteE->fNext;

#if 0
    int local_top = leftE->fFirstY;
    SkASSERT(local_top == riteE->fFirstY);
#else
    
    
    int local_top = SkMax32(leftE->fFirstY, riteE->fFirstY);
#endif
    SkASSERT(local_top >= start_y);

    for (;;) {
        SkASSERT(leftE->fFirstY <= stop_y);
        SkASSERT(riteE->fFirstY <= stop_y);

        if (leftE->fX > riteE->fX || (leftE->fX == riteE->fX &&
                                      leftE->fDX > riteE->fDX)) {
            SkTSwap(leftE, riteE);
        }

        int local_bot = SkMin32(leftE->fLastY, riteE->fLastY);
        local_bot = SkMin32(local_bot, stop_y - 1);
        SkASSERT(local_top <= local_bot);

        SkFixed left = leftE->fX;
        SkFixed dLeft = leftE->fDX;
        SkFixed rite = riteE->fX;
        SkFixed dRite = riteE->fDX;
        int count = local_bot - local_top;
        SkASSERT(count >= 0);
        if (0 == (dLeft | dRite)) {
            int L = SkFixedRoundToInt(left);
            int R = SkFixedRoundToInt(rite);
            if (L < R) {
                count += 1;
                blitter->blitRect(L, local_top, R - L, count);
                left += count * dLeft;
                rite += count * dRite;
            }
            local_top = local_bot + 1;
        } else {
            do {
                int L = SkFixedRoundToInt(left);
                int R = SkFixedRoundToInt(rite);
                if (L < R) {
                    blitter->blitH(L, local_top, R - L);
                }
                left += dLeft;
                rite += dRite;
                local_top += 1;
            } while (--count >= 0);
        }

        leftE->fX = left;
        riteE->fX = rite;

        if (update_edge(leftE, local_bot)) {
            if (currE->fFirstY >= stop_y) {
                break;
            }
            leftE = currE;
            currE = currE->fNext;
        }
        if (update_edge(riteE, local_bot)) {
            if (currE->fFirstY >= stop_y) {
                break;
            }
            riteE = currE;
            currE = currE->fNext;
        }

        SkASSERT(leftE);
        SkASSERT(riteE);

        
        SkASSERT(local_top == local_bot + 1);
        if (local_top >= stop_y) {
            break;
        }
    }
}








class InverseBlitter : public SkBlitter {
public:
    void setBlitter(SkBlitter* blitter, const SkIRect& clip, int shift) {
        fBlitter = blitter;
        fFirstX = clip.fLeft << shift;
        fLastX = clip.fRight << shift;
    }
    void prepost(int y, bool isStart) {
        if (isStart) {
            fPrevX = fFirstX;
        } else {
            int invWidth = fLastX - fPrevX;
            if (invWidth > 0) {
                fBlitter->blitH(fPrevX, y, invWidth);
            }
        }
    }

    
    virtual void blitH(int x, int y, int width) {
        int invWidth = x - fPrevX;
        if (invWidth > 0) {
            fBlitter->blitH(fPrevX, y, invWidth);
        }
        fPrevX = x + width;
    }

    
    virtual void blitAntiH(int, int, const SkAlpha[], const int16_t runs[]) {
        SkDEBUGFAIL("blitAntiH unexpected");
    }
    virtual void blitV(int x, int y, int height, SkAlpha alpha) {
        SkDEBUGFAIL("blitV unexpected");
    }
    virtual void blitRect(int x, int y, int width, int height) {
        SkDEBUGFAIL("blitRect unexpected");
    }
    virtual void blitMask(const SkMask&, const SkIRect& clip) {
        SkDEBUGFAIL("blitMask unexpected");
    }
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) {
        SkDEBUGFAIL("justAnOpaqueColor unexpected");
        return NULL;
    }

private:
    SkBlitter*  fBlitter;
    int         fFirstX, fLastX, fPrevX;
};

static void PrePostInverseBlitterProc(SkBlitter* blitter, int y, bool isStart) {
    ((InverseBlitter*)blitter)->prepost(y, isStart);
}



#if defined _WIN32 && _MSC_VER >= 1300
#pragma warning ( pop )
#endif

#ifdef SK_USE_STD_SORT_FOR_EDGES
extern "C" {
    static int edge_compare(const void* a, const void* b) {
        const SkEdge* edgea = *(const SkEdge**)a;
        const SkEdge* edgeb = *(const SkEdge**)b;

        int valuea = edgea->fFirstY;
        int valueb = edgeb->fFirstY;

        if (valuea == valueb) {
            valuea = edgea->fX;
            valueb = edgeb->fX;
        }

        
        
        
        return (valuea < valueb) ? -1 : (valuea > valueb);
    }
}
#else
static bool operator<(const SkEdge& a, const SkEdge& b) {
    int valuea = a.fFirstY;
    int valueb = b.fFirstY;

    if (valuea == valueb) {
        valuea = a.fX;
        valueb = b.fX;
    }

    return valuea < valueb;
}
#endif

static SkEdge* sort_edges(SkEdge* list[], int count, SkEdge** last) {
#ifdef SK_USE_STD_SORT_FOR_EDGES
    qsort(list, count, sizeof(SkEdge*), edge_compare);
#else
    SkTQSort(list, list + count - 1);
#endif

    
    for (int i = 1; i < count; i++) {
        list[i - 1]->fNext = list[i];
        list[i]->fPrev = list[i - 1];
    }

    *last = list[count - 1];
    return list[0];
}






void sk_fill_path(const SkPath& path, const SkIRect* clipRect, SkBlitter* blitter,
                  int start_y, int stop_y, int shiftEdgesUp,
                  const SkRegion& clipRgn) {
    SkASSERT(blitter);

    SkEdgeBuilder   builder;

    int count = builder.build(path, clipRect, shiftEdgesUp);
    SkEdge**    list = builder.edgeList();

    if (count < 2) {
        if (path.isInverseFillType()) {
            





            SkIRect rect = clipRgn.getBounds();
            if (rect.fTop < start_y) {
                rect.fTop = start_y;
            }
            if (rect.fBottom > stop_y) {
                rect.fBottom = stop_y;
            }
            if (!rect.isEmpty()) {
                blitter->blitRect(rect.fLeft << shiftEdgesUp,
                                  rect.fTop << shiftEdgesUp,
                                  rect.width() << shiftEdgesUp,
                                  rect.height() << shiftEdgesUp);
            }
        }

        return;
    }

    SkEdge headEdge, tailEdge, *last;
    
    SkEdge* edge = sort_edges(list, count, &last);

    headEdge.fPrev = NULL;
    headEdge.fNext = edge;
    headEdge.fFirstY = kEDGE_HEAD_Y;
    headEdge.fX = SK_MinS32;
    edge->fPrev = &headEdge;

    tailEdge.fPrev = last;
    tailEdge.fNext = NULL;
    tailEdge.fFirstY = kEDGE_TAIL_Y;
    last->fNext = &tailEdge;

    

    start_y <<= shiftEdgesUp;
    stop_y <<= shiftEdgesUp;
    if (clipRect && start_y < clipRect->fTop) {
        start_y = clipRect->fTop;
    }
    if (clipRect && stop_y > clipRect->fBottom) {
        stop_y = clipRect->fBottom;
    }

    InverseBlitter  ib;
    PrePostProc     proc = NULL;

    if (path.isInverseFillType()) {
        ib.setBlitter(blitter, clipRgn.getBounds(), shiftEdgesUp);
        blitter = &ib;
        proc = PrePostInverseBlitterProc;
    }

    if (path.isConvex() && (NULL == proc)) {
        walk_convex_edges(&headEdge, path.getFillType(), blitter, start_y, stop_y, NULL);
    } else {
        walk_edges(&headEdge, path.getFillType(), blitter, start_y, stop_y, proc);
    }
}

void sk_blit_above(SkBlitter* blitter, const SkIRect& ir, const SkRegion& clip) {
    const SkIRect& cr = clip.getBounds();
    SkIRect tmp;

    tmp.fLeft = cr.fLeft;
    tmp.fRight = cr.fRight;
    tmp.fTop = cr.fTop;
    tmp.fBottom = ir.fTop;
    if (!tmp.isEmpty()) {
        blitter->blitRectRegion(tmp, clip);
    }
}

void sk_blit_below(SkBlitter* blitter, const SkIRect& ir, const SkRegion& clip) {
    const SkIRect& cr = clip.getBounds();
    SkIRect tmp;

    tmp.fLeft = cr.fLeft;
    tmp.fRight = cr.fRight;
    tmp.fTop = ir.fBottom;
    tmp.fBottom = cr.fBottom;
    if (!tmp.isEmpty()) {
        blitter->blitRectRegion(tmp, clip);
    }
}








SkScanClipper::SkScanClipper(SkBlitter* blitter, const SkRegion* clip,
                             const SkIRect& ir, bool skipRejectTest) {
    fBlitter = NULL;     
    fClipRect = NULL;

    if (clip) {
        fClipRect = &clip->getBounds();
        if (!skipRejectTest && !SkIRect::Intersects(*fClipRect, ir)) { 
            return;
        }

        if (clip->isRect()) {
            if (fClipRect->contains(ir)) {
                fClipRect = NULL;
            } else {
                
                if (fClipRect->fLeft > ir.fLeft || fClipRect->fRight < ir.fRight) {
                    fRectBlitter.init(blitter, *fClipRect);
                    blitter = &fRectBlitter;
                }
            }
        } else {
            fRgnBlitter.init(blitter, clip);
            blitter = &fRgnBlitter;
        }
    }
    fBlitter = blitter;
}



static bool clip_to_limit(const SkRegion& orig, SkRegion* reduced) {
    const int32_t limit = 32767;

    SkIRect limitR;
    limitR.set(-limit, -limit, limit, limit);
    if (limitR.contains(orig.getBounds())) {
        return false;
    }
    reduced->op(orig, limitR, SkRegion::kIntersect_Op);
    return true;
}

void SkScan::FillPath(const SkPath& path, const SkRegion& origClip,
                      SkBlitter* blitter) {
    if (origClip.isEmpty()) {
        return;
    }

    
    
    const SkRegion* clipPtr = &origClip;
    SkRegion finiteClip;
    if (clip_to_limit(origClip, &finiteClip)) {
        if (finiteClip.isEmpty()) {
            return;
        }
        clipPtr = &finiteClip;
    }
        

    SkIRect ir;
    
    
    
    
    path.getBounds().dround(&ir);
    if (ir.isEmpty()) {
        if (path.isInverseFillType()) {
            blitter->blitRegion(*clipPtr);
        }
        return;
    }

    SkScanClipper clipper(blitter, clipPtr, ir, path.isInverseFillType());

    blitter = clipper.getBlitter();
    if (blitter) {
        
        
        if (path.isInverseFillType()) {
            sk_blit_above(blitter, ir, *clipPtr);
        }
        sk_fill_path(path, clipper.getClipRect(), blitter, ir.fTop, ir.fBottom,
                     0, *clipPtr);
        if (path.isInverseFillType()) {
            sk_blit_below(blitter, ir, *clipPtr);
        }
    } else {
        
    }
}

void SkScan::FillPath(const SkPath& path, const SkIRect& ir,
                      SkBlitter* blitter) {
    SkRegion rgn(ir);
    FillPath(path, rgn, blitter);
}



static int build_tri_edges(SkEdge edge[], const SkPoint pts[],
                           const SkIRect* clipRect, SkEdge* list[]) {
    SkEdge** start = list;

    if (edge->setLine(pts[0], pts[1], clipRect, 0)) {
        *list++ = edge;
        edge = (SkEdge*)((char*)edge + sizeof(SkEdge));
    }
    if (edge->setLine(pts[1], pts[2], clipRect, 0)) {
        *list++ = edge;
        edge = (SkEdge*)((char*)edge + sizeof(SkEdge));
    }
    if (edge->setLine(pts[2], pts[0], clipRect, 0)) {
        *list++ = edge;
    }
    return (int)(list - start);
}


static void sk_fill_triangle(const SkPoint pts[], const SkIRect* clipRect,
                             SkBlitter* blitter, const SkIRect& ir) {
    SkASSERT(pts && blitter);

    SkEdge edgeStorage[3];
    SkEdge* list[3];

    int count = build_tri_edges(edgeStorage, pts, clipRect, list);
    if (count < 2) {
        return;
    }

    SkEdge headEdge, tailEdge, *last;

    
    SkEdge* edge = sort_edges(list, count, &last);

    headEdge.fPrev = NULL;
    headEdge.fNext = edge;
    headEdge.fFirstY = kEDGE_HEAD_Y;
    headEdge.fX = SK_MinS32;
    edge->fPrev = &headEdge;

    tailEdge.fPrev = last;
    tailEdge.fNext = NULL;
    tailEdge.fFirstY = kEDGE_TAIL_Y;
    last->fNext = &tailEdge;

    
    int stop_y = ir.fBottom;
    if (clipRect && stop_y > clipRect->fBottom) {
        stop_y = clipRect->fBottom;
    }
    int start_y = ir.fTop;
    if (clipRect && start_y < clipRect->fTop) {
        start_y = clipRect->fTop;
    }
    walk_convex_edges(&headEdge, SkPath::kEvenOdd_FillType, blitter, start_y, stop_y, NULL);

}

void SkScan::FillTriangle(const SkPoint pts[], const SkRasterClip& clip,
                          SkBlitter* blitter) {
    if (clip.isEmpty()) {
        return;
    }

    SkRect  r;
    SkIRect ir;
    r.set(pts, 3);
    r.round(&ir);
    if (ir.isEmpty() || !SkIRect::Intersects(ir, clip.getBounds())) {
        return;
    }

    SkAAClipBlitterWrapper wrap;
    const SkRegion* clipRgn;
    if (clip.isBW()) {
        clipRgn = &clip.bwRgn();
    } else {
        wrap.init(clip, blitter);
        clipRgn = &wrap.getRgn();
        blitter = wrap.getBlitter();
    }

    SkScanClipper clipper(blitter, clipRgn, ir);
    blitter = clipper.getBlitter();
    if (NULL != blitter) {
        sk_fill_triangle(pts, clipper.getClipRect(), blitter, ir);
    }
}
