









#ifndef GrGpuVertex_DEFINED
#define GrGpuVertex_DEFINED

#include "GrGLConfig.h"
#include "GrPoint.h"

#if GR_TEXT_SCALAR_IS_USHORT
    typedef uint16_t                GrTextScalar;  
    #define GrIntToTextScalar(x)    ((uint16_t)x)
    #define GrFixedToTextScalar(x)  (x)
#elif GR_TEXT_SCALAR_IS_FIXED
    typedef GrFixed                 GrTextScalar;
    #define GrIntToTextScalar(x)    GrIntToFixed(x)
    #define GrFixedToTextScalar(x)  (x)
#elif GR_TEXT_SCALAR_IS_FLOAT
    typedef float                   GrTextScalar;    
    #define GrIntToTextScalar(x)    ((GrTextScalar)x)
    #define GrFixedToTextScalar(x)  GrFixedToFloat(x)
#else
    #error "Text scalar type not defined"
#endif



struct GrGpuTextVertex {
    GrTextScalar fX;
    GrTextScalar fY;

    void set(GrTextScalar x, GrTextScalar y) {
        fX = x;
        fY = y;
    }

    void setI(int x, int y) {
        fX = GrIntToTextScalar(x);
        fY = GrIntToTextScalar(y);
    }
    
    void setX(GrFixed x, GrFixed y) {
        fX = GrFixedToTextScalar(x);
        fY = GrFixedToTextScalar(y);
    }
    
    

    void setRectFan(GrTextScalar l, GrTextScalar t, GrTextScalar r,
                    GrTextScalar b) {
        GrGpuTextVertex* v = this;
        v[0].set(l, t);
        v[1].set(l, b);
        v[2].set(r, b);
        v[3].set(r, t);
    }

    void setIRectFan(int l, int t, int r, int b) {
        this->setRectFan(GrIntToTextScalar(l), GrIntToTextScalar(t),
                         GrIntToTextScalar(r), GrIntToTextScalar(b));
    }

    void setIRectFan(int l, int t, int r, int b, size_t stride) {
        GrAssert(stride > sizeof(GrGpuTextVertex));
        char* v = (char*)this;
        ((GrGpuTextVertex*)(v + 0*stride))->setI(l, t);
        ((GrGpuTextVertex*)(v + 1*stride))->setI(l, b);
        ((GrGpuTextVertex*)(v + 2*stride))->setI(r, b);
        ((GrGpuTextVertex*)(v + 3*stride))->setI(r, t);
    }

    
    void setXRectFan(GrFixed l, GrFixed t, GrFixed r, GrFixed b) {
        this->setRectFan(GrFixedToTextScalar(l), GrFixedToTextScalar(t),
                         GrFixedToTextScalar(r), GrFixedToTextScalar(b));
    }

    void setXRectFan(GrFixed l, GrFixed t, GrFixed r, GrFixed b, size_t stride) {
        GrAssert(stride > sizeof(GrGpuTextVertex));
        char* v = (char*)this;
        ((GrGpuTextVertex*)(v + 0*stride))->setX(l, t);
        ((GrGpuTextVertex*)(v + 1*stride))->setX(l, b);
        ((GrGpuTextVertex*)(v + 2*stride))->setX(r, b);
        ((GrGpuTextVertex*)(v + 3*stride))->setX(r, t);
    }

};

#endif

