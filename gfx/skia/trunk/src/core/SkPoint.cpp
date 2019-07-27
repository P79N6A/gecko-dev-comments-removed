








#include "SkMathPriv.h"
#include "SkPoint.h"

void SkIPoint::rotateCW(SkIPoint* dst) const {
    SkASSERT(dst);

    
    int32_t tmp = fX;
    dst->fX = -fY;
    dst->fY = tmp;
}

void SkIPoint::rotateCCW(SkIPoint* dst) const {
    SkASSERT(dst);

    
    int32_t tmp = fX;
    dst->fX = fY;
    dst->fY = -tmp;
}



void SkPoint::setIRectFan(int l, int t, int r, int b, size_t stride) {
    SkASSERT(stride >= sizeof(SkPoint));

    ((SkPoint*)((intptr_t)this + 0 * stride))->set(SkIntToScalar(l),
                                                   SkIntToScalar(t));
    ((SkPoint*)((intptr_t)this + 1 * stride))->set(SkIntToScalar(l),
                                                   SkIntToScalar(b));
    ((SkPoint*)((intptr_t)this + 2 * stride))->set(SkIntToScalar(r),
                                                   SkIntToScalar(b));
    ((SkPoint*)((intptr_t)this + 3 * stride))->set(SkIntToScalar(r),
                                                   SkIntToScalar(t));
}

void SkPoint::setRectFan(SkScalar l, SkScalar t, SkScalar r, SkScalar b,
                         size_t stride) {
    SkASSERT(stride >= sizeof(SkPoint));

    ((SkPoint*)((intptr_t)this + 0 * stride))->set(l, t);
    ((SkPoint*)((intptr_t)this + 1 * stride))->set(l, b);
    ((SkPoint*)((intptr_t)this + 2 * stride))->set(r, b);
    ((SkPoint*)((intptr_t)this + 3 * stride))->set(r, t);
}

void SkPoint::rotateCW(SkPoint* dst) const {
    SkASSERT(dst);

    
    SkScalar tmp = fX;
    dst->fX = -fY;
    dst->fY = tmp;
}

void SkPoint::rotateCCW(SkPoint* dst) const {
    SkASSERT(dst);

    
    SkScalar tmp = fX;
    dst->fX = fY;
    dst->fY = -tmp;
}

void SkPoint::scale(SkScalar scale, SkPoint* dst) const {
    SkASSERT(dst);
    dst->set(SkScalarMul(fX, scale), SkScalarMul(fY, scale));
}

bool SkPoint::normalize() {
    return this->setLength(fX, fY, SK_Scalar1);
}

bool SkPoint::setNormalize(SkScalar x, SkScalar y) {
    return this->setLength(x, y, SK_Scalar1);
}

bool SkPoint::setLength(SkScalar length) {
    return this->setLength(fX, fY, length);
}


static inline float getLengthSquared(float dx, float dy) {
    return dx * dx + dy * dy;
}







static inline bool isLengthNearlyZero(float dx, float dy,
                                      float *lengthSquared) {
    *lengthSquared = getLengthSquared(dx, dy);
    return *lengthSquared <= (SK_ScalarNearlyZero * SK_ScalarNearlyZero);
}

SkScalar SkPoint::Normalize(SkPoint* pt) {
    float x = pt->fX;
    float y = pt->fY;
    float mag2;
    if (isLengthNearlyZero(x, y, &mag2)) {
        return 0;
    }

    float mag, scale;
    if (SkScalarIsFinite(mag2)) {
        mag = sk_float_sqrt(mag2);
        scale = 1 / mag;
    } else {
        
        
        
        double xx = x;
        double yy = y;
        double magmag = sqrt(xx * xx + yy * yy);
        mag = (float)magmag;
        
        
        
        
        scale = (float)(1 / magmag);
    }
    pt->set(x * scale, y * scale);
    return mag;
}

SkScalar SkPoint::Length(SkScalar dx, SkScalar dy) {
    float mag2 = dx * dx + dy * dy;
    if (SkScalarIsFinite(mag2)) {
        return sk_float_sqrt(mag2);
    } else {
        double xx = dx;
        double yy = dy;
        return (float)sqrt(xx * xx + yy * yy);
    }
}









bool SkPoint::setLength(float x, float y, float length) {
    float mag2;
    if (isLengthNearlyZero(x, y, &mag2)) {
        return false;
    }

    float scale;
    if (SkScalarIsFinite(mag2)) {
        scale = length / sk_float_sqrt(mag2);
    } else {
        
        
        
        double xx = x;
        double yy = y;
    #ifdef SK_DISCARD_DENORMALIZED_FOR_SPEED
        
        
        
        double dscale = length / sqrt(xx * xx + yy * yy);
        fX = x * dscale;
        fY = y * dscale;
        return true;
    #else
        scale = (float)(length / sqrt(xx * xx + yy * yy));
    #endif
    }
    fX = x * scale;
    fY = y * scale;
    return true;
}

bool SkPoint::setLengthFast(float length) {
    return this->setLengthFast(fX, fY, length);
}

bool SkPoint::setLengthFast(float x, float y, float length) {
    float mag2;
    if (isLengthNearlyZero(x, y, &mag2)) {
        return false;
    }

    float scale;
    if (SkScalarIsFinite(mag2)) {
        scale = length * sk_float_rsqrt(mag2);  
    } else {
        
        
        
        double xx = x;
        double yy = y;
        scale = (float)(length / sqrt(xx * xx + yy * yy));
    }
    fX = x * scale;
    fY = y * scale;
    return true;
}




SkScalar SkPoint::distanceToLineBetweenSqd(const SkPoint& a,
                                           const SkPoint& b,
                                           Side* side) const {

    SkVector u = b - a;
    SkVector v = *this - a;

    SkScalar uLengthSqd = u.lengthSqd();
    SkScalar det = u.cross(v);
    if (NULL != side) {
        SkASSERT(-1 == SkPoint::kLeft_Side &&
                  0 == SkPoint::kOn_Side &&
                  1 == kRight_Side);
        *side = (Side) SkScalarSignAsInt(det);
    }
    return SkScalarMulDiv(det, det, uLengthSqd);
}

SkScalar SkPoint::distanceToLineSegmentBetweenSqd(const SkPoint& a,
                                                  const SkPoint& b) const {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    SkVector u = b - a;
    SkVector v = *this - a;

    SkScalar uLengthSqd = u.lengthSqd();
    SkScalar uDotV = SkPoint::DotProduct(u, v);

    if (uDotV <= 0) {
        return v.lengthSqd();
    } else if (uDotV > uLengthSqd) {
        return b.distanceToSqd(*this);
    } else {
        SkScalar det = u.cross(v);
        return SkScalarMulDiv(det, det, uLengthSqd);
    }
}
