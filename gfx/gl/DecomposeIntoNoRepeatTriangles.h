





#ifndef DecomposeIntoNoRepeatTriangles_h_
#define DecomposeIntoNoRepeatTriangles_h_

#include "GLTypes.h"
#include "nsRect.h"
#include "nsTArray.h"

namespace mozilla {
namespace gl {



class RectTriangles {
public:
    typedef struct { GLfloat x,y; } coord;

    
    
    
    void addRect(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
                 GLfloat tx0, GLfloat ty0, GLfloat tx1, GLfloat ty1,
                 bool flip_y = false);

    




    InfallibleTArray<coord>& vertCoords() {
        return mVertexCoords;
    }

    InfallibleTArray<coord>& texCoords() {
        return mTexCoords;
    }

    unsigned int elements() {
        return mVertexCoords.Length();
    }
private:
    
    nsAutoTArray<coord, 6> mVertexCoords;
    nsAutoTArray<coord, 6> mTexCoords;

    static void
    AppendRectToCoordArray(InfallibleTArray<coord>& array, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1);
};















void DecomposeIntoNoRepeatTriangles(const gfx::IntRect& aTexCoordRect,
                                    const gfx::IntSize& aTexSize,
                                    RectTriangles& aRects,
                                    bool aFlipY = false);

} 
} 

#endif 
