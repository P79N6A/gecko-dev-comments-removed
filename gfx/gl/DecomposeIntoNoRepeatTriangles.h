





#ifndef DecomposeIntoNoRepeatTriangles_h_
#define DecomposeIntoNoRepeatTriangles_h_

#include "GLTypes.h"
#include "nsRect.h"
#include "nsTArray.h"
#include "gfx3DMatrix.h"

namespace mozilla {
namespace gl {



class RectTriangles {
public:
    RectTriangles() : mIsSimpleQuad(false) { }

    
    
    
    void addRect(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
                  GLfloat tx0, GLfloat ty0, GLfloat tx1, GLfloat ty1,
                  bool flip_y = false);

    
    
    
    
    
    bool IsSimpleQuad(gfx3DMatrix& aOutTextureTransform) const {
      aOutTextureTransform = mTextureTransform;
      return mIsSimpleQuad;
    }

    




    float* vertexPointer() {
        return &vertexCoords[0].x;
    }

    float* texCoordPointer() {
        return &texCoords[0].u;
    }

    unsigned int elements() {
        return vertexCoords.Length();
    }

    typedef struct { GLfloat x,y; } vert_coord;
    typedef struct { GLfloat u,v; } tex_coord;
private:
    
    nsAutoTArray<vert_coord, 6> vertexCoords;
    nsAutoTArray<tex_coord, 6>  texCoords;
    gfx3DMatrix mTextureTransform;
    bool mIsSimpleQuad;
};















void DecomposeIntoNoRepeatTriangles(const nsIntRect& aTexCoordRect,
                                    const nsIntSize& aTexSize,
                                    RectTriangles& aRects,
                                    bool aFlipY = false);

}
}

#endif 