








#ifndef SkRect_DEFINED
#define SkRect_DEFINED

#include "SkPoint.h"
#include "SkSize.h"





struct SK_API SkIRect {
    int32_t fLeft, fTop, fRight, fBottom;

    static SkIRect SK_WARN_UNUSED_RESULT MakeEmpty() {
        SkIRect r;
        r.setEmpty();
        return r;
    }

    static SkIRect SK_WARN_UNUSED_RESULT MakeLargest() {
        SkIRect r;
        r.setLargest();
        return r;
    }

    static SkIRect SK_WARN_UNUSED_RESULT MakeWH(int32_t w, int32_t h) {
        SkIRect r;
        r.set(0, 0, w, h);
        return r;
    }

    static SkIRect SK_WARN_UNUSED_RESULT MakeSize(const SkISize& size) {
        SkIRect r;
        r.set(0, 0, size.width(), size.height());
        return r;
    }

    static SkIRect SK_WARN_UNUSED_RESULT MakeLTRB(int32_t l, int32_t t, int32_t r, int32_t b) {
        SkIRect rect;
        rect.set(l, t, r, b);
        return rect;
    }

    static SkIRect SK_WARN_UNUSED_RESULT MakeXYWH(int32_t x, int32_t y, int32_t w, int32_t h) {
        SkIRect r;
        r.set(x, y, x + w, y + h);
        return r;
    }

    int left() const { return fLeft; }
    int top() const { return fTop; }
    int right() const { return fRight; }
    int bottom() const { return fBottom; }

    
    int x() const { return fLeft; }
    
    int y() const { return fTop; }
    



    int width() const { return fRight - fLeft; }

    



    int height() const { return fBottom - fTop; }

    






    int centerX() const { return (fRight + fLeft) >> 1; }

    






    int centerY() const { return (fBottom + fTop) >> 1; }

    


    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

    bool isLargest() const { return SK_MinS32 == fLeft &&
                                    SK_MinS32 == fTop &&
                                    SK_MaxS32 == fRight &&
                                    SK_MaxS32 == fBottom; }

    friend bool operator==(const SkIRect& a, const SkIRect& b) {
        return !memcmp(&a, &b, sizeof(a));
    }

    friend bool operator!=(const SkIRect& a, const SkIRect& b) {
        return !(a == b);
    }

    bool is16Bit() const {
        return  SkIsS16(fLeft) && SkIsS16(fTop) &&
                SkIsS16(fRight) && SkIsS16(fBottom);
    }

    

    void setEmpty() { memset(this, 0, sizeof(*this)); }

    void set(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        fLeft   = left;
        fTop    = top;
        fRight  = right;
        fBottom = bottom;
    }
    
    void setLTRB(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        this->set(left, top, right, bottom);
    }

    void setXYWH(int32_t x, int32_t y, int32_t width, int32_t height) {
        fLeft = x;
        fTop = y;
        fRight = x + width;
        fBottom = y + height;
    }

    


    void setLargest() {
        fLeft = fTop = SK_MinS32;
        fRight = fBottom = SK_MaxS32;
    }

    



    void setLargestInverted() {
        fLeft = fTop = SK_MaxS32;
        fRight = fBottom = SK_MinS32;
    }

    


    SkIRect makeOffset(int dx, int dy) const {
        return MakeLTRB(fLeft + dx, fTop + dy, fRight + dx, fBottom + dy);
    }

    


    SkIRect makeInset(int dx, int dy) const {
        return MakeLTRB(fLeft + dx, fTop + dy, fRight - dx, fBottom - dy);
    }

    


    void offset(int32_t dx, int32_t dy) {
        fLeft   += dx;
        fTop    += dy;
        fRight  += dx;
        fBottom += dy;
    }

    void offset(const SkIPoint& delta) {
        this->offset(delta.fX, delta.fY);
    }

    


    void offsetTo(int32_t newX, int32_t newY) {
        fRight += newX - fLeft;
        fBottom += newY - fTop;
        fLeft = newX;
        fTop = newY;
    }

    



    void inset(int32_t dx, int32_t dy) {
        fLeft   += dx;
        fTop    += dy;
        fRight  -= dx;
        fBottom -= dy;
    }

   




    void outset(int32_t dx, int32_t dy)  { this->inset(-dx, -dy); }

    bool quickReject(int l, int t, int r, int b) const {
        return l >= fRight || fLeft >= r || t >= fBottom || fTop >= b;
    }

    




    bool contains(int32_t x, int32_t y) const {
        return  (unsigned)(x - fLeft) < (unsigned)(fRight - fLeft) &&
                (unsigned)(y - fTop) < (unsigned)(fBottom - fTop);
    }

    


    bool contains(int32_t left, int32_t top, int32_t right, int32_t bottom) const {
        return  left < right && top < bottom && !this->isEmpty() && 
                fLeft <= left && fTop <= top &&
                fRight >= right && fBottom >= bottom;
    }

    

    bool contains(const SkIRect& r) const {
        return  !r.isEmpty() && !this->isEmpty() &&     
                fLeft <= r.fLeft && fTop <= r.fTop &&
                fRight >= r.fRight && fBottom >= r.fBottom;
    }

    





    bool containsNoEmptyCheck(int32_t left, int32_t top,
                              int32_t right, int32_t bottom) const {
        SkASSERT(fLeft < fRight && fTop < fBottom);
        SkASSERT(left < right && top < bottom);

        return fLeft <= left && fTop <= top &&
               fRight >= right && fBottom >= bottom;
    }

    bool containsNoEmptyCheck(const SkIRect& r) const {
        return containsNoEmptyCheck(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    



    bool intersect(const SkIRect& r) {
        SkASSERT(&r);
        return this->intersect(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    



    bool intersect(const SkIRect& a, const SkIRect& b) {
        SkASSERT(&a && &b);

        if (!a.isEmpty() && !b.isEmpty() &&
                a.fLeft < b.fRight && b.fLeft < a.fRight &&
                a.fTop < b.fBottom && b.fTop < a.fBottom) {
            fLeft   = SkMax32(a.fLeft,   b.fLeft);
            fTop    = SkMax32(a.fTop,    b.fTop);
            fRight  = SkMin32(a.fRight,  b.fRight);
            fBottom = SkMin32(a.fBottom, b.fBottom);
            return true;
        }
        return false;
    }

    





    bool intersectNoEmptyCheck(const SkIRect& a, const SkIRect& b) {
        SkASSERT(&a && &b);
        SkASSERT(!a.isEmpty() && !b.isEmpty());

        if (a.fLeft < b.fRight && b.fLeft < a.fRight &&
                a.fTop < b.fBottom && b.fTop < a.fBottom) {
            fLeft   = SkMax32(a.fLeft,   b.fLeft);
            fTop    = SkMax32(a.fTop,    b.fTop);
            fRight  = SkMin32(a.fRight,  b.fRight);
            fBottom = SkMin32(a.fBottom, b.fBottom);
            return true;
        }
        return false;
    }

    




    bool intersect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        if (left < right && top < bottom && !this->isEmpty() &&
                fLeft < right && left < fRight && fTop < bottom && top < fBottom) {
            if (fLeft < left) fLeft = left;
            if (fTop < top) fTop = top;
            if (fRight > right) fRight = right;
            if (fBottom > bottom) fBottom = bottom;
            return true;
        }
        return false;
    }

    

    static bool Intersects(const SkIRect& a, const SkIRect& b) {
        return  !a.isEmpty() && !b.isEmpty() &&              
        a.fLeft < b.fRight && b.fLeft < a.fRight &&
        a.fTop < b.fBottom && b.fTop < a.fBottom;
    }

    


    static bool IntersectsNoEmptyCheck(const SkIRect& a, const SkIRect& b) {
        SkASSERT(!a.isEmpty());
        SkASSERT(!b.isEmpty());
        return  a.fLeft < b.fRight && b.fLeft < a.fRight &&
                a.fTop < b.fBottom && b.fTop < a.fBottom;
    }

    



    void join(int32_t left, int32_t top, int32_t right, int32_t bottom);

    



    void join(const SkIRect& r) {
        this->join(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    




    void sort();

    static const SkIRect& SK_WARN_UNUSED_RESULT EmptyIRect() {
        static const SkIRect gEmpty = { 0, 0, 0, 0 };
        return gEmpty;
    }
};



struct SK_API SkRect {
    SkScalar    fLeft, fTop, fRight, fBottom;

    static SkRect SK_WARN_UNUSED_RESULT MakeEmpty() {
        SkRect r;
        r.setEmpty();
        return r;
    }

    static SkRect SK_WARN_UNUSED_RESULT MakeLargest() {
        SkRect r;
        r.setLargest();
        return r;
    }

    static SkRect SK_WARN_UNUSED_RESULT MakeWH(SkScalar w, SkScalar h) {
        SkRect r;
        r.set(0, 0, w, h);
        return r;
    }

    static SkRect SK_WARN_UNUSED_RESULT MakeSize(const SkSize& size) {
        SkRect r;
        r.set(0, 0, size.width(), size.height());
        return r;
    }

    static SkRect SK_WARN_UNUSED_RESULT MakeLTRB(SkScalar l, SkScalar t, SkScalar r, SkScalar b) {
        SkRect rect;
        rect.set(l, t, r, b);
        return rect;
    }

    static SkRect SK_WARN_UNUSED_RESULT MakeXYWH(SkScalar x, SkScalar y, SkScalar w, SkScalar h) {
        SkRect r;
        r.set(x, y, x + w, y + h);
        return r;
    }

    SK_ATTR_DEPRECATED("use Make()")
    static SkRect SK_WARN_UNUSED_RESULT MakeFromIRect(const SkIRect& irect) {
        SkRect r;
        r.set(SkIntToScalar(irect.fLeft),
              SkIntToScalar(irect.fTop),
              SkIntToScalar(irect.fRight),
              SkIntToScalar(irect.fBottom));
        return r;
    }

    static SkRect SK_WARN_UNUSED_RESULT Make(const SkIRect& irect) {
        SkRect r;
        r.set(SkIntToScalar(irect.fLeft),
              SkIntToScalar(irect.fTop),
              SkIntToScalar(irect.fRight),
              SkIntToScalar(irect.fBottom));
        return r;
    }

    


    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

    bool isLargest() const { return SK_ScalarMin == fLeft &&
                                    SK_ScalarMin == fTop &&
                                    SK_ScalarMax == fRight &&
                                    SK_ScalarMax == fBottom; }

    




    bool isFinite() const {
        float accum = 0;
        accum *= fLeft;
        accum *= fTop;
        accum *= fRight;
        accum *= fBottom;

        
        SkASSERT(0 == accum || !(accum == accum));

        
        
        return accum == accum;
    }

    SkScalar    x() const { return fLeft; }
    SkScalar    y() const { return fTop; }
    SkScalar    left() const { return fLeft; }
    SkScalar    top() const { return fTop; }
    SkScalar    right() const { return fRight; }
    SkScalar    bottom() const { return fBottom; }
    SkScalar    width() const { return fRight - fLeft; }
    SkScalar    height() const { return fBottom - fTop; }
    SkScalar    centerX() const { return SkScalarHalf(fLeft + fRight); }
    SkScalar    centerY() const { return SkScalarHalf(fTop + fBottom); }

    friend bool operator==(const SkRect& a, const SkRect& b) {
        return SkScalarsEqual((SkScalar*)&a, (SkScalar*)&b, 4);
    }

    friend bool operator!=(const SkRect& a, const SkRect& b) {
        return !SkScalarsEqual((SkScalar*)&a, (SkScalar*)&b, 4);
    }

    


    void toQuad(SkPoint quad[4]) const;

    

    void setEmpty() { memset(this, 0, sizeof(*this)); }

    void set(const SkIRect& src) {
        fLeft   = SkIntToScalar(src.fLeft);
        fTop    = SkIntToScalar(src.fTop);
        fRight  = SkIntToScalar(src.fRight);
        fBottom = SkIntToScalar(src.fBottom);
    }

    void set(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom) {
        fLeft   = left;
        fTop    = top;
        fRight  = right;
        fBottom = bottom;
    }
    
    void setLTRB(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom) {
        this->set(left, top, right, bottom);
    }

    


    void iset(int left, int top, int right, int bottom) {
        fLeft   = SkIntToScalar(left);
        fTop    = SkIntToScalar(top);
        fRight  = SkIntToScalar(right);
        fBottom = SkIntToScalar(bottom);
    }

    



    void isetWH(int width, int height) {
        fLeft = fTop = 0;
        fRight = SkIntToScalar(width);
        fBottom = SkIntToScalar(height);
    }

    



    void set(const SkPoint pts[], int count) {
        
        
        
        (void)this->setBoundsCheck(pts, count);
    }

    
    void setBounds(const SkPoint pts[], int count) {
        (void)this->setBoundsCheck(pts, count);
    }

    




    bool setBoundsCheck(const SkPoint pts[], int count);

    void set(const SkPoint& p0, const SkPoint& p1) {
        fLeft =   SkMinScalar(p0.fX, p1.fX);
        fRight =  SkMaxScalar(p0.fX, p1.fX);
        fTop =    SkMinScalar(p0.fY, p1.fY);
        fBottom = SkMaxScalar(p0.fY, p1.fY);
    }

    void setXYWH(SkScalar x, SkScalar y, SkScalar width, SkScalar height) {
        fLeft = x;
        fTop = y;
        fRight = x + width;
        fBottom = y + height;
    }

    void setWH(SkScalar width, SkScalar height) {
        fLeft = 0;
        fTop = 0;
        fRight = width;
        fBottom = height;
    }

    


    void setLargest() {
        fLeft = fTop = SK_ScalarMin;
        fRight = fBottom = SK_ScalarMax;
    }

    



    void setLargestInverted() {
        fLeft = fTop = SK_ScalarMax;
        fRight = fBottom = SK_ScalarMin;
    }

    


    SkRect makeOffset(SkScalar dx, SkScalar dy) const {
        return MakeLTRB(fLeft + dx, fTop + dy, fRight + dx, fBottom + dy);
    }

    


    SkRect makeInset(SkScalar dx, SkScalar dy) const {
        return MakeLTRB(fLeft + dx, fTop + dy, fRight - dx, fBottom - dy);
    }

    


    void offset(SkScalar dx, SkScalar dy) {
        fLeft   += dx;
        fTop    += dy;
        fRight  += dx;
        fBottom += dy;
    }

    void offset(const SkPoint& delta) {
        this->offset(delta.fX, delta.fY);
    }

    


    void offsetTo(SkScalar newX, SkScalar newY) {
        fRight += newX - fLeft;
        fBottom += newY - fTop;
        fLeft = newX;
        fTop = newY;
    }

    




    void inset(SkScalar dx, SkScalar dy)  {
        fLeft   += dx;
        fTop    += dy;
        fRight  -= dx;
        fBottom -= dy;
    }

   




    void outset(SkScalar dx, SkScalar dy)  { this->inset(-dx, -dy); }

    



    bool intersect(const SkRect& r);
    bool intersect2(const SkRect& r);

    




    bool intersect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom);

    



    bool intersects(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom) const {
        return 
               left < right && top < bottom &&
               fLeft < fRight && fTop < fBottom &&
               
               fLeft < right && left < fRight &&
               fTop < bottom && top < fBottom;
    }

    



    bool intersect(const SkRect& a, const SkRect& b);

    


    static bool Intersects(const SkRect& a, const SkRect& b) {
        return  !a.isEmpty() && !b.isEmpty() &&
                a.fLeft < b.fRight && b.fLeft < a.fRight &&
                a.fTop < b.fBottom && b.fTop < a.fBottom;
    }

    




    void join(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom);

    



    void join(const SkRect& r) {
        this->join(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }
    
    void growToInclude(const SkRect& r) { this->join(r); }

    








    void growToInclude(SkScalar x, SkScalar y) {
        fLeft  = SkMinScalar(x, fLeft);
        fRight = SkMaxScalar(x, fRight);
        fTop    = SkMinScalar(y, fTop);
        fBottom = SkMaxScalar(y, fBottom);
    }

    
    void growToInclude(const SkPoint pts[], int count) {
        this->growToInclude(pts, sizeof(SkPoint), count);
    }

    
    void growToInclude(const SkPoint pts[], size_t stride, int count) {
        SkASSERT(count >= 0);
        SkASSERT(stride >= sizeof(SkPoint));
        const SkPoint* end = (const SkPoint*)((intptr_t)pts + count * stride);
        for (; pts < end; pts = (const SkPoint*)((intptr_t)pts + stride)) {
            this->growToInclude(pts->fX, pts->fY);
        }
    }

    



    bool contains(const SkRect& r) const {
        
        return  !r.isEmpty() && !this->isEmpty() &&
                fLeft <= r.fLeft && fTop <= r.fTop &&
                fRight >= r.fRight && fBottom >= r.fBottom;
    }

    



    void round(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkScalarRoundToInt(fLeft), SkScalarRoundToInt(fTop),
                 SkScalarRoundToInt(fRight), SkScalarRoundToInt(fBottom));
    }

    











    void dround(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkDScalarRoundToInt(fLeft), SkDScalarRoundToInt(fTop),
                 SkDScalarRoundToInt(fRight), SkDScalarRoundToInt(fBottom));
    }

    



    void roundOut(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkScalarFloorToInt(fLeft), SkScalarFloorToInt(fTop),
                 SkScalarCeilToInt(fRight), SkScalarCeilToInt(fBottom));
    }

    




    void roundOut() {
        this->set(SkScalarFloorToScalar(fLeft),
                  SkScalarFloorToScalar(fTop),
                  SkScalarCeilToScalar(fRight),
                  SkScalarCeilToScalar(fBottom));
    }

    





    void roundIn(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkScalarCeilToInt(fLeft), SkScalarCeilToInt(fTop),
                 SkScalarFloorToInt(fRight), SkScalarFloorToInt(fBottom));
    }

    



    SkIRect round() const {
        SkIRect ir;
        this->round(&ir);
        return ir;
    }

    





    void sort();

    


    const SkScalar* asScalars() const { return &fLeft; }

#ifdef SK_DEVELOPER
    



    void dump() const {
        SkDebugf("{ l: %f, t: %f, r: %f, b: %f }", fLeft, fTop, fRight, fBottom);
    }
#endif

};

#endif
