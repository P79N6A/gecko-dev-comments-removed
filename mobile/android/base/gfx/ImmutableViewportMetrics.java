




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.util.FloatUtils;

import android.graphics.PointF;
import android.graphics.RectF;






public class ImmutableViewportMetrics {

    
    
    public final float pageRectLeft;
    public final float pageRectTop;
    public final float pageRectRight;
    public final float pageRectBottom;
    public final float cssPageRectLeft;
    public final float cssPageRectTop;
    public final float cssPageRectRight;
    public final float cssPageRectBottom;
    public final float viewportRectLeft;
    public final float viewportRectTop;
    public final float viewportRectRight;
    public final float viewportRectBottom;
    public final float zoomFactor;

    public ImmutableViewportMetrics(ViewportMetrics m) {
        RectF viewportRect = m.getViewport();
        viewportRectLeft = viewportRect.left;
        viewportRectTop = viewportRect.top;
        viewportRectRight = viewportRect.right;
        viewportRectBottom = viewportRect.bottom;

        RectF pageRect = m.getPageRect();
        pageRectLeft = pageRect.left;
        pageRectTop = pageRect.top;
        pageRectRight = pageRect.right;
        pageRectBottom = pageRect.bottom;

        RectF cssPageRect = m.getCssPageRect();
        cssPageRectLeft = cssPageRect.left;
        cssPageRectTop = cssPageRect.top;
        cssPageRectRight = cssPageRect.right;
        cssPageRectBottom = cssPageRect.bottom;

        zoomFactor = m.getZoomFactor();
    }

    private ImmutableViewportMetrics(float aPageRectLeft, float aPageRectTop,
        float aPageRectRight, float aPageRectBottom, float aCssPageRectLeft,
        float aCssPageRectTop, float aCssPageRectRight, float aCssPageRectBottom,
        float aViewportRectLeft, float aViewportRectTop, float aViewportRectRight,
        float aViewportRectBottom, float aZoomFactor)
    {
        pageRectLeft = aPageRectLeft;
        pageRectTop = aPageRectTop;
        pageRectRight = aPageRectRight;
        pageRectBottom = aPageRectBottom;
        cssPageRectLeft = aCssPageRectLeft;
        cssPageRectTop = aCssPageRectTop;
        cssPageRectRight = aCssPageRectRight;
        cssPageRectBottom = aCssPageRectBottom;
        viewportRectLeft = aViewportRectLeft;
        viewportRectTop = aViewportRectTop;
        viewportRectRight = aViewportRectRight;
        viewportRectBottom = aViewportRectBottom;
        zoomFactor = aZoomFactor;
    }

    public float getWidth() {
        return viewportRectRight - viewportRectLeft;
    }

    public float getHeight() {
        return viewportRectBottom - viewportRectTop;
    }

    

    public PointF getOrigin() {
        return new PointF(viewportRectLeft, viewportRectTop);
    }

    public FloatSize getSize() {
        return new FloatSize(viewportRectRight - viewportRectLeft, viewportRectBottom - viewportRectTop);
    }

    public RectF getViewport() {
        return new RectF(viewportRectLeft,
                         viewportRectTop,
                         viewportRectRight,
                         viewportRectBottom);
    }

    public RectF getCssViewport() {
        return RectUtils.scale(getViewport(), 1/zoomFactor);
    }

    public RectF getPageRect() {
        return new RectF(pageRectLeft, pageRectTop, pageRectRight, pageRectBottom);
    }

    public float getPageWidth() {
        return pageRectRight - pageRectLeft;
    }

    public float getPageHeight() {
        return pageRectBottom - pageRectTop;
    }

    public RectF getCssPageRect() {
        return new RectF(cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom);
    }

    




    public ImmutableViewportMetrics interpolate(ImmutableViewportMetrics to, float t) {
        return new ImmutableViewportMetrics(
            FloatUtils.interpolate(pageRectLeft, to.pageRectLeft, t),
            FloatUtils.interpolate(pageRectTop, to.pageRectTop, t),
            FloatUtils.interpolate(pageRectRight, to.pageRectRight, t),
            FloatUtils.interpolate(pageRectBottom, to.pageRectBottom, t),
            FloatUtils.interpolate(cssPageRectLeft, to.cssPageRectLeft, t),
            FloatUtils.interpolate(cssPageRectTop, to.cssPageRectTop, t),
            FloatUtils.interpolate(cssPageRectRight, to.cssPageRectRight, t),
            FloatUtils.interpolate(cssPageRectBottom, to.cssPageRectBottom, t),
            FloatUtils.interpolate(viewportRectLeft, to.viewportRectLeft, t),
            FloatUtils.interpolate(viewportRectTop, to.viewportRectTop, t),
            FloatUtils.interpolate(viewportRectRight, to.viewportRectRight, t),
            FloatUtils.interpolate(viewportRectBottom, to.viewportRectBottom, t),
            FloatUtils.interpolate(zoomFactor, to.zoomFactor, t));
    }

    @Override
    public String toString() {
        return "ImmutableViewportMetrics v=(" + viewportRectLeft + "," + viewportRectTop + ","
                + viewportRectRight + "," + viewportRectBottom + ") p=(" + pageRectLeft + ","
                + pageRectTop + "," + pageRectRight + "," + pageRectBottom + ") c=("
                + cssPageRectLeft + "," + cssPageRectTop + "," + cssPageRectRight + ","
                + cssPageRectBottom + ") z=" + zoomFactor;
    }
}
