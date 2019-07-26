




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.util.FloatUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.util.DisplayMetrics;






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
    public final float fixedLayerMarginLeft;
    public final float fixedLayerMarginTop;
    public final float fixedLayerMarginRight;
    public final float fixedLayerMarginBottom;
    public final float zoomFactor;

    public ImmutableViewportMetrics(DisplayMetrics metrics) {
        viewportRectLeft   = pageRectLeft   = cssPageRectLeft   = 0;
        viewportRectTop    = pageRectTop    = cssPageRectTop    = 0;
        viewportRectRight  = pageRectRight  = cssPageRectRight  = metrics.widthPixels;
        viewportRectBottom = pageRectBottom = cssPageRectBottom = metrics.heightPixels;
        fixedLayerMarginLeft = fixedLayerMarginTop = fixedLayerMarginRight = fixedLayerMarginBottom = 0;
        zoomFactor = 1.0f;
    }

    private ImmutableViewportMetrics(float aPageRectLeft, float aPageRectTop,
        float aPageRectRight, float aPageRectBottom, float aCssPageRectLeft,
        float aCssPageRectTop, float aCssPageRectRight, float aCssPageRectBottom,
        float aViewportRectLeft, float aViewportRectTop, float aViewportRectRight,
        float aViewportRectBottom, float aZoomFactor)
    {
        this(aPageRectLeft, aPageRectTop,
             aPageRectRight, aPageRectBottom, aCssPageRectLeft,
             aCssPageRectTop, aCssPageRectRight, aCssPageRectBottom,
             aViewportRectLeft, aViewportRectTop, aViewportRectRight,
             aViewportRectBottom, 0.0f, 0.0f, 0.0f, 0.0f, aZoomFactor);
    }

    private ImmutableViewportMetrics(float aPageRectLeft, float aPageRectTop,
        float aPageRectRight, float aPageRectBottom, float aCssPageRectLeft,
        float aCssPageRectTop, float aCssPageRectRight, float aCssPageRectBottom,
        float aViewportRectLeft, float aViewportRectTop, float aViewportRectRight,
        float aViewportRectBottom, float aFixedLayerMarginLeft,
        float aFixedLayerMarginTop, float aFixedLayerMarginRight,
        float aFixedLayerMarginBottom, float aZoomFactor)
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
        fixedLayerMarginLeft = aFixedLayerMarginLeft;
        fixedLayerMarginTop = aFixedLayerMarginTop;
        fixedLayerMarginRight = aFixedLayerMarginRight;
        fixedLayerMarginBottom = aFixedLayerMarginBottom;
        zoomFactor = aZoomFactor;
    }

    public float getWidth() {
        return viewportRectRight - viewportRectLeft;
    }

    public float getHeight() {
        return viewportRectBottom - viewportRectTop;
    }

    public float getWidthWithoutMargins() {
        return viewportRectRight - viewportRectLeft - fixedLayerMarginLeft - fixedLayerMarginRight;
    }

    public float getHeightWithoutMargins() {
        return viewportRectBottom - viewportRectTop - fixedLayerMarginTop - fixedLayerMarginBottom;
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
            FloatUtils.interpolate(fixedLayerMarginLeft, to.fixedLayerMarginLeft, t),
            FloatUtils.interpolate(fixedLayerMarginTop, to.fixedLayerMarginTop, t),
            FloatUtils.interpolate(fixedLayerMarginRight, to.fixedLayerMarginRight, t),
            FloatUtils.interpolate(fixedLayerMarginBottom, to.fixedLayerMarginBottom, t),
            FloatUtils.interpolate(zoomFactor, to.zoomFactor, t));
    }

    public ImmutableViewportMetrics setViewportSize(float width, float height) {
        if (FloatUtils.fuzzyEquals(width, getWidth()) && FloatUtils.fuzzyEquals(height, getHeight())) {
            return this;
        }

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectLeft + width, viewportRectTop + height,
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            zoomFactor);
    }

    public ImmutableViewportMetrics setViewportOrigin(float newOriginX, float newOriginY) {
        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            newOriginX, newOriginY, newOriginX + getWidth(), newOriginY + getHeight(),
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            zoomFactor);
    }

    public ImmutableViewportMetrics setZoomFactor(float newZoomFactor) {
        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            newZoomFactor);
    }

    public ImmutableViewportMetrics offsetViewportBy(float dx, float dy) {
        return setViewportOrigin(viewportRectLeft + dx, viewportRectTop + dy);
    }

    public ImmutableViewportMetrics setPageRect(RectF pageRect, RectF cssPageRect) {
        return new ImmutableViewportMetrics(
            pageRect.left, pageRect.top, pageRect.right, pageRect.bottom,
            cssPageRect.left, cssPageRect.top, cssPageRect.right, cssPageRect.bottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            zoomFactor);
    }

    public ImmutableViewportMetrics setFixedLayerMargins(float left, float top, float right, float bottom) {
        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            left, top, right, bottom, zoomFactor);
    }

    public ImmutableViewportMetrics setFixedLayerMarginsFrom(ImmutableViewportMetrics fromMetrics) {
        return setFixedLayerMargins(fromMetrics.fixedLayerMarginLeft,
                                    fromMetrics.fixedLayerMarginTop,
                                    fromMetrics.fixedLayerMarginRight,
                                    fromMetrics.fixedLayerMarginBottom);
    }

    



    public ImmutableViewportMetrics scaleTo(float newZoomFactor, PointF focus) {
        
        
        float newPageRectLeft = cssPageRectLeft * newZoomFactor;
        float newPageRectTop = cssPageRectTop * newZoomFactor;
        float newPageRectRight = cssPageRectLeft + ((cssPageRectRight - cssPageRectLeft) * newZoomFactor);
        float newPageRectBottom = cssPageRectTop + ((cssPageRectBottom - cssPageRectTop) * newZoomFactor);

        PointF origin = getOrigin();
        origin.offset(focus.x, focus.y);
        origin = PointUtils.scale(origin, newZoomFactor / zoomFactor);
        origin.offset(-focus.x, -focus.y);

        return new ImmutableViewportMetrics(
            newPageRectLeft, newPageRectTop, newPageRectRight, newPageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            origin.x, origin.y, origin.x + getWidth(), origin.y + getHeight(),
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            newZoomFactor);
    }

    
    private ImmutableViewportMetrics clamp(float marginLeft, float marginTop,
                                           float marginRight, float marginBottom) {
        RectF newViewport = getViewport();

        
        if (newViewport.right > pageRectRight + marginRight)
            newViewport.offset((pageRectRight + marginRight) - newViewport.right, 0);
        if (newViewport.left < pageRectLeft - marginLeft)
            newViewport.offset((pageRectLeft - marginLeft) - newViewport.left, 0);

        if (newViewport.bottom > pageRectBottom + marginBottom)
            newViewport.offset(0, (pageRectBottom + marginBottom) - newViewport.bottom);
        if (newViewport.top < pageRectTop - marginTop)
            newViewport.offset(0, (pageRectTop - marginTop) - newViewport.top);

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            newViewport.left, newViewport.top, newViewport.right, newViewport.bottom,
            fixedLayerMarginLeft, fixedLayerMarginTop, fixedLayerMarginRight, fixedLayerMarginBottom,
            zoomFactor);
    }

    public ImmutableViewportMetrics clamp() {
        return clamp(0, 0, 0, 0);
    }

    public ImmutableViewportMetrics clampWithMargins() {
        return clamp(fixedLayerMarginLeft, fixedLayerMarginTop,
                     fixedLayerMarginRight, fixedLayerMarginBottom);
    }

    public boolean fuzzyEquals(ImmutableViewportMetrics other) {
        
        
        
        
        
        
        
        return FloatUtils.fuzzyEquals(cssPageRectLeft, other.cssPageRectLeft)
            && FloatUtils.fuzzyEquals(cssPageRectTop, other.cssPageRectTop)
            && FloatUtils.fuzzyEquals(cssPageRectRight, other.cssPageRectRight)
            && FloatUtils.fuzzyEquals(cssPageRectBottom, other.cssPageRectBottom)
            && FloatUtils.fuzzyEquals(viewportRectLeft, other.viewportRectLeft)
            && FloatUtils.fuzzyEquals(viewportRectTop, other.viewportRectTop)
            && FloatUtils.fuzzyEquals(viewportRectRight, other.viewportRectRight)
            && FloatUtils.fuzzyEquals(viewportRectBottom, other.viewportRectBottom)
            && FloatUtils.fuzzyEquals(zoomFactor, other.zoomFactor);
    }

    @Override
    public String toString() {
        return "ImmutableViewportMetrics v=(" + viewportRectLeft + "," + viewportRectTop + ","
                + viewportRectRight + "," + viewportRectBottom + ") p=(" + pageRectLeft + ","
                + pageRectTop + "," + pageRectRight + "," + pageRectBottom + ") c=("
                + cssPageRectLeft + "," + cssPageRectTop + "," + cssPageRectRight + ","
                + cssPageRectBottom + ") m=(" + fixedLayerMarginLeft + ","
                + fixedLayerMarginTop + "," + fixedLayerMarginRight + ","
                + fixedLayerMarginBottom + ") z=" + zoomFactor;
    }
}
