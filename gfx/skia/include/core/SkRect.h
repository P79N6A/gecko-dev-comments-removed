








#ifndef SkRect_DEFINED
#define SkRect_DEFINED

#include "SkPoint.h"
#include "SkSize.h"





struct SK_API SkIRect {
    int32_t fLeft, fTop, fRight, fBottom;

    static SkIRect MakeEmpty() {
        SkIRect r;
        r.setEmpty();
        return r;
    }
    
    static SkIRect MakeWH(int32_t w, int32_t h) {
        SkIRect r;
        r.set(0, 0, w, h);
        return r;
    }
    
    static SkIRect MakeSize(const SkISize& size) {
        SkIRect r;
        r.set(0, 0, size.width(), size.height());
        return r;
    }
    
    static SkIRect MakeLTRB(int32_t l, int32_t t, int32_t r, int32_t b) {
        SkIRect rect;
        rect.set(l, t, r, b);
        return rect;
    }
    
    static SkIRect MakeXYWH(int32_t x, int32_t y, int32_t w, int32_t h) {
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
    
    


    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

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
    
    


    void offset(int32_t dx, int32_t dy) {
        fLeft   += dx;
        fTop    += dy;
        fRight  += dx;
        fBottom += dy;
    }

    void offset(const SkIPoint& delta) {
        this->offset(delta.fX, delta.fY);
    }

    



    void inset(int32_t dx, int32_t dy) {
        fLeft   += dx;
        fTop    += dy;
        fRight  -= dx;
        fBottom -= dy;
    }

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
    
    



    void join(int32_t left, int32_t top, int32_t right, int32_t bottom);

    



    void join(const SkIRect& r) {
        this->join(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    




    void sort();

    static const SkIRect& EmptyIRect() {
        static const SkIRect gEmpty = { 0, 0, 0, 0 };
        return gEmpty;
    }
};



struct SK_API SkRect {
    SkScalar    fLeft, fTop, fRight, fBottom;

    static SkRect MakeEmpty() {
        SkRect r;
        r.setEmpty();
        return r;
    }

    static SkRect MakeWH(SkScalar w, SkScalar h) {
        SkRect r;
        r.set(0, 0, w, h);
        return r;
    }

    static SkRect MakeSize(const SkSize& size) {
        SkRect r;
        r.set(0, 0, size.width(), size.height());
        return r;
    }

    static SkRect MakeLTRB(SkScalar l, SkScalar t, SkScalar r, SkScalar b) {
        SkRect rect;
        rect.set(l, t, r, b);
        return rect;
    }

    static SkRect MakeXYWH(SkScalar x, SkScalar y, SkScalar w, SkScalar h) {
        SkRect r;
        r.set(x, y, x + w, y + h);
        return r;
    }

    

    bool        isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }
    bool        hasValidCoordinates() const;
    SkScalar    left() const { return fLeft; }
    SkScalar    top() const { return fTop; }
    SkScalar    right() const { return fRight; }
    SkScalar    bottom() const { return fBottom; }
    SkScalar    width() const { return fRight - fLeft; }
    SkScalar    height() const { return fBottom - fTop; }
    SkScalar    centerX() const { return SkScalarHalf(fLeft + fRight); }
    SkScalar    centerY() const { return SkScalarHalf(fTop + fBottom); }

    friend bool operator==(const SkRect& a, const SkRect& b) {
        return 0 == memcmp(&a, &b, sizeof(a));
    }

    friend bool operator!=(const SkRect& a, const SkRect& b) {
        return 0 != memcmp(&a, &b, sizeof(a));
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

    



    void set(const SkPoint pts[], int count);

    
    void setBounds(const SkPoint pts[], int count) {
        this->set(pts, count);
    }

    void setXYWH(SkScalar x, SkScalar y, SkScalar width, SkScalar height) {
        fLeft = x;
        fTop = y;
        fRight = x + width;
        fBottom = y + height;
    }

    


    void setLargest() {
        fLeft = fTop = SK_ScalarMin;
        fRight = fBottom = SK_ScalarMax;
    }
    
    



    void setLargestInverted() {
        fLeft = fTop = SK_ScalarMax;
        fRight = fBottom = SK_ScalarMin;
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

    




    void inset(SkScalar dx, SkScalar dy)  {
        fLeft   += dx;
        fTop    += dy;
        fRight  -= dx;
        fBottom -= dy;
    }

   




    void outset(SkScalar dx, SkScalar dy)  { this->inset(-dx, -dy); }

    



    bool intersect(const SkRect& r);

    




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
    
    








    bool contains(const SkPoint& p) const {
        return !this->isEmpty() &&
               fLeft <= p.fX && p.fX < fRight && fTop <= p.fY && p.fY < fBottom;
    }

    








    bool contains(SkScalar x, SkScalar y) const {
        return  !this->isEmpty() &&
                fLeft <= x && x < fRight && fTop <= y && y < fBottom;
    }

    



    bool contains(const SkRect& r) const {
        return  !r.isEmpty() && !this->isEmpty() &&
                fLeft <= r.fLeft && fTop <= r.fTop &&
                fRight >= r.fRight && fBottom >= r.fBottom;
    }

    



    void round(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkScalarRound(fLeft), SkScalarRound(fTop),
                 SkScalarRound(fRight), SkScalarRound(fBottom));
    }

    



    void roundOut(SkIRect* dst) const {
        SkASSERT(dst);
        dst->set(SkScalarFloor(fLeft), SkScalarFloor(fTop),
                 SkScalarCeil(fRight), SkScalarCeil(fBottom));
    }

    




    void roundOut() {
        this->set(SkScalarFloorToScalar(fLeft),
                  SkScalarFloorToScalar(fTop),
                  SkScalarCeilToScalar(fRight),
                  SkScalarCeilToScalar(fBottom));
    }

    





    void sort();
};

#endif

