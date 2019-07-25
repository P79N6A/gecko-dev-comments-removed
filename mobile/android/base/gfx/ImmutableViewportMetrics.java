




package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.graphics.RectF;






public class ImmutableViewportMetrics {

    
    
    public final float pageSizeWidth;
    public final float pageSizeHeight;
    public final float cssPageSizeWidth;
    public final float cssPageSizeHeight;
    public final float viewportRectBottom;
    public final float viewportRectLeft;
    public final float viewportRectRight;
    public final float viewportRectTop;
    public final float zoomFactor;

    public ImmutableViewportMetrics(ViewportMetrics m) {
        RectF viewportRect = m.getViewport();
        viewportRectBottom = viewportRect.bottom;
        viewportRectLeft = viewportRect.left;
        viewportRectRight = viewportRect.right;
        viewportRectTop = viewportRect.top;

        FloatSize pageSize = m.getPageSize();
        pageSizeWidth = pageSize.width;
        pageSizeHeight = pageSize.height;

        FloatSize cssPageSize = m.getCssPageSize();
        cssPageSizeWidth = cssPageSize.width;
        cssPageSizeHeight = cssPageSize.height;

        zoomFactor = m.getZoomFactor();
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

    public FloatSize getPageSize() {
        return new FloatSize(pageSizeWidth, pageSizeHeight);
    }

    public FloatSize getCssPageSize() {
        return new FloatSize(cssPageSizeWidth, cssPageSizeHeight);
    }
}
