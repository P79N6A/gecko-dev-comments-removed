





































#ifndef GFX_CONTEXT_H
#define GFX_CONTEXT_H

#include "gfxTypes.h"

#include "gfxASurface.h"
#include "gfxColor.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "gfxPattern.h"
#include "gfxPath.h"

typedef struct _cairo cairo_t;















class THEBES_API gfxContext {
    THEBES_INLINE_DECL_REFCOUNTING(gfxContext)

public:
    


    gfxContext(gfxASurface *surface);
    ~gfxContext();

    


    gfxASurface *OriginalSurface();

    





    already_AddRefed<gfxASurface> CurrentSurface(gfxFloat *dx, gfxFloat *dy);
    already_AddRefed<gfxASurface> CurrentSurface() {
        return CurrentSurface(NULL, NULL);
    }

    



    cairo_t *GetCairo() { return mCairo; }

    


    
    void Save();
    void Restore();

    



    






    void Stroke();
    




    void Fill();

    


    void NewPath();

    




    void ClosePath();

    


    void MoveTo(const gfxPoint& pt);

    


    gfxPoint CurrentPoint() const;

    




    void LineTo(const gfxPoint& pt);

    


    void CurveTo(const gfxPoint& pt1, const gfxPoint& pt2, const gfxPoint& pt3);

    






    void Arc(const gfxPoint& center, gfxFloat radius,
             gfxFloat angle1, gfxFloat angle2);

    







    void NegativeArc(const gfxPoint& center, gfxFloat radius,
                     gfxFloat angle1, gfxFloat angle2);

    
    


    void Line(const gfxPoint& start, const gfxPoint& end); 

    



    void Rectangle(const gfxRect& rect, PRBool snapToPixels = PR_FALSE);
    void Ellipse(const gfxPoint& center, const gfxSize& dimensions);
    void Polygon(const gfxPoint *points, PRUint32 numPoints);

    



    



    void Translate(const gfxPoint& pt);

    



    void Scale(gfxFloat x, gfxFloat y);

    





    void Rotate(gfxFloat angle);

    




    void Multiply(const gfxMatrix& other);

    


    void SetMatrix(const gfxMatrix& matrix);

    


    void IdentityMatrix();

    


    gfxMatrix CurrentMatrix() const;

    



    gfxPoint DeviceToUser(const gfxPoint& point) const;

    



    gfxSize DeviceToUser(const gfxSize& size) const;

    




    gfxRect DeviceToUser(const gfxRect& rect) const;

    



    gfxPoint UserToDevice(const gfxPoint& point) const;

    



    gfxSize UserToDevice(const gfxSize& size) const;

    




    gfxRect UserToDevice(const gfxRect& rect) const;

    






    PRBool UserToDevicePixelSnapped(gfxRect& rect) const;

    






    void PixelSnappedRectangleAndSetPattern(const gfxRect& rect, gfxPattern *pattern);

    



    


    void SetColor(const gfxRGBA& c);

    




    PRBool GetColor(gfxRGBA& c);

    





    void SetSource(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    


    void SetPattern(gfxPattern *pattern);

    


    already_AddRefed<gfxPattern> GetPattern();

    


    



    void Paint(gfxFloat alpha = 1.0);

    


    



    void Mask(gfxPattern *pattern);

    



    void Mask(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    



    



    void DrawSurface(gfxASurface *surface, const gfxSize& size);

    



    typedef enum {
        gfxLineSolid,
        gfxLineDashed,
        gfxLineDotted
    } gfxLineType;

    void SetDash(gfxLineType ltype);
    void SetDash(gfxFloat *dashes, int ndash, gfxFloat offset);
    

    


    void SetLineWidth(gfxFloat width);

    




    gfxFloat CurrentLineWidth() const;

    enum GraphicsLineCap {
        LINE_CAP_BUTT,
        LINE_CAP_ROUND,
        LINE_CAP_SQUARE
    };
    


    void SetLineCap(GraphicsLineCap cap);
    GraphicsLineCap CurrentLineCap() const;

    enum GraphicsLineJoin {
        LINE_JOIN_MITER,
        LINE_JOIN_ROUND,
        LINE_JOIN_BEVEL
    };
    



    void SetLineJoin(GraphicsLineJoin join);
    GraphicsLineJoin CurrentLineJoin() const;

    void SetMiterLimit(gfxFloat limit);
    gfxFloat CurrentMiterLimit() const;

    



    enum FillRule {
        FILL_RULE_WINDING,
        FILL_RULE_EVEN_ODD
    };
    void SetFillRule(FillRule rule);
    FillRule CurrentFillRule() const;

    



    
    enum GraphicsOperator {
        OPERATOR_CLEAR,
        OPERATOR_SOURCE,

        OPERATOR_OVER,
        OPERATOR_IN,
        OPERATOR_OUT,
        OPERATOR_ATOP,

        OPERATOR_DEST,
        OPERATOR_DEST_OVER,
        OPERATOR_DEST_IN,
        OPERATOR_DEST_OUT,
        OPERATOR_DEST_ATOP,

        OPERATOR_XOR,
        OPERATOR_ADD,
        OPERATOR_SATURATE
    };
    





    void SetOperator(GraphicsOperator op);
    GraphicsOperator CurrentOperator() const;

    




    enum AntialiasMode {
        MODE_ALIASED,
        MODE_COVERAGE
    };
    void SetAntialiasMode(AntialiasMode mode);
    AntialiasMode CurrentAntialiasMode() const;

    



    



    void Clip();

    



    void ResetClip();

    



    void Clip(const gfxRect& rect); 

    



    void UpdateSurfaceClip();
    
    


    void PushGroup(gfxASurface::gfxContentType content = gfxASurface::CONTENT_COLOR);
    already_AddRefed<gfxPattern> PopGroup();
    void PopGroupToSource();

    


    PRBool PointInFill(const gfxPoint& pt);
    PRBool PointInStroke(const gfxPoint& pt);

    


    gfxRect GetUserFillExtent();
    gfxRect GetUserStrokeExtent();

    


    already_AddRefed<gfxFlattenedPath> GetFlattenedPath();

private:
    cairo_t *mCairo;
    nsRefPtr<gfxASurface> mSurface;
};

#endif 
