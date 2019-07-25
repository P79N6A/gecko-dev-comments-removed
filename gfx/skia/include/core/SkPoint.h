








#ifndef SkPoint_DEFINED
#define SkPoint_DEFINED

#include "SkMath.h"
#include "SkScalar.h"





struct SkIPoint {
    int32_t fX, fY;

    static SkIPoint Make(int32_t x, int32_t y) {
        SkIPoint pt;
        pt.set(x, y);
        return pt;
    }

    int32_t x() const { return fX; }
    int32_t y() const { return fY; }
    void setX(int32_t x) { fX = x; }
    void setY(int32_t y) { fY = y; }

    


    bool isZero() const { return (fX | fY) == 0; }

    


    void setZero() { fX = fY = 0; }

    
    void set(int32_t x, int32_t y) { fX = x; fY = y; }

    


    void rotateCW(SkIPoint* dst) const;

    


    void rotateCW() { this->rotateCW(this); }

    


    void rotateCCW(SkIPoint* dst) const;

    


    void rotateCCW() { this->rotateCCW(this); }

    

    void negate() { fX = -fX; fY = -fY; }

    


    SkIPoint operator-() const {
        SkIPoint neg;
        neg.fX = -fX;
        neg.fY = -fY;
        return neg;
    }

    
    void operator+=(const SkIPoint& v) {
        fX += v.fX;
        fY += v.fY;
    }

    
    void operator-=(const SkIPoint& v) {
        fX -= v.fX;
        fY -= v.fY;
    }

    
    bool equals(int32_t x, int32_t y) const {
        return fX == x && fY == y;
    }

    friend bool operator==(const SkIPoint& a, const SkIPoint& b) {
        return a.fX == b.fX && a.fY == b.fY;
    }

    friend bool operator!=(const SkIPoint& a, const SkIPoint& b) {
        return a.fX != b.fX || a.fY != b.fY;
    }

    


    friend SkIPoint operator-(const SkIPoint& a, const SkIPoint& b) {
        SkIPoint v;
        v.set(a.fX - b.fX, a.fY - b.fY);
        return v;
    }

    

    friend SkIPoint operator+(const SkIPoint& a, const SkIPoint& b) {
        SkIPoint v;
        v.set(a.fX + b.fX, a.fY + b.fY);
        return v;
    }

    

    static int32_t DotProduct(const SkIPoint& a, const SkIPoint& b) {
        return a.fX * b.fX + a.fY * b.fY;
    }

    

    static int32_t CrossProduct(const SkIPoint& a, const SkIPoint& b) {
        return a.fX * b.fY - a.fY * b.fX;
    }
};

struct SK_API SkPoint {
    SkScalar    fX, fY;

    static SkPoint Make(SkScalar x, SkScalar y) {
        SkPoint pt;
        pt.set(x, y);
        return pt;
    }

    SkScalar x() const { return fX; }
    SkScalar y() const { return fY; }

    
    void set(SkScalar x, SkScalar y) { fX = x; fY = y; }

    


    void iset(int32_t x, int32_t y) {
        fX = SkIntToScalar(x);
        fY = SkIntToScalar(y);
    }

    


    void iset(const SkIPoint& p) {
        fX = SkIntToScalar(p.fX);
        fY = SkIntToScalar(p.fY);
    }

    void setAbs(const SkPoint& pt) {
        fX = SkScalarAbs(pt.fX);
        fY = SkScalarAbs(pt.fY);
    }
    
    
    void setIRectFan(int l, int t, int r, int b) {
        SkPoint* v = this;
        v[0].set(SkIntToScalar(l), SkIntToScalar(t));
        v[1].set(SkIntToScalar(l), SkIntToScalar(b));
        v[2].set(SkIntToScalar(r), SkIntToScalar(b));
        v[3].set(SkIntToScalar(r), SkIntToScalar(t));
    }
    void setIRectFan(int l, int t, int r, int b, size_t stride);

    
    void setRectFan(SkScalar l, SkScalar t, SkScalar r, SkScalar b) {
        SkPoint* v = this;
        v[0].set(l, t);
        v[1].set(l, b);
        v[2].set(r, b);
        v[3].set(r, t);
    }
    void setRectFan(SkScalar l, SkScalar t, SkScalar r, SkScalar b, size_t stride);

    static void Offset(SkPoint points[], int count, const SkPoint& offset) {
        Offset(points, count, offset.fX, offset.fY);
    }

    static void Offset(SkPoint points[], int count, SkScalar dx, SkScalar dy) {
        for (int i = 0; i < count; ++i) {
            points[i].offset(dx, dy);
        }
    }

    void offset(SkScalar dx, SkScalar dy) {
        fX += dx;
        fY += dy;
    }

    

    SkScalar length() const { return SkPoint::Length(fX, fY); }
    SkScalar distanceToOrigin() const { return this->length(); }

    



    static bool CanNormalize(SkScalar dx, SkScalar dy);

    bool canNormalize() const {
        return CanNormalize(fX, fY);
    }

    



    bool normalize();

    



    bool setNormalize(SkScalar x, SkScalar y);

    



    bool setLength(SkScalar length);

    



    bool setLength(SkScalar x, SkScalar y, SkScalar length);

    


    void scale(SkScalar scale, SkPoint* dst) const;

    


    void scale(SkScalar value) { this->scale(value, this); }

    


    void rotateCW(SkPoint* dst) const;

    


    void rotateCW() { this->rotateCW(this); }

    


    void rotateCCW(SkPoint* dst) const;

    


    void rotateCCW() { this->rotateCCW(this); }

    

    void negate() {
        fX = -fX;
        fY = -fY;
    }

    

    SkPoint operator-() const {
        SkPoint neg;
        neg.fX = -fX;
        neg.fY = -fY;
        return neg;
    }

    

    void operator+=(const SkPoint& v) {
        fX += v.fX;
        fY += v.fY;
    }

    

    void operator-=(const SkPoint& v) {
        fX -= v.fX;
        fY -= v.fY;
    }

    

    bool equals(SkScalar x, SkScalar y) const { return fX == x && fY == y; }

    friend bool operator==(const SkPoint& a, const SkPoint& b) {
        return a.fX == b.fX && a.fY == b.fY;
    }

    friend bool operator!=(const SkPoint& a, const SkPoint& b) {
        return a.fX != b.fX || a.fY != b.fY;
    }

    


    friend SkPoint operator-(const SkPoint& a, const SkPoint& b) {
        SkPoint v;
        v.set(a.fX - b.fX, a.fY - b.fY);
        return v;
    }

    

    friend SkPoint operator+(const SkPoint& a, const SkPoint& b) {
        SkPoint v;
        v.set(a.fX + b.fX, a.fY + b.fY);
        return v;
    }

    

    static SkScalar Length(SkScalar x, SkScalar y);

    








    static SkScalar Normalize(SkPoint* pt);

    

    static SkScalar Distance(const SkPoint& a, const SkPoint& b) {
        return Length(a.fX - b.fX, a.fY - b.fY);
    }

    

    static SkScalar DotProduct(const SkPoint& a, const SkPoint& b) {
        return SkScalarMul(a.fX, b.fX) + SkScalarMul(a.fY, b.fY);
    }

    

    static SkScalar CrossProduct(const SkPoint& a, const SkPoint& b) {
        return SkScalarMul(a.fX, b.fY) - SkScalarMul(a.fY, b.fX);
    }

    SkScalar cross(const SkPoint& vec) const {
        return CrossProduct(*this, vec);
    }

    SkScalar dot(const SkPoint& vec) const {
        return DotProduct(*this, vec);
    }
    
    SkScalar lengthSqd() const {
        return DotProduct(*this, *this);
    }
    
    SkScalar distanceToSqd(const SkPoint& pt) const {
        SkScalar dx = fX - pt.fX;
        SkScalar dy = fY - pt.fY;
        return SkScalarMul(dx, dx) + SkScalarMul(dy, dy);
    }

    



    enum Side {
        kLeft_Side  = -1,
        kOn_Side    =  0,
        kRight_Side =  1
    };

    




    SkScalar distanceToLineBetweenSqd(const SkPoint& a,
                                      const SkPoint& b,
                                      Side* side = NULL) const;

    




    SkScalar distanceToLineBetween(const SkPoint& a,
                                   const SkPoint& b,
                                   Side* side = NULL) const {
        return SkScalarSqrt(this->distanceToLineBetweenSqd(a, b, side));
    }

    


    SkScalar distanceToLineSegmentBetweenSqd(const SkPoint& a,
                                             const SkPoint& b) const;

    


    SkScalar distanceToLineSegmentBetween(const SkPoint& a,
                                          const SkPoint& b) const {
        return SkScalarSqrt(this->distanceToLineSegmentBetweenSqd(a, b));
    }

    




    void setOrthog(const SkPoint& vec, Side side = kLeft_Side) {
        
        SkScalar tmp = vec.fX;
        if (kLeft_Side == side) {
            fX = -vec.fY;
            fY = tmp;
        } else {
            SkASSERT(kRight_Side == side);
            fX = vec.fY;
            fY = -tmp;
        }
    }
};

typedef SkPoint SkVector;

#endif
