





#include "DecomposeIntoNoRepeatTriangles.h"

namespace mozilla {
namespace gl {

void
RectTriangles::addRect(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
                       GLfloat tx0, GLfloat ty0, GLfloat tx1, GLfloat ty1,
                       bool flip_y )
{
    vert_coord v;
    v.x = x0; v.y = y0;
    vertexCoords.AppendElement(v);
    v.x = x1; v.y = y0;
    vertexCoords.AppendElement(v);
    v.x = x0; v.y = y1;
    vertexCoords.AppendElement(v);

    v.x = x0; v.y = y1;
    vertexCoords.AppendElement(v);
    v.x = x1; v.y = y0;
    vertexCoords.AppendElement(v);
    v.x = x1; v.y = y1;
    vertexCoords.AppendElement(v);

    if (flip_y) {
        tex_coord t;
        t.u = tx0; t.v = ty1;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty1;
        texCoords.AppendElement(t);
        t.u = tx0; t.v = ty0;
        texCoords.AppendElement(t);

        t.u = tx0; t.v = ty0;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty1;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty0;
        texCoords.AppendElement(t);
    } else {
        tex_coord t;
        t.u = tx0; t.v = ty0;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty0;
        texCoords.AppendElement(t);
        t.u = tx0; t.v = ty1;
        texCoords.AppendElement(t);

        t.u = tx0; t.v = ty1;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty0;
        texCoords.AppendElement(t);
        t.u = tx1; t.v = ty1;
        texCoords.AppendElement(t);
    }
}

static GLfloat
WrapTexCoord(GLfloat v)
{
    
    
    
    
    if (v < 0.0f) {
        return 1.0f + fmodf(v, 1.0f);
    }

    return fmodf(v, 1.0f);
}

void
DecomposeIntoNoRepeatTriangles(const nsIntRect& aTexCoordRect,
                               const nsIntSize& aTexSize,
                               RectTriangles& aRects,
                               bool aFlipY )
{
    
    nsIntRect tcr(aTexCoordRect);
    while (tcr.x >= aTexSize.width)
        tcr.x -= aTexSize.width;
    while (tcr.y >= aTexSize.height)
        tcr.y -= aTexSize.height;

    
    GLfloat tl[2] =
        { GLfloat(tcr.x) / GLfloat(aTexSize.width),
          GLfloat(tcr.y) / GLfloat(aTexSize.height) };
    GLfloat br[2] =
        { GLfloat(tcr.XMost()) / GLfloat(aTexSize.width),
          GLfloat(tcr.YMost()) / GLfloat(aTexSize.height) };

    
    
    

    bool xwrap = false, ywrap = false;
    if (tcr.x < 0 || tcr.x > aTexSize.width ||
        tcr.XMost() < 0 || tcr.XMost() > aTexSize.width)
    {
        xwrap = true;
        tl[0] = WrapTexCoord(tl[0]);
        br[0] = WrapTexCoord(br[0]);
    }

    if (tcr.y < 0 || tcr.y > aTexSize.height ||
        tcr.YMost() < 0 || tcr.YMost() > aTexSize.height)
    {
        ywrap = true;
        tl[1] = WrapTexCoord(tl[1]);
        br[1] = WrapTexCoord(br[1]);
    }

    NS_ASSERTION(tl[0] >= 0.0f && tl[0] <= 1.0f &&
                 tl[1] >= 0.0f && tl[1] <= 1.0f &&
                 br[0] >= 0.0f && br[0] <= 1.0f &&
                 br[1] >= 0.0f && br[1] <= 1.0f,
                 "Somehow generated invalid texture coordinates");

    
    
    
    
    

    
    
    
    
    
    
    
    GLfloat xlen = (1.0f - tl[0]) + br[0];
    GLfloat ylen = (1.0f - tl[1]) + br[1];

    NS_ASSERTION(!xwrap || xlen > 0.0f, "xlen isn't > 0, what's going on?");
    NS_ASSERTION(!ywrap || ylen > 0.0f, "ylen isn't > 0, what's going on?");
    NS_ASSERTION(aTexCoordRect.width <= aTexSize.width &&
                 aTexCoordRect.height <= aTexSize.height, "tex coord rect would cause tiling!");

    if (!xwrap && !ywrap) {
        aRects.addRect(0.0f, 0.0f,
                       1.0f, 1.0f,
                       tl[0], tl[1],
                       br[0], br[1],
                       aFlipY);
    } else if (!xwrap && ywrap) {
        GLfloat ymid = (1.0f - tl[1]) / ylen;
        aRects.addRect(0.0f, 0.0f,
                       1.0f, ymid,
                       tl[0], tl[1],
                       br[0], 1.0f,
                       aFlipY);
        aRects.addRect(0.0f, ymid,
                       1.0f, 1.0f,
                       tl[0], 0.0f,
                       br[0], br[1],
                       aFlipY);
    } else if (xwrap && !ywrap) {
        GLfloat xmid = (1.0f - tl[0]) / xlen;
        aRects.addRect(0.0f, 0.0f,
                       xmid, 1.0f,
                       tl[0], tl[1],
                       1.0f, br[1],
                       aFlipY);
        aRects.addRect(xmid, 0.0f,
                       1.0f, 1.0f,
                       0.0f, tl[1],
                       br[0], br[1],
                       aFlipY);
    } else {
        GLfloat xmid = (1.0f - tl[0]) / xlen;
        GLfloat ymid = (1.0f - tl[1]) / ylen;
        aRects.addRect(0.0f, 0.0f,
                       xmid, ymid,
                       tl[0], tl[1],
                       1.0f, 1.0f,
                       aFlipY);
        aRects.addRect(xmid, 0.0f,
                       1.0f, ymid,
                       0.0f, tl[1],
                       br[0], 1.0f,
                       aFlipY);
        aRects.addRect(0.0f, ymid,
                       xmid, 1.0f,
                       tl[0], 0.0f,
                       1.0f, br[1],
                       aFlipY);
        aRects.addRect(xmid, ymid,
                       1.0f, 1.0f,
                       0.0f, 0.0f,
                       br[0], br[1],
                       aFlipY);
    }
}

}
}