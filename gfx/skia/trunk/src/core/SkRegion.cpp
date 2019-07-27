








#include "SkRegionPriv.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkUtils.h"











SkDEBUGCODE(int32_t gRgnAllocCounter;)







static SkRegion::RunType* skip_intervals(const SkRegion::RunType runs[]) {
    int intervals = runs[-1];
#ifdef SK_DEBUG
    if (intervals > 0) {
        SkASSERT(runs[0] < runs[1]);
        SkASSERT(runs[1] < SkRegion::kRunTypeSentinel);
    } else {
        SkASSERT(0 == intervals);
        SkASSERT(SkRegion::kRunTypeSentinel == runs[0]);
    }
#endif
    runs += intervals * 2 + 1;
    return const_cast<SkRegion::RunType*>(runs);
}

bool SkRegion::RunsAreARect(const SkRegion::RunType runs[], int count,
                            SkIRect* bounds) {
    assert_sentinel(runs[0], false);    
    SkASSERT(count >= kRectRegionRuns);

    if (count == kRectRegionRuns) {
        assert_sentinel(runs[1], false);    
        SkASSERT(1 == runs[2]);
        assert_sentinel(runs[3], false);    
        assert_sentinel(runs[4], false);    
        assert_sentinel(runs[5], true);
        assert_sentinel(runs[6], true);

        SkASSERT(runs[0] < runs[1]);    
        SkASSERT(runs[3] < runs[4]);    

        bounds->set(runs[3], runs[0], runs[4], runs[1]);
        return true;
    }
    return false;
}



SkRegion::SkRegion() {
    fBounds.set(0, 0, 0, 0);
    fRunHead = SkRegion_gEmptyRunHeadPtr;
}

SkRegion::SkRegion(const SkRegion& src) {
    fRunHead = SkRegion_gEmptyRunHeadPtr;   
    this->setRegion(src);
}

SkRegion::SkRegion(const SkIRect& rect) {
    fRunHead = SkRegion_gEmptyRunHeadPtr;   
    this->setRect(rect);
}

SkRegion::~SkRegion() {
    this->freeRuns();
}

void SkRegion::freeRuns() {
    if (this->isComplex()) {
        SkASSERT(fRunHead->fRefCnt >= 1);
        if (sk_atomic_dec(&fRunHead->fRefCnt) == 1) {
            
            
            
            sk_free(fRunHead);
        }
    }
}

void SkRegion::allocateRuns(int count, int ySpanCount, int intervalCount) {
    fRunHead = RunHead::Alloc(count, ySpanCount, intervalCount);
}

void SkRegion::allocateRuns(int count) {
    fRunHead = RunHead::Alloc(count);
}

void SkRegion::allocateRuns(const RunHead& head) {
    fRunHead = RunHead::Alloc(head.fRunCount,
                              head.getYSpanCount(),
                              head.getIntervalCount());
}

SkRegion& SkRegion::operator=(const SkRegion& src) {
    (void)this->setRegion(src);
    return *this;
}

void SkRegion::swap(SkRegion& other) {
    SkTSwap<SkIRect>(fBounds, other.fBounds);
    SkTSwap<RunHead*>(fRunHead, other.fRunHead);
}

int SkRegion::computeRegionComplexity() const {
  if (this->isEmpty()) {
    return 0;
  } else if (this->isRect()) {
    return 1;
  }
  return fRunHead->getIntervalCount();
}

bool SkRegion::setEmpty() {
    this->freeRuns();
    fBounds.set(0, 0, 0, 0);
    fRunHead = SkRegion_gEmptyRunHeadPtr;
    return false;
}

bool SkRegion::setRect(int32_t left, int32_t top,
                       int32_t right, int32_t bottom) {
    if (left >= right || top >= bottom) {
        return this->setEmpty();
    }
    this->freeRuns();
    fBounds.set(left, top, right, bottom);
    fRunHead = SkRegion_gRectRunHeadPtr;
    return true;
}

bool SkRegion::setRect(const SkIRect& r) {
    return this->setRect(r.fLeft, r.fTop, r.fRight, r.fBottom);
}

bool SkRegion::setRegion(const SkRegion& src) {
    if (this != &src) {
        this->freeRuns();

        fBounds = src.fBounds;
        fRunHead = src.fRunHead;
        if (this->isComplex()) {
            sk_atomic_inc(&fRunHead->fRefCnt);
        }
    }
    return fRunHead != SkRegion_gEmptyRunHeadPtr;
}

bool SkRegion::op(const SkIRect& rect, const SkRegion& rgn, Op op) {
    SkRegion tmp(rect);

    return this->op(tmp, rgn, op);
}

bool SkRegion::op(const SkRegion& rgn, const SkIRect& rect, Op op) {
    SkRegion tmp(rect);

    return this->op(rgn, tmp, op);
}



#ifdef SK_BUILD_FOR_ANDROID
#include <stdio.h>
char* SkRegion::toString() {
    Iterator iter(*this);
    int count = 0;
    while (!iter.done()) {
        count++;
        iter.next();
    }
    
    const int max = (count*((11*4)+5))+11+1;
    char* result = (char*)malloc(max);
    if (result == NULL) {
        return NULL;
    }
    count = sprintf(result, "SkRegion(");
    iter.reset(*this);
    while (!iter.done()) {
        const SkIRect& r = iter.rect();
        count += sprintf(result+count, "(%d,%d,%d,%d)", r.fLeft, r.fTop, r.fRight, r.fBottom);
        iter.next();
    }
    count += sprintf(result+count, ")");
    return result;
}
#endif



int SkRegion::count_runtype_values(int* itop, int* ibot) const {
    int maxT;

    if (this->isRect()) {
        maxT = 2;
    } else {
        SkASSERT(this->isComplex());
        maxT = fRunHead->getIntervalCount() * 2;
    }
    *itop = fBounds.fTop;
    *ibot = fBounds.fBottom;
    return maxT;
}

static bool isRunCountEmpty(int count) {
    return count <= 2;
}

bool SkRegion::setRuns(RunType runs[], int count) {
    SkDEBUGCODE(this->validate();)
    SkASSERT(count > 0);

    if (isRunCountEmpty(count)) {
    
        assert_sentinel(runs[count-1], true);
        return this->setEmpty();
    }

    
    
    if (count > kRectRegionRuns) {
        RunType* stop = runs + count;
        assert_sentinel(runs[0], false);    
        assert_sentinel(runs[1], false);    
        

        if (runs[3] == SkRegion::kRunTypeSentinel) {  
            runs += 3;  
            runs[0] = runs[-2]; 
            assert_sentinel(runs[1], false);    
            assert_sentinel(runs[2], false);    
            assert_sentinel(runs[3], false);    
            assert_sentinel(runs[4], false);    
        }

        assert_sentinel(stop[-1], true);
        assert_sentinel(stop[-2], true);

        
        if (stop[-5] == SkRegion::kRunTypeSentinel) { 
            stop[-4] = SkRegion::kRunTypeSentinel;    
            stop -= 3;
            assert_sentinel(stop[-1], true);    
            assert_sentinel(stop[-2], true);    
            assert_sentinel(stop[-3], false);   
            assert_sentinel(stop[-4], false);   
            assert_sentinel(stop[-5], false);   
            assert_sentinel(stop[-6], false);   
        }
        count = (int)(stop - runs);
    }

    SkASSERT(count >= kRectRegionRuns);

    if (SkRegion::RunsAreARect(runs, count, &fBounds)) {
        return this->setRect(fBounds);
    }

    

    if (!this->isComplex() || fRunHead->fRunCount != count) {
        this->freeRuns();
        this->allocateRuns(count);
    }

    
    
    fRunHead = fRunHead->ensureWritable();
    memcpy(fRunHead->writable_runs(), runs, count * sizeof(RunType));
    fRunHead->computeRunBounds(&fBounds);

    SkDEBUGCODE(this->validate();)

    return true;
}

void SkRegion::BuildRectRuns(const SkIRect& bounds,
                             RunType runs[kRectRegionRuns]) {
    runs[0] = bounds.fTop;
    runs[1] = bounds.fBottom;
    runs[2] = 1;    
    runs[3] = bounds.fLeft;
    runs[4] = bounds.fRight;
    runs[5] = kRunTypeSentinel;
    runs[6] = kRunTypeSentinel;
}

bool SkRegion::contains(int32_t x, int32_t y) const {
    SkDEBUGCODE(this->validate();)

    if (!fBounds.contains(x, y)) {
        return false;
    }
    if (this->isRect()) {
        return true;
    }
    SkASSERT(this->isComplex());

    const RunType* runs = fRunHead->findScanline(y);

    
    runs += 2;

    
    
    
    
    
    
    for (;;) {
        if (x < runs[0]) {
            break;
        }
        if (x < runs[1]) {
            return true;
        }
        runs += 2;
    }
    return false;
}

static SkRegion::RunType scanline_bottom(const SkRegion::RunType runs[]) {
    return runs[0];
}

static const SkRegion::RunType* scanline_next(const SkRegion::RunType runs[]) {
    
    return runs + 2 + runs[1] * 2 + 1;
}

static bool scanline_contains(const SkRegion::RunType runs[],
                              SkRegion::RunType L, SkRegion::RunType R) {
    runs += 2;  
    for (;;) {
        if (L < runs[0]) {
            break;
        }
        if (R <= runs[1]) {
            return true;
        }
        runs += 2;
    }
    return false;
}

bool SkRegion::contains(const SkIRect& r) const {
    SkDEBUGCODE(this->validate();)

    if (!fBounds.contains(r)) {
        return false;
    }
    if (this->isRect()) {
        return true;
    }
    SkASSERT(this->isComplex());

    const RunType* scanline = fRunHead->findScanline(r.fTop);
    for (;;) {
        if (!scanline_contains(scanline, r.fLeft, r.fRight)) {
            return false;
        }
        if (r.fBottom <= scanline_bottom(scanline)) {
            break;
        }
        scanline = scanline_next(scanline);
    }
    return true;
}

bool SkRegion::contains(const SkRegion& rgn) const {
    SkDEBUGCODE(this->validate();)
    SkDEBUGCODE(rgn.validate();)

    if (this->isEmpty() || rgn.isEmpty() || !fBounds.contains(rgn.fBounds)) {
        return false;
    }
    if (this->isRect()) {
        return true;
    }
    if (rgn.isRect()) {
        return this->contains(rgn.getBounds());
    }

    



    return !Oper(rgn, *this, kDifference_Op, NULL);
}

const SkRegion::RunType* SkRegion::getRuns(RunType tmpStorage[],
                                           int* intervals) const {
    SkASSERT(tmpStorage && intervals);
    const RunType* runs = tmpStorage;

    if (this->isEmpty()) {
        tmpStorage[0] = kRunTypeSentinel;
        *intervals = 0;
    } else if (this->isRect()) {
        BuildRectRuns(fBounds, tmpStorage);
        *intervals = 1;
    } else {
        runs = fRunHead->readonly_runs();
        *intervals = fRunHead->getIntervalCount();
    }
    return runs;
}



static bool scanline_intersects(const SkRegion::RunType runs[],
                                SkRegion::RunType L, SkRegion::RunType R) {
    runs += 2;  
    for (;;) {
        if (R <= runs[0]) {
            break;
        }
        if (L < runs[1]) {
            return true;
        }
        runs += 2;
    }
    return false;
}

bool SkRegion::intersects(const SkIRect& r) const {
    SkDEBUGCODE(this->validate();)

    if (this->isEmpty() || r.isEmpty()) {
        return false;
    }

    SkIRect sect;
    if (!sect.intersect(fBounds, r)) {
        return false;
    }
    if (this->isRect()) {
        return true;
    }
    SkASSERT(this->isComplex());

    const RunType* scanline = fRunHead->findScanline(sect.fTop);
    for (;;) {
        if (scanline_intersects(scanline, sect.fLeft, sect.fRight)) {
            return true;
        }
        if (sect.fBottom <= scanline_bottom(scanline)) {
            break;
        }
        scanline = scanline_next(scanline);
    }
    return false;
}

bool SkRegion::intersects(const SkRegion& rgn) const {
    if (this->isEmpty() || rgn.isEmpty()) {
        return false;
    }

    if (!SkIRect::Intersects(fBounds, rgn.fBounds)) {
        return false;
    }

    bool weAreARect = this->isRect();
    bool theyAreARect = rgn.isRect();

    if (weAreARect && theyAreARect) {
        return true;
    }
    if (weAreARect) {
        return rgn.intersects(this->getBounds());
    }
    if (theyAreARect) {
        return this->intersects(rgn.getBounds());
    }

    
    return Oper(*this, rgn, kIntersect_Op, NULL);
}



bool SkRegion::operator==(const SkRegion& b) const {
    SkDEBUGCODE(validate();)
    SkDEBUGCODE(b.validate();)

    if (this == &b) {
        return true;
    }
    if (fBounds != b.fBounds) {
        return false;
    }

    const SkRegion::RunHead* ah = fRunHead;
    const SkRegion::RunHead* bh = b.fRunHead;

    
    if (ah == bh) {
        return true;
    }
    
    if (!this->isComplex() || !b.isComplex()) {
        return false;
    }
    return  ah->fRunCount == bh->fRunCount &&
            !memcmp(ah->readonly_runs(), bh->readonly_runs(),
                    ah->fRunCount * sizeof(SkRegion::RunType));
}

void SkRegion::translate(int dx, int dy, SkRegion* dst) const {
    SkDEBUGCODE(this->validate();)

    if (NULL == dst) {
        return;
    }
    if (this->isEmpty()) {
        dst->setEmpty();
    } else if (this->isRect()) {
        dst->setRect(fBounds.fLeft + dx, fBounds.fTop + dy,
                     fBounds.fRight + dx, fBounds.fBottom + dy);
    } else {
        if (this == dst) {
            dst->fRunHead = dst->fRunHead->ensureWritable();
        } else {
            SkRegion    tmp;
            tmp.allocateRuns(*fRunHead);
            tmp.fBounds = fBounds;
            dst->swap(tmp);
        }

        dst->fBounds.offset(dx, dy);

        const RunType*  sruns = fRunHead->readonly_runs();
        RunType*        druns = dst->fRunHead->writable_runs();

        *druns++ = (SkRegion::RunType)(*sruns++ + dy);    
        for (;;) {
            int bottom = *sruns++;
            if (bottom == kRunTypeSentinel) {
                break;
            }
            *druns++ = (SkRegion::RunType)(bottom + dy);  
            *druns++ = *sruns++;    
            for (;;) {
                int x = *sruns++;
                if (x == kRunTypeSentinel) {
                    break;
                }
                *druns++ = (SkRegion::RunType)(x + dx);
                *druns++ = (SkRegion::RunType)(*sruns++ + dx);
            }
            *druns++ = kRunTypeSentinel;    
        }
        *druns++ = kRunTypeSentinel;    

        SkASSERT(sruns - fRunHead->readonly_runs() == fRunHead->fRunCount);
        SkASSERT(druns - dst->fRunHead->readonly_runs() == dst->fRunHead->fRunCount);
    }

    SkDEBUGCODE(this->validate();)
}



bool SkRegion::setRects(const SkIRect rects[], int count) {
    if (0 == count) {
        this->setEmpty();
    } else {
        this->setRect(rects[0]);
        for (int i = 1; i < count; i++) {
            this->op(rects[i], kUnion_Op);
        }
    }
    return !this->isEmpty();
}



#if defined _WIN32 && _MSC_VER >= 1300  
#pragma warning ( push )
#pragma warning ( disable : 4701 )
#endif

#ifdef SK_DEBUG
static void assert_valid_pair(int left, int rite)
{
    SkASSERT(left == SkRegion::kRunTypeSentinel || left < rite);
}
#else
    #define assert_valid_pair(left, rite)
#endif

struct spanRec {
    const SkRegion::RunType*    fA_runs;
    const SkRegion::RunType*    fB_runs;
    int                         fA_left, fA_rite, fB_left, fB_rite;
    int                         fLeft, fRite, fInside;

    void init(const SkRegion::RunType a_runs[],
              const SkRegion::RunType b_runs[]) {
        fA_left = *a_runs++;
        fA_rite = *a_runs++;
        fB_left = *b_runs++;
        fB_rite = *b_runs++;

        fA_runs = a_runs;
        fB_runs = b_runs;
    }

    bool done() const {
        SkASSERT(fA_left <= SkRegion::kRunTypeSentinel);
        SkASSERT(fB_left <= SkRegion::kRunTypeSentinel);
        return fA_left == SkRegion::kRunTypeSentinel &&
               fB_left == SkRegion::kRunTypeSentinel;
    }

    void next() {
        assert_valid_pair(fA_left, fA_rite);
        assert_valid_pair(fB_left, fB_rite);

        int     inside, left, rite SK_INIT_TO_AVOID_WARNING;
        bool    a_flush = false;
        bool    b_flush = false;

        int a_left = fA_left;
        int a_rite = fA_rite;
        int b_left = fB_left;
        int b_rite = fB_rite;

        if (a_left < b_left) {
            inside = 1;
            left = a_left;
            if (a_rite <= b_left) {   
                rite = a_rite;
                a_flush = true;
            } else { 
                rite = a_left = b_left;
            }
        } else if (b_left < a_left) {
            inside = 2;
            left = b_left;
            if (b_rite <= a_left) {   
                rite = b_rite;
                b_flush = true;
            } else {    
                rite = b_left = a_left;
            }
        } else {    
            inside = 3;
            left = a_left;  
            if (a_rite <= b_rite) {
                rite = b_left = a_rite;
                a_flush = true;
            }
            if (b_rite <= a_rite) {
                rite = a_left = b_rite;
                b_flush = true;
            }
        }

        if (a_flush) {
            a_left = *fA_runs++;
            a_rite = *fA_runs++;
        }
        if (b_flush) {
            b_left = *fB_runs++;
            b_rite = *fB_runs++;
        }

        SkASSERT(left <= rite);

        
        fA_left = a_left;
        fA_rite = a_rite;
        fB_left = b_left;
        fB_rite = b_rite;

        fLeft = left;
        fRite = rite;
        fInside = inside;
    }
};

static SkRegion::RunType* operate_on_span(const SkRegion::RunType a_runs[],
                                          const SkRegion::RunType b_runs[],
                                          SkRegion::RunType dst[],
                                          int min, int max) {
    spanRec rec;
    bool    firstInterval = true;

    rec.init(a_runs, b_runs);

    while (!rec.done()) {
        rec.next();

        int left = rec.fLeft;
        int rite = rec.fRite;

        
        if ((unsigned)(rec.fInside - min) <= (unsigned)(max - min) &&
                left < rite) {    
            if (firstInterval || dst[-1] < left) {
                *dst++ = (SkRegion::RunType)(left);
                *dst++ = (SkRegion::RunType)(rite);
                firstInterval = false;
            } else {
                
                dst[-1] = (SkRegion::RunType)(rite);
            }
        }
    }

    *dst++ = SkRegion::kRunTypeSentinel;
    return dst;
}

#if defined _WIN32 && _MSC_VER >= 1300
#pragma warning ( pop )
#endif

static const struct {
    uint8_t fMin;
    uint8_t fMax;
} gOpMinMax[] = {
    { 1, 1 },   
    { 3, 3 },   
    { 1, 3 },   
    { 1, 2 }    
};

class RgnOper {
public:
    RgnOper(int top, SkRegion::RunType dst[], SkRegion::Op op) {
        
        SkASSERT(SkRegion::kDifference_Op == 0);
        SkASSERT(SkRegion::kIntersect_Op == 1);
        SkASSERT(SkRegion::kUnion_Op == 2);
        SkASSERT(SkRegion::kXOR_Op == 3);
        SkASSERT((unsigned)op <= 3);

        fStartDst = dst;
        fPrevDst = dst + 1;
        fPrevLen = 0;       
        fTop = (SkRegion::RunType)(top);    

        fMin = gOpMinMax[op].fMin;
        fMax = gOpMinMax[op].fMax;
    }

    void addSpan(int bottom, const SkRegion::RunType a_runs[],
                 const SkRegion::RunType b_runs[]) {
        
        SkRegion::RunType*  start = fPrevDst + fPrevLen + 2;
        
        SkRegion::RunType*  stop = operate_on_span(a_runs, b_runs, start, fMin, fMax);
        size_t              len = stop - start;
        SkASSERT(len >= 1 && (len & 1) == 1);
        SkASSERT(SkRegion::kRunTypeSentinel == stop[-1]);

        if (fPrevLen == len &&
            (1 == len || !memcmp(fPrevDst, start,
                                 (len - 1) * sizeof(SkRegion::RunType)))) {
            
            fPrevDst[-2] = (SkRegion::RunType)(bottom);
        } else {    
            if (len == 1 && fPrevLen == 0) {
                fTop = (SkRegion::RunType)(bottom); 
            } else {
                start[-2] = (SkRegion::RunType)(bottom);
                start[-1] = SkToS32(len >> 1);
                fPrevDst = start;
                fPrevLen = len;
            }
        }
    }

    int flush() {
        fStartDst[0] = fTop;
        fPrevDst[fPrevLen] = SkRegion::kRunTypeSentinel;
        return (int)(fPrevDst - fStartDst + fPrevLen + 1);
    }

    bool isEmpty() const { return 0 == fPrevLen; }

    uint8_t fMin, fMax;

private:
    SkRegion::RunType*  fStartDst;
    SkRegion::RunType*  fPrevDst;
    size_t              fPrevLen;
    SkRegion::RunType   fTop;
};


#define QUICK_EXIT_TRUE_COUNT   (-1)

static int operate(const SkRegion::RunType a_runs[],
                   const SkRegion::RunType b_runs[],
                   SkRegion::RunType dst[],
                   SkRegion::Op op,
                   bool quickExit) {
    const SkRegion::RunType gEmptyScanline[] = {
        0,  
        0,  
        SkRegion::kRunTypeSentinel,
        
        
        
        
        
        0
    };
    const SkRegion::RunType* const gSentinel = &gEmptyScanline[2];

    int a_top = *a_runs++;
    int a_bot = *a_runs++;
    int b_top = *b_runs++;
    int b_bot = *b_runs++;

    a_runs += 1;    
    b_runs += 1;    

    

    assert_sentinel(a_top, false);
    assert_sentinel(a_bot, false);
    assert_sentinel(b_top, false);
    assert_sentinel(b_bot, false);

    RgnOper oper(SkMin32(a_top, b_top), dst, op);

    int prevBot = SkRegion::kRunTypeSentinel; 

    while (a_bot < SkRegion::kRunTypeSentinel ||
           b_bot < SkRegion::kRunTypeSentinel) {
        int                         top, bot SK_INIT_TO_AVOID_WARNING;
        const SkRegion::RunType*    run0 = gSentinel;
        const SkRegion::RunType*    run1 = gSentinel;
        bool                        a_flush = false;
        bool                        b_flush = false;

        if (a_top < b_top) {
            top = a_top;
            run0 = a_runs;
            if (a_bot <= b_top) {   
                bot = a_bot;
                a_flush = true;
            } else {  
                bot = a_top = b_top;
            }
        } else if (b_top < a_top) {
            top = b_top;
            run1 = b_runs;
            if (b_bot <= a_top) {   
                bot = b_bot;
                b_flush = true;
            } else {    
                bot = b_top = a_top;
            }
        } else {    
            top = a_top;    
            run0 = a_runs;
            run1 = b_runs;
            if (a_bot <= b_bot) {
                bot = b_top = a_bot;
                a_flush = true;
            }
            if (b_bot <= a_bot) {
                bot = a_top = b_bot;
                b_flush = true;
            }
        }

        if (top > prevBot) {
            oper.addSpan(top, gSentinel, gSentinel);
        }
        oper.addSpan(bot, run0, run1);

        if (quickExit && !oper.isEmpty()) {
            return QUICK_EXIT_TRUE_COUNT;
        }

        if (a_flush) {
            a_runs = skip_intervals(a_runs);
            a_top = a_bot;
            a_bot = *a_runs++;
            a_runs += 1;    
            if (a_bot == SkRegion::kRunTypeSentinel) {
                a_top = a_bot;
            }
        }
        if (b_flush) {
            b_runs = skip_intervals(b_runs);
            b_top = b_bot;
            b_bot = *b_runs++;
            b_runs += 1;    
            if (b_bot == SkRegion::kRunTypeSentinel) {
                b_top = b_bot;
            }
        }

        prevBot = bot;
    }
    return oper.flush();
}











#if 0 
static int count_to_intervals(int count) {
    SkASSERT(count >= 6);   
    return (count - 4) >> 1;
}
#endif







static int intervals_to_count(int intervals) {
    return 1 + intervals * 5 + 1;
}




static int compute_worst_case_count(int a_intervals, int b_intervals) {
    
    int intervals = 2 * a_intervals * b_intervals + a_intervals + b_intervals;
    
    return intervals_to_count(intervals);
}

static bool setEmptyCheck(SkRegion* result) {
    return result ? result->setEmpty() : false;
}

static bool setRectCheck(SkRegion* result, const SkIRect& rect) {
    return result ? result->setRect(rect) : !rect.isEmpty();
}

static bool setRegionCheck(SkRegion* result, const SkRegion& rgn) {
    return result ? result->setRegion(rgn) : !rgn.isEmpty();
}

bool SkRegion::Oper(const SkRegion& rgnaOrig, const SkRegion& rgnbOrig, Op op,
                    SkRegion* result) {
    SkASSERT((unsigned)op < kOpCount);

    if (kReplace_Op == op) {
        return setRegionCheck(result, rgnbOrig);
    }

    
    const SkRegion* rgna = &rgnaOrig;
    const SkRegion* rgnb = &rgnbOrig;
    

    
    if (kReverseDifference_Op == op) {
        SkTSwap<const SkRegion*>(rgna, rgnb);
        op = kDifference_Op;
    }

    SkIRect bounds;
    bool    a_empty = rgna->isEmpty();
    bool    b_empty = rgnb->isEmpty();
    bool    a_rect = rgna->isRect();
    bool    b_rect = rgnb->isRect();

    switch (op) {
    case kDifference_Op:
        if (a_empty) {
            return setEmptyCheck(result);
        }
        if (b_empty || !SkIRect::IntersectsNoEmptyCheck(rgna->fBounds,
                                                        rgnb->fBounds)) {
            return setRegionCheck(result, *rgna);
        }
        if (b_rect && rgnb->fBounds.containsNoEmptyCheck(rgna->fBounds)) {
            return setEmptyCheck(result);
        }
        break;

    case kIntersect_Op:
        if ((a_empty | b_empty)
                || !bounds.intersect(rgna->fBounds, rgnb->fBounds)) {
            return setEmptyCheck(result);
        }
        if (a_rect & b_rect) {
            return setRectCheck(result, bounds);
        }
        if (a_rect && rgna->fBounds.contains(rgnb->fBounds)) {
            return setRegionCheck(result, *rgnb);
        }
        if (b_rect && rgnb->fBounds.contains(rgna->fBounds)) {
            return setRegionCheck(result, *rgna);
        }
        break;

    case kUnion_Op:
        if (a_empty) {
            return setRegionCheck(result, *rgnb);
        }
        if (b_empty) {
            return setRegionCheck(result, *rgna);
        }
        if (a_rect && rgna->fBounds.contains(rgnb->fBounds)) {
            return setRegionCheck(result, *rgna);
        }
        if (b_rect && rgnb->fBounds.contains(rgna->fBounds)) {
            return setRegionCheck(result, *rgnb);
        }
        break;

    case kXOR_Op:
        if (a_empty) {
            return setRegionCheck(result, *rgnb);
        }
        if (b_empty) {
            return setRegionCheck(result, *rgna);
        }
        break;
    default:
        SkDEBUGFAIL("unknown region op");
        return false;
    }

    RunType tmpA[kRectRegionRuns];
    RunType tmpB[kRectRegionRuns];

    int a_intervals, b_intervals;
    const RunType* a_runs = rgna->getRuns(tmpA, &a_intervals);
    const RunType* b_runs = rgnb->getRuns(tmpB, &b_intervals);

    int dstCount = compute_worst_case_count(a_intervals, b_intervals);
    SkAutoSTMalloc<256, RunType> array(dstCount);

#ifdef SK_DEBUG


#endif

    int count = operate(a_runs, b_runs, array.get(), op, NULL == result);
    SkASSERT(count <= dstCount);

    if (result) {
        SkASSERT(count >= 0);
        return result->setRuns(array.get(), count);
    } else {
        return (QUICK_EXIT_TRUE_COUNT == count) || !isRunCountEmpty(count);
    }
}

bool SkRegion::op(const SkRegion& rgna, const SkRegion& rgnb, Op op) {
    SkDEBUGCODE(this->validate();)
    return SkRegion::Oper(rgna, rgnb, op, this);
}



#include "SkBuffer.h"

size_t SkRegion::writeToMemory(void* storage) const {
    if (NULL == storage) {
        size_t size = sizeof(int32_t); 
        if (!this->isEmpty()) {
            size += sizeof(fBounds);
            if (this->isComplex()) {
                size += 2 * sizeof(int32_t);    
                size += fRunHead->fRunCount * sizeof(RunType);
            }
        }
        return size;
    }

    SkWBuffer   buffer(storage);

    if (this->isEmpty()) {
        buffer.write32(-1);
    } else {
        bool isRect = this->isRect();

        buffer.write32(isRect ? 0 : fRunHead->fRunCount);
        buffer.write(&fBounds, sizeof(fBounds));

        if (!isRect) {
            buffer.write32(fRunHead->getYSpanCount());
            buffer.write32(fRunHead->getIntervalCount());
            buffer.write(fRunHead->readonly_runs(),
                         fRunHead->fRunCount * sizeof(RunType));
        }
    }
    return buffer.pos();
}

size_t SkRegion::readFromMemory(const void* storage, size_t length) {
    SkRBufferWithSizeCheck  buffer(storage, length);
    SkRegion                tmp;
    int32_t                 count;

    if (buffer.readS32(&count) && (count >= 0) && buffer.read(&tmp.fBounds, sizeof(tmp.fBounds))) {
        if (count == 0) {
            tmp.fRunHead = SkRegion_gRectRunHeadPtr;
        } else {
            int32_t ySpanCount, intervalCount;
            if (buffer.readS32(&ySpanCount) && buffer.readS32(&intervalCount)) {
                tmp.allocateRuns(count, ySpanCount, intervalCount);
                buffer.read(tmp.fRunHead->writable_runs(), count * sizeof(RunType));
            }
        }
    }
    size_t sizeRead = 0;
    if (buffer.isValid()) {
        this->swap(tmp);
        sizeRead = buffer.pos();
    }
    return sizeRead;
}



const SkRegion& SkRegion::GetEmptyRegion() {
    static SkRegion gEmpty;
    return gEmpty;
}



#ifdef SK_DEBUG


static const SkRegion::RunType* skip_intervals_slow(const SkRegion::RunType runs[]) {
    
    
    
    
    SkRegion::RunType prevR = -SkRegion::kRunTypeSentinel;

    while (runs[0] < SkRegion::kRunTypeSentinel) {
        SkASSERT(prevR < runs[0]);
        SkASSERT(runs[0] < runs[1]);
        SkASSERT(runs[1] < SkRegion::kRunTypeSentinel);
        prevR = runs[1];
        runs += 2;
    }
    return runs;
}

static void compute_bounds(const SkRegion::RunType runs[],
                           SkIRect* bounds, int* ySpanCountPtr,
                           int* intervalCountPtr) {
    assert_sentinel(runs[0], false);    

    int left = SK_MaxS32;
    int rite = SK_MinS32;
    int bot;
    int ySpanCount = 0;
    int intervalCount = 0;

    bounds->fTop = *runs++;
    do {
        bot = *runs++;
        SkASSERT(SkRegion::kRunTypeSentinel > bot);

        ySpanCount += 1;

        runs += 1;  
        if (*runs < SkRegion::kRunTypeSentinel) {
            if (left > *runs) {
                left = *runs;
            }

            const SkRegion::RunType* prev = runs;
            runs = skip_intervals_slow(runs);
            int intervals = SkToInt((runs - prev) >> 1);
            SkASSERT(prev[-1] == intervals);
            intervalCount += intervals;

            if (rite < runs[-1]) {
                rite = runs[-1];
            }
        } else {
            SkASSERT(0 == runs[-1]);    
        }
        SkASSERT(SkRegion::kRunTypeSentinel == *runs);
        runs += 1;
    } while (SkRegion::kRunTypeSentinel != *runs);

    bounds->fLeft = left;
    bounds->fRight = rite;
    bounds->fBottom = bot;
    *ySpanCountPtr = ySpanCount;
    *intervalCountPtr = intervalCount;
}

void SkRegion::validate() const {
    if (this->isEmpty()) {
        
        
        
        SkASSERT(fBounds.fLeft == 0 && fBounds.fTop == 0 && fBounds.fRight == 0 && fBounds.fBottom == 0);
    } else {
        SkASSERT(!fBounds.isEmpty());
        if (!this->isRect()) {
            SkASSERT(fRunHead->fRefCnt >= 1);
            SkASSERT(fRunHead->fRunCount > kRectRegionRuns);

            const RunType* run = fRunHead->readonly_runs();

            
            {
                SkIRect bounds;
                int ySpanCount, intervalCount;
                compute_bounds(run, &bounds, &ySpanCount, &intervalCount);

                SkASSERT(bounds == fBounds);
                SkASSERT(ySpanCount > 0);
                SkASSERT(fRunHead->getYSpanCount() == ySpanCount);
           
                SkASSERT(fRunHead->getIntervalCount() == intervalCount);
            }
        }
    }
}

void SkRegion::dump() const {
    if (this->isEmpty()) {
        SkDebugf("  rgn: empty\n");
    } else {
        SkDebugf("  rgn: [%d %d %d %d]", fBounds.fLeft, fBounds.fTop, fBounds.fRight, fBounds.fBottom);
        if (this->isComplex()) {
            const RunType* runs = fRunHead->readonly_runs();
            for (int i = 0; i < fRunHead->fRunCount; i++)
                SkDebugf(" %d", runs[i]);
        }
        SkDebugf("\n");
    }
}

#endif



SkRegion::Iterator::Iterator(const SkRegion& rgn) {
    this->reset(rgn);
}

bool SkRegion::Iterator::rewind() {
    if (fRgn) {
        this->reset(*fRgn);
        return true;
    }
    return false;
}

void SkRegion::Iterator::reset(const SkRegion& rgn) {
    fRgn = &rgn;
    if (rgn.isEmpty()) {
        fDone = true;
    } else {
        fDone = false;
        if (rgn.isRect()) {
            fRect = rgn.fBounds;
            fRuns = NULL;
        } else {
            fRuns = rgn.fRunHead->readonly_runs();
            fRect.set(fRuns[3], fRuns[0], fRuns[4], fRuns[1]);
            fRuns += 5;
            
        }
    }
}

void SkRegion::Iterator::next() {
    if (fDone) {
        return;
    }

    if (fRuns == NULL) {   
        fDone = true;
        return;
    }

    const RunType* runs = fRuns;

    if (runs[0] < kRunTypeSentinel) { 
        fRect.fLeft = runs[0];
        fRect.fRight = runs[1];
        runs += 2;
    } else {    
        runs += 1;
        if (runs[0] < kRunTypeSentinel) { 
            int intervals = runs[1];
            if (0 == intervals) {    
                fRect.fTop = runs[0];
                runs += 3;
            } else {
                fRect.fTop = fRect.fBottom;
            }

            fRect.fBottom = runs[0];
            assert_sentinel(runs[2], false);
            assert_sentinel(runs[3], false);
            fRect.fLeft = runs[2];
            fRect.fRight = runs[3];
            runs += 4;
        } else {    
            fDone = true;
        }
    }
    fRuns = runs;
}

SkRegion::Cliperator::Cliperator(const SkRegion& rgn, const SkIRect& clip)
        : fIter(rgn), fClip(clip), fDone(true) {
    const SkIRect& r = fIter.rect();

    while (!fIter.done()) {
        if (r.fTop >= clip.fBottom) {
            break;
        }
        if (fRect.intersect(clip, r)) {
            fDone = false;
            break;
        }
        fIter.next();
    }
}

void SkRegion::Cliperator::next() {
    if (fDone) {
        return;
    }

    const SkIRect& r = fIter.rect();

    fDone = true;
    fIter.next();
    while (!fIter.done()) {
        if (r.fTop >= fClip.fBottom) {
            break;
        }
        if (fRect.intersect(fClip, r)) {
            fDone = false;
            break;
        }
        fIter.next();
    }
}



SkRegion::Spanerator::Spanerator(const SkRegion& rgn, int y, int left,
                                 int right) {
    SkDEBUGCODE(rgn.validate();)

    const SkIRect& r = rgn.getBounds();

    fDone = true;
    if (!rgn.isEmpty() && y >= r.fTop && y < r.fBottom &&
            right > r.fLeft && left < r.fRight) {
        if (rgn.isRect()) {
            if (left < r.fLeft) {
                left = r.fLeft;
            }
            if (right > r.fRight) {
                right = r.fRight;
            }
            fLeft = left;
            fRight = right;
            fRuns = NULL;    
            fDone = false;
        } else {
            const SkRegion::RunType* runs = rgn.fRunHead->findScanline(y);
            runs += 2;  
            for (;;) {
                
                if (runs[0] >= right) {
                    break;
                }
                
                if (runs[1] <= left) {
                    runs += 2;
                    continue;
                }
                
                fRuns = runs;
                fLeft = left;
                fRight = right;
                fDone = false;
                break;
            }
        }
    }
}

bool SkRegion::Spanerator::next(int* left, int* right) {
    if (fDone) {
        return false;
    }

    if (fRuns == NULL) {   
        fDone = true;   
        if (left) {
            *left = fLeft;
        }
        if (right) {
            *right = fRight;
        }
        return true;    
    }

    const SkRegion::RunType* runs = fRuns;

    if (runs[0] >= fRight) {
        fDone = true;
        return false;
    }

    SkASSERT(runs[1] > fLeft);

    if (left) {
        *left = SkMax32(fLeft, runs[0]);
    }
    if (right) {
        *right = SkMin32(fRight, runs[1]);
    }
    fRuns = runs + 2;
    return true;
}



#ifdef SK_DEBUG

bool SkRegion::debugSetRuns(const RunType runs[], int count) {
    
    

    SkAutoTArray<RunType> storage(count);
    memcpy(storage.get(), runs, count * sizeof(RunType));
    return this->setRuns(storage.get(), count);
}

#endif
