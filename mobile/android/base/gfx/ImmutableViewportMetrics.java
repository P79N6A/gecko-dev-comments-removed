




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
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
    public final float marginLeft;
    public final float marginTop;
    public final float marginRight;
    public final float marginBottom;
    public final float zoomFactor;
    public final boolean isRTL;

    public ImmutableViewportMetrics(DisplayMetrics metrics) {
        viewportRectLeft   = pageRectLeft   = cssPageRectLeft   = 0;
        viewportRectTop    = pageRectTop    = cssPageRectTop    = 0;
        viewportRectRight  = pageRectRight  = cssPageRectRight  = metrics.widthPixels;
        viewportRectBottom = pageRectBottom = cssPageRectBottom = metrics.heightPixels;
        marginLeft = marginTop = marginRight = marginBottom = 0;
        zoomFactor = 1.0f;
        isRTL = false;
    }

    


    @WrapElementForJNI(allowMultithread = true)
    public ImmutableViewportMetrics(float aPageRectLeft, float aPageRectTop,
        float aPageRectRight, float aPageRectBottom, float aCssPageRectLeft,
        float aCssPageRectTop, float aCssPageRectRight, float aCssPageRectBottom,
        float aViewportRectLeft, float aViewportRectTop, float aViewportRectRight,
        float aViewportRectBottom, float aZoomFactor)
    {
        this(aPageRectLeft, aPageRectTop,
             aPageRectRight, aPageRectBottom, aCssPageRectLeft,
             aCssPageRectTop, aCssPageRectRight, aCssPageRectBottom,
             aViewportRectLeft, aViewportRectTop, aViewportRectRight,
             aViewportRectBottom, 0.0f, 0.0f, 0.0f, 0.0f, aZoomFactor, false);
    }

    private ImmutableViewportMetrics(float aPageRectLeft, float aPageRectTop,
        float aPageRectRight, float aPageRectBottom, float aCssPageRectLeft,
        float aCssPageRectTop, float aCssPageRectRight, float aCssPageRectBottom,
        float aViewportRectLeft, float aViewportRectTop, float aViewportRectRight,
        float aViewportRectBottom, float aMarginLeft,
        float aMarginTop, float aMarginRight,
        float aMarginBottom, float aZoomFactor, boolean aIsRTL)
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
        marginLeft = aMarginLeft;
        marginTop = aMarginTop;
        marginRight = aMarginRight;
        marginBottom = aMarginBottom;
        zoomFactor = aZoomFactor;
        isRTL = aIsRTL;
    }

    public float getWidth() {
        return viewportRectRight - viewportRectLeft;
    }

    public float getHeight() {
        return viewportRectBottom - viewportRectTop;
    }

    public float getWidthWithoutMargins() {
        return viewportRectRight - viewportRectLeft - marginLeft - marginRight;
    }

    public float getHeightWithoutMargins() {
        return viewportRectBottom - viewportRectTop - marginTop - marginBottom;
    }

    public PointF getOrigin() {
        return new PointF(viewportRectLeft, viewportRectTop);
    }

    public PointF getMarginOffset() {
        if (isRTL) {
            return new PointF(marginLeft - marginRight, marginTop);
        }
        return new PointF(marginLeft, marginTop);
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

    public float getPageWidthWithMargins() {
        return (pageRectRight - pageRectLeft) + marginLeft + marginRight;
    }

    public float getPageHeight() {
        return pageRectBottom - pageRectTop;
    }

    public float getPageHeightWithMargins() {
        return (pageRectBottom - pageRectTop) + marginTop + marginBottom;
    }

    public RectF getCssPageRect() {
        return new RectF(cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom);
    }

    public RectF getOverscroll() {
        return new RectF(Math.max(0, pageRectLeft - viewportRectLeft),
                         Math.max(0, pageRectTop - viewportRectTop),
                         Math.max(0, viewportRectRight - pageRectRight),
                         Math.max(0, viewportRectBottom - pageRectBottom));
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
            FloatUtils.interpolate(marginLeft, to.marginLeft, t),
            FloatUtils.interpolate(marginTop, to.marginTop, t),
            FloatUtils.interpolate(marginRight, to.marginRight, t),
            FloatUtils.interpolate(marginBottom, to.marginBottom, t),
            FloatUtils.interpolate(zoomFactor, to.zoomFactor, t),
            t >= 0.5 ? to.isRTL : isRTL);
    }

    public ImmutableViewportMetrics setViewportSize(float width, float height) {
        if (FloatUtils.fuzzyEquals(width, getWidth()) && FloatUtils.fuzzyEquals(height, getHeight())) {
            return this;
        }

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectLeft + width, viewportRectTop + height,
            marginLeft, marginTop, marginRight, marginBottom,
            zoomFactor, isRTL);
    }

    public ImmutableViewportMetrics setViewportOrigin(float newOriginX, float newOriginY) {
        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            newOriginX, newOriginY, newOriginX + getWidth(), newOriginY + getHeight(),
            marginLeft, marginTop, marginRight, marginBottom,
            zoomFactor, isRTL);
    }

    public ImmutableViewportMetrics setZoomFactor(float newZoomFactor) {
        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            marginLeft, marginTop, marginRight, marginBottom,
            newZoomFactor, isRTL);
    }

    public ImmutableViewportMetrics offsetViewportBy(float dx, float dy) {
        return setViewportOrigin(viewportRectLeft + dx, viewportRectTop + dy);
    }

    public ImmutableViewportMetrics offsetViewportByAndClamp(float dx, float dy) {
        if (isRTL) {
            return setViewportOrigin(
                Math.min(pageRectRight - getWidthWithoutMargins(), Math.max(viewportRectLeft + dx, pageRectLeft)),
                Math.max(pageRectTop, Math.min(viewportRectTop + dy, pageRectBottom - getHeightWithoutMargins())));
        }
        return setViewportOrigin(
            Math.max(pageRectLeft, Math.min(viewportRectLeft + dx, pageRectRight - getWidthWithoutMargins())),
            Math.max(pageRectTop, Math.min(viewportRectTop + dy, pageRectBottom - getHeightWithoutMargins())));
    }

    public ImmutableViewportMetrics setPageRect(RectF pageRect, RectF cssPageRect) {
        return new ImmutableViewportMetrics(
            pageRect.left, pageRect.top, pageRect.right, pageRect.bottom,
            cssPageRect.left, cssPageRect.top, cssPageRect.right, cssPageRect.bottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            marginLeft, marginTop, marginRight, marginBottom,
            zoomFactor, isRTL);
    }

    public ImmutableViewportMetrics setMargins(float left, float top, float right, float bottom) {
        if (FloatUtils.fuzzyEquals(left, marginLeft)
                && FloatUtils.fuzzyEquals(top, marginTop)
                && FloatUtils.fuzzyEquals(right, marginRight)
                && FloatUtils.fuzzyEquals(bottom, marginBottom)) {
            return this;
        }

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            left, top, right, bottom, zoomFactor, isRTL);
    }

    public ImmutableViewportMetrics setMarginsFrom(ImmutableViewportMetrics fromMetrics) {
        return setMargins(fromMetrics.marginLeft,
                          fromMetrics.marginTop,
                          fromMetrics.marginRight,
                          fromMetrics.marginBottom);
    }

    public ImmutableViewportMetrics setIsRTL(boolean aIsRTL) {
        if (isRTL == aIsRTL) {
            return this;
        }

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            viewportRectLeft, viewportRectTop, viewportRectRight, viewportRectBottom,
            marginLeft, marginTop, marginRight, marginBottom, zoomFactor, aIsRTL);
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
            marginLeft, marginTop, marginRight, marginBottom,
            newZoomFactor, isRTL);
    }

    
    private ImmutableViewportMetrics clamp(float marginLeft, float marginTop,
                                           float marginRight, float marginBottom) {
        RectF newViewport = getViewport();
        PointF offset = getMarginOffset();

        
        if (newViewport.right > pageRectRight + marginLeft + marginRight)
            newViewport.offset((pageRectRight + marginLeft + marginRight) - newViewport.right, 0);
        if (newViewport.left < pageRectLeft)
            newViewport.offset(pageRectLeft - newViewport.left, 0);

        if (newViewport.bottom > pageRectBottom + marginTop + marginBottom)
            newViewport.offset(0, (pageRectBottom + marginTop + marginBottom) - newViewport.bottom);
        if (newViewport.top < pageRectTop)
            newViewport.offset(0, pageRectTop - newViewport.top);

        return new ImmutableViewportMetrics(
            pageRectLeft, pageRectTop, pageRectRight, pageRectBottom,
            cssPageRectLeft, cssPageRectTop, cssPageRectRight, cssPageRectBottom,
            newViewport.left, newViewport.top, newViewport.right, newViewport.bottom,
            marginLeft, marginTop, marginRight, marginBottom,
            zoomFactor, isRTL);
    }

    public ImmutableViewportMetrics clamp() {
        return clamp(0, 0, 0, 0);
    }

    public ImmutableViewportMetrics clampWithMargins() {
        return clamp(marginLeft, marginTop,
                     marginRight, marginBottom);
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
                + cssPageRectBottom + ") m=(" + marginLeft + ","
                + marginTop + "," + marginRight + ","
                + marginBottom + ") z=" + zoomFactor + ", rtl=" + isRTL;
    }
}
