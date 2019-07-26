








#ifndef GrGLIRect_DEFINED
#define GrGLIRect_DEFINED

#include "gl/GrGLInterface.h"
#include "GrGLUtil.h"





struct GrGLIRect {
    GrGLint   fLeft;
    GrGLint   fBottom;
    GrGLsizei fWidth;
    GrGLsizei fHeight;

    void pushToGLViewport(const GrGLInterface* gl) const {
        GR_GL_CALL(gl, Viewport(fLeft, fBottom, fWidth, fHeight));
    }

    void pushToGLScissor(const GrGLInterface* gl) const {
        GR_GL_CALL(gl, Scissor(fLeft, fBottom, fWidth, fHeight));
    }

    void setFromGLViewport(const GrGLInterface* gl) {
        GR_STATIC_ASSERT(sizeof(GrGLIRect) == 4*sizeof(GrGLint));
        GR_GL_GetIntegerv(gl, GR_GL_VIEWPORT, (GrGLint*) this);
    }

    
    
    
    void setRelativeTo(const GrGLIRect& glRect,
                       int leftOffset,
                       int topOffset,
                       int width,
                       int height,
                       GrSurfaceOrigin origin) {
        fLeft = glRect.fLeft + leftOffset;
        fWidth = width;
        if (kBottomLeft_GrSurfaceOrigin == origin) {
            fBottom = glRect.fBottom + (glRect.fHeight - topOffset - height);
        } else {
            fBottom = glRect.fBottom + topOffset;
        }
        fHeight = height;

        SkASSERT(fLeft >= 0);
        SkASSERT(fWidth >= 0);
        SkASSERT(fBottom >= 0);
        SkASSERT(fHeight >= 0);
    }

    bool contains(const GrGLIRect& glRect) const {
        return fLeft <= glRect.fLeft &&
               fBottom <= glRect.fBottom &&
               fLeft + fWidth >=  glRect.fLeft + glRect.fWidth &&
               fBottom + fHeight >=  glRect.fBottom + glRect.fHeight;
    }

    void invalidate() {fLeft = fWidth = fBottom = fHeight = -1;}

    bool operator ==(const GrGLIRect& glRect) const {
        return 0 == memcmp(this, &glRect, sizeof(GrGLIRect));
    }

    bool operator !=(const GrGLIRect& glRect) const {return !(*this == glRect);}
};

#endif
