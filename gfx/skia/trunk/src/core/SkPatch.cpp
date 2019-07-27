






#include "SkPatch.h"

#include "SkGeometry.h"
#include "SkColorPriv.h"




















class FwDCubicEvaluator {
    
public:
    FwDCubicEvaluator() { }
    
    


    FwDCubicEvaluator(SkPoint a, SkPoint b, SkPoint c, SkPoint d) {
        fPoints[0] = a;
        fPoints[1] = b;
        fPoints[2] = c;
        fPoints[3] = d;
        
        SkScalar cx[4], cy[4];
        SkGetCubicCoeff(fPoints, cx, cy);
        fCoefs[0].set(cx[0], cy[0]);
        fCoefs[1].set(cx[1], cy[1]);
        fCoefs[2].set(cx[2], cy[2]);
        fCoefs[3].set(cx[3], cy[3]);
        
        this->restart(1);
    }
    
    


    void restart(int divisions) {
        fDivisions = divisions;
        SkScalar h  = 1.f / fDivisions;
        fCurrent    = 0;
        fMax        = fDivisions + 1;
        fFwDiff[0]  = fCoefs[3];
        SkScalar h2 = h * h;
        SkScalar h3 = h2 * h;
        
        fFwDiff[3].set(6.f * fCoefs[0].x() * h3, 6.f * fCoefs[0].y() * h3); 
        fFwDiff[2].set(fFwDiff[3].x() + 2.f * fCoefs[1].x() * h2, 
                       fFwDiff[3].y() + 2.f * fCoefs[1].y() * h2);
        fFwDiff[1].set(fCoefs[0].x() * h3 + fCoefs[1].x() * h2 + fCoefs[2].x() * h,
                       fCoefs[0].y() * h3 + fCoefs[1].y() * h2 + fCoefs[2].y() * h);
    }
    
    


    bool done() const {
        return fCurrent > fMax;
    }
    
    


    SkPoint next() {
        SkPoint point = fFwDiff[0];
        fFwDiff[0]    += fFwDiff[1];
        fFwDiff[1]    += fFwDiff[2];
        fFwDiff[2]    += fFwDiff[3];
        fCurrent++;
        return point;
    }
    
    const SkPoint* getCtrlPoints() const {
        return fPoints;
    }
    
private:
    int fMax, fCurrent, fDivisions;
    SkPoint fFwDiff[4], fCoefs[4], fPoints[4];
};



SkPatch::SkPatch(SkPoint points[12], SkColor colors[4]) {
        
    for (int i = 0; i<12; i++) {
        fCtrlPoints[i] = points[i];
    }
    
    fCornerColors[0] = SkPreMultiplyColor(colors[0]);
    fCornerColors[1] = SkPreMultiplyColor(colors[1]);
    fCornerColors[2] = SkPreMultiplyColor(colors[2]);
    fCornerColors[3] = SkPreMultiplyColor(colors[3]);
}

uint8_t bilinear(SkScalar tx, SkScalar ty, SkScalar c00, SkScalar c10, SkScalar c01, SkScalar c11) {
    SkScalar a = c00 * (1.f - tx) + c10 * tx;
    SkScalar b = c01 * (1.f - tx) + c11 * tx;
    return uint8_t(a * (1.f - ty) + b * ty);
}

bool SkPatch::getVertexData(SkPatch::VertexData* data, int divisions) {
    
    if (divisions < 1) {
        return false;
    }
    
    int divX = divisions, divY = divisions;
    
    data->fVertexCount = (divX + 1) * (divY + 1);
    data->fIndexCount = divX * divY * 6;

    data->fPoints = SkNEW_ARRAY(SkPoint, data->fVertexCount);
    data->fColors = SkNEW_ARRAY(uint32_t, data->fVertexCount);
    data->fTexCoords = SkNEW_ARRAY(SkPoint, data->fVertexCount);
    data->fIndices = SkNEW_ARRAY(uint16_t, data->fIndexCount);
    
    FwDCubicEvaluator fBottom(fCtrlPoints[kBottomP0_CubicCtrlPts],
                              fCtrlPoints[kBottomP1_CubicCtrlPts],
                              fCtrlPoints[kBottomP2_CubicCtrlPts],
                              fCtrlPoints[kBottomP3_CubicCtrlPts]),
                        fTop(fCtrlPoints[kTopP0_CubicCtrlPts],
                             fCtrlPoints[kTopP1_CubicCtrlPts],
                             fCtrlPoints[kTopP2_CubicCtrlPts],
                             fCtrlPoints[kTopP2_CubicCtrlPts]),
                        fLeft(fCtrlPoints[kLeftP0_CubicCtrlPts],
                              fCtrlPoints[kLeftP1_CubicCtrlPts],
                              fCtrlPoints[kLeftP2_CubicCtrlPts],
                              fCtrlPoints[kLeftP3_CubicCtrlPts]),
                        fRight(fCtrlPoints[kRightP0_CubicCtrlPts],
                               fCtrlPoints[kRightP1_CubicCtrlPts],
                               fCtrlPoints[kRightP2_CubicCtrlPts],
                               fCtrlPoints[kRightP3_CubicCtrlPts]);

    fBottom.restart(divX);
    fTop.restart(divX);

    SkScalar u = 0.0f;
    int stride = divY + 1;
    for (int x = 0; x <= divX; x++) {
        SkPoint bottom = fBottom.next(), top = fTop.next();
        fLeft.restart(divY);
        fRight.restart(divY);
        SkScalar v = 0.f;
        for (int y = 0; y <= divY; y++) {
            int dataIndex = x * (divX + 1) + y;

            SkPoint left = fLeft.next(), right = fRight.next();

            SkPoint s0 = SkPoint::Make((1.0f - v) * top.x() + v * bottom.x(),
                                       (1.0f - v) * top.y() + v * bottom.y());
            SkPoint s1 = SkPoint::Make((1.0f - u) * left.x() + u * right.x(),
                                       (1.0f - u) * left.y() + u * right.y());
            SkPoint s2 = SkPoint::Make(
                                       (1.0f - v) * ((1.0f - u) * fTop.getCtrlPoints()[0].x()
                                        + u * fTop.getCtrlPoints()[3].x())
                                        + v * ((1.0f - u) * fBottom.getCtrlPoints()[0].x()
                                        + u * fBottom.getCtrlPoints()[3].x()),
                                       (1.0f - v) * ((1.0f - u) * fTop.getCtrlPoints()[0].y()
                                        + u * fTop.getCtrlPoints()[3].y())
                                        + v * ((1.0f - u) * fBottom.getCtrlPoints()[0].y()
                                        + u * fBottom.getCtrlPoints()[3].y()));
            data->fPoints[dataIndex] = s0 + s1 - s2;

            uint8_t a = bilinear(u, v, 
                            SkScalar(SkColorGetA(fCornerColors[0])), 
                            SkScalar(SkColorGetA(fCornerColors[1])), 
                            SkScalar(SkColorGetA(fCornerColors[2])), 
                            SkScalar(SkColorGetA(fCornerColors[3])));
            uint8_t r = bilinear(u, v, 
                            SkScalar(SkColorGetR(fCornerColors[0])), 
                            SkScalar(SkColorGetR(fCornerColors[1])), 
                            SkScalar(SkColorGetR(fCornerColors[2])), 
                            SkScalar(SkColorGetR(fCornerColors[3])));
            uint8_t g = bilinear(u, v, 
                            SkScalar(SkColorGetG(fCornerColors[0])), 
                            SkScalar(SkColorGetG(fCornerColors[1])), 
                            SkScalar(SkColorGetG(fCornerColors[2])), 
                            SkScalar(SkColorGetG(fCornerColors[3])));
            uint8_t b = bilinear(u, v, 
                            SkScalar(SkColorGetB(fCornerColors[0])), 
                            SkScalar(SkColorGetB(fCornerColors[1])), 
                            SkScalar(SkColorGetB(fCornerColors[2])), 
                            SkScalar(SkColorGetB(fCornerColors[3])));
            data->fColors[dataIndex] = SkPackARGB32(a,r,g,b);
            
            data->fTexCoords[dataIndex] = SkPoint::Make(u, v);

            if(x < divX && y < divY) {
                int i = 6 * (x * divY + y);
                data->fIndices[i] = x * stride + y;
                data->fIndices[i + 1] = x * stride + 1 + y;
                data->fIndices[i + 2] = (x + 1) * stride + 1 + y;
                data->fIndices[i + 3] = data->fIndices[i];
                data->fIndices[i + 4] = data->fIndices[i + 2];
                data->fIndices[i + 5] = (x + 1) * stride + y;
            }
            v += 1.f / divY;
        }
        u += 1.f / divX;
    }
    return true;
}
