






#include "SkRRect.h"



void SkRRect::setRectXY(const SkRect& rect, SkScalar xRad, SkScalar yRad) {
    if (rect.isEmpty()) {
        this->setEmpty();
        return;
    }

    if (xRad <= 0 || yRad <= 0) {
        
        this->setRect(rect);
        return;
    }

    if (rect.width() < xRad+xRad || rect.height() < yRad+yRad) {
        SkScalar scale = SkMinScalar(SkScalarDiv(rect.width(), xRad + xRad),
                                     SkScalarDiv(rect.height(), yRad + yRad));
        SkASSERT(scale < SK_Scalar1);
        xRad = SkScalarMul(xRad, scale);
        yRad = SkScalarMul(yRad, scale);
    }

    fRect = rect;
    for (int i = 0; i < 4; ++i) {
        fRadii[i].set(xRad, yRad);
    }
    fType = kSimple_Type;
    if (xRad >= SkScalarHalf(fRect.width()) && yRad >= SkScalarHalf(fRect.height())) {
        fType = kOval_Type;
        
    }

    SkDEBUGCODE(this->validate();)
}

void SkRRect::setRectRadii(const SkRect& rect, const SkVector radii[4]) {
    if (rect.isEmpty()) {
        this->setEmpty();
        return;
    }

    fRect = rect;
    memcpy(fRadii, radii, sizeof(fRadii));

    bool allCornersSquare = true;

    
    for (int i = 0; i < 4; ++i) {
        if (fRadii[i].fX <= 0 || fRadii[i].fY <= 0) {
            
            
            
            
            fRadii[i].fX = 0;
            fRadii[i].fY = 0;
        } else {
            allCornersSquare = false;
        }
    }

    if (allCornersSquare) {
        this->setRect(rect);
        return;
    }

    
    
    
    
    
    
    
    
    
    
    SkScalar scale = SK_Scalar1;

    if (fRadii[0].fX + fRadii[1].fX > rect.width()) {
        scale = SkMinScalar(scale,
                            SkScalarDiv(rect.width(), fRadii[0].fX + fRadii[1].fX));
    }
    if (fRadii[1].fY + fRadii[2].fY > rect.height()) {
        scale = SkMinScalar(scale,
                            SkScalarDiv(rect.height(), fRadii[1].fY + fRadii[2].fY));
    }
    if (fRadii[2].fX + fRadii[3].fX > rect.width()) {
        scale = SkMinScalar(scale,
                            SkScalarDiv(rect.width(), fRadii[2].fX + fRadii[3].fX));
    }
    if (fRadii[3].fY + fRadii[0].fY > rect.height()) {
        scale = SkMinScalar(scale,
                            SkScalarDiv(rect.height(), fRadii[3].fY + fRadii[0].fY));
    }

    if (scale < SK_Scalar1) {
        for (int i = 0; i < 4; ++i) {
            fRadii[i].fX = SkScalarMul(fRadii[i].fX, scale);
            fRadii[i].fY = SkScalarMul(fRadii[i].fY, scale);
        }
    }

    
    
    
    fType = (SkRRect::Type) kUnknown_Type;

    SkDEBUGCODE(this->validate();)
}

bool SkRRect::contains(SkScalar x, SkScalar y) const {
    SkDEBUGCODE(this->validate();)

    if (kEmpty_Type == this->type()) {
        return false;
    }

    if (!fRect.contains(x, y)) {
        return false;
    }

    if (kRect_Type == this->type()) {
        
        return true;
    }

    
    
    SkPoint canonicalPt; 
    int index;

    if (kOval_Type == this->type()) {
        canonicalPt.set(x - fRect.centerX(), y - fRect.centerY());
        index = kUpperLeft_Corner;  
    } else {
        if (x < fRect.fLeft + fRadii[kUpperLeft_Corner].fX &&
            y < fRect.fTop + fRadii[kUpperLeft_Corner].fY) {
            
            index = kUpperLeft_Corner;
            canonicalPt.set(x - (fRect.fLeft + fRadii[kUpperLeft_Corner].fX),
                            y - (fRect.fTop + fRadii[kUpperLeft_Corner].fY));
            SkASSERT(canonicalPt.fX < 0 && canonicalPt.fY < 0);
        } else if (x < fRect.fLeft + fRadii[kLowerLeft_Corner].fX &&
                   y > fRect.fBottom - fRadii[kLowerLeft_Corner].fY) {
            
            index = kLowerLeft_Corner;
            canonicalPt.set(x - (fRect.fLeft + fRadii[kLowerLeft_Corner].fX),
                            y - (fRect.fBottom - fRadii[kLowerLeft_Corner].fY));
            SkASSERT(canonicalPt.fX < 0 && canonicalPt.fY > 0);
        } else if (x > fRect.fRight - fRadii[kUpperRight_Corner].fX &&
                   y < fRect.fTop + fRadii[kUpperRight_Corner].fY) {
            
            index = kUpperRight_Corner;
            canonicalPt.set(x - (fRect.fRight - fRadii[kUpperRight_Corner].fX),
                            y - (fRect.fTop + fRadii[kUpperRight_Corner].fY));
            SkASSERT(canonicalPt.fX > 0 && canonicalPt.fY < 0);
        } else if (x > fRect.fRight - fRadii[kLowerRight_Corner].fX &&
                   y > fRect.fBottom - fRadii[kLowerRight_Corner].fY) {
            
            index = kLowerRight_Corner;
            canonicalPt.set(x - (fRect.fRight - fRadii[kLowerRight_Corner].fX),
                            y - (fRect.fBottom - fRadii[kLowerRight_Corner].fY));
            SkASSERT(canonicalPt.fX > 0 && canonicalPt.fY > 0);
        } else {
            
            return true;
        }
    }

    
    
    
    
    SkScalar dist =  SkScalarDiv(SkScalarSquare(canonicalPt.fX), SkScalarSquare(fRadii[index].fX)) +
                     SkScalarDiv(SkScalarSquare(canonicalPt.fY), SkScalarSquare(fRadii[index].fY));
    return dist <= SK_Scalar1;
}


void SkRRect::computeType() const {
    SkDEBUGCODE(this->validate();)

    if (fRect.isEmpty()) {
        fType = kEmpty_Type;
        return;
    }

    bool allRadiiEqual = true; 
    bool allCornersSquare = 0 == fRadii[0].fX || 0 == fRadii[0].fY;

    for (int i = 1; i < 4; ++i) {
        if (0 != fRadii[i].fX && 0 != fRadii[i].fY) {
            
            
            allCornersSquare = false;
        }
        if (fRadii[i].fX != fRadii[i-1].fX || fRadii[i].fY != fRadii[i-1].fY) {
            allRadiiEqual = false;
        }
    }

    if (allCornersSquare) {
        fType = kRect_Type;
        return;
    }

    if (allRadiiEqual) {
        if (fRadii[0].fX >= SkScalarHalf(fRect.width()) &&
            fRadii[0].fY >= SkScalarHalf(fRect.height())) {
            fType = kOval_Type;
        } else {
            fType = kSimple_Type;
        }
        return;
    }

    fType = kComplex_Type;
}



void SkRRect::inset(SkScalar dx, SkScalar dy, SkRRect* dst) const {
    SkRect r = fRect;

    r.inset(dx, dy);
    if (r.isEmpty()) {
        dst->setEmpty();
        return;
    }

    SkVector radii[4];
    memcpy(radii, fRadii, sizeof(radii));
    for (int i = 0; i < 4; ++i) {
        if (radii[i].fX) {
            radii[i].fX -= dx;
        }
        if (radii[i].fY) {
            radii[i].fY -= dy;
        }
    }
    dst->setRectRadii(r, radii);
}



uint32_t SkRRect::writeToMemory(void* buffer) const {
    SkASSERT(kSizeInMemory == sizeof(SkRect) + sizeof(fRadii));

    memcpy(buffer, &fRect, sizeof(SkRect));
    memcpy((char*)buffer + sizeof(SkRect), fRadii, sizeof(fRadii));
    return kSizeInMemory;
}

uint32_t SkRRect::readFromMemory(const void* buffer) {
    SkScalar storage[12];
    SkASSERT(sizeof(storage) == kSizeInMemory);

    
    memcpy(storage, buffer, kSizeInMemory);

    this->setRectRadii(*(const SkRect*)&storage[0],
                       (const SkVector*)&storage[4]);
    return kSizeInMemory;
}



#ifdef SK_DEBUG
void SkRRect::validate() const {
    bool allRadiiZero = (0 == fRadii[0].fX && 0 == fRadii[0].fY);
    bool allCornersSquare = (0 == fRadii[0].fX || 0 == fRadii[0].fY);
    bool allRadiiSame = true;

    for (int i = 1; i < 4; ++i) {
        if (0 != fRadii[i].fX || 0 != fRadii[i].fY) {
            allRadiiZero = false;
        }

        if (fRadii[i].fX != fRadii[i-1].fX || fRadii[i].fY != fRadii[i-1].fY) {
            allRadiiSame = false;
        }

        if (0 != fRadii[i].fX && 0 != fRadii[i].fY) {
            allCornersSquare = false;
        }
    }

    switch (fType) {
        case kEmpty_Type:
            SkASSERT(fRect.isEmpty());
            SkASSERT(allRadiiZero && allRadiiSame && allCornersSquare);

            SkASSERT(0 == fRect.fLeft && 0 == fRect.fTop &&
                     0 == fRect.fRight && 0 == fRect.fBottom);
            break;
        case kRect_Type:
            SkASSERT(!fRect.isEmpty());
            SkASSERT(allRadiiZero && allRadiiSame && allCornersSquare);
            break;
        case kOval_Type:
            SkASSERT(!fRect.isEmpty());
            SkASSERT(!allRadiiZero && allRadiiSame && !allCornersSquare);

            for (int i = 0; i < 4; ++i) {
                SkASSERT(SkScalarNearlyEqual(fRadii[i].fX, SkScalarHalf(fRect.width())));
                SkASSERT(SkScalarNearlyEqual(fRadii[i].fY, SkScalarHalf(fRect.height())));
            }
            break;
        case kSimple_Type:
            SkASSERT(!fRect.isEmpty());
            SkASSERT(!allRadiiZero && allRadiiSame && !allCornersSquare);
            break;
        case kComplex_Type:
            SkASSERT(!fRect.isEmpty());
            SkASSERT(!allRadiiZero && !allRadiiSame && !allCornersSquare);
            break;
        case kUnknown_Type:
            
            break;
    }
}
#endif 


