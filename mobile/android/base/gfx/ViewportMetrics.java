





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.DisplayMetrics;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.RectUtils;
import org.json.JSONException;
import org.json.JSONObject;
import android.util.Log;





public class ViewportMetrics {
    private static final String LOGTAG = "GeckoViewportMetrics";

    private FloatSize mPageSize;
    private RectF mViewportRect;
    private float mZoomFactor;

    
    
    private PointF mViewportBias;
    private static final float MAX_BIAS = 0.8f;

    public ViewportMetrics() {
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        mPageSize = new FloatSize(metrics.widthPixels, metrics.heightPixels);
        mViewportRect = new RectF(0, 0, metrics.widthPixels, metrics.heightPixels);
        mZoomFactor = 1.0f;
        mViewportBias = new PointF(0.0f, 0.0f);
    }

    public ViewportMetrics(ViewportMetrics viewport) {
        mPageSize = new FloatSize(viewport.getPageSize());
        mViewportRect = new RectF(viewport.getViewport());
        mZoomFactor = viewport.getZoomFactor();
        mViewportBias = viewport.mViewportBias;
    }

    public ViewportMetrics(JSONObject json) throws JSONException {
        float x = (float)json.getDouble("x");
        float y = (float)json.getDouble("y");
        float width = (float)json.getDouble("width");
        float height = (float)json.getDouble("height");
        float pageWidth = (float)json.getDouble("pageWidth");
        float pageHeight = (float)json.getDouble("pageHeight");
        float zoom = (float)json.getDouble("zoom");

        mPageSize = new FloatSize(pageWidth, pageHeight);
        mViewportRect = new RectF(x, y, x + width, y + height);
        mZoomFactor = zoom;
        mViewportBias = new PointF(0.0f, 0.0f);
    }

    public PointF getOptimumViewportOffset(IntSize displayportSize) {
        RectF viewport = getClampedViewport();

        FloatSize bufferSpace = new FloatSize(displayportSize.width - viewport.width(),
                                            displayportSize.height - viewport.height());
        PointF optimumOffset =
            new PointF(bufferSpace.width * ((mViewportBias.x + 1.0f) / 2.0f),
                       bufferSpace.height * ((mViewportBias.y + 1.0f) / 2.0f));

        
        
        
        if (viewport.left - optimumOffset.x < 0)
          optimumOffset.x = viewport.left;
        else if ((bufferSpace.width - optimumOffset.x) + viewport.right > mPageSize.width)
          optimumOffset.x = bufferSpace.width - (mPageSize.width - viewport.right);

        if (viewport.top - optimumOffset.y < 0)
          optimumOffset.y = viewport.top;
        else if ((bufferSpace.height - optimumOffset.y) + viewport.bottom > mPageSize.height)
          optimumOffset.y = bufferSpace.height - (mPageSize.height - viewport.bottom);

        return new PointF(Math.round(optimumOffset.x), Math.round(optimumOffset.y));
    }

    public PointF getOrigin() {
        return new PointF(mViewportRect.left, mViewportRect.top);
    }

    public PointF getDisplayportOrigin() {
        return new PointF(mViewportRect.left,
                          mViewportRect.top);
    }

    public FloatSize getSize() {
        return new FloatSize(mViewportRect.width(), mViewportRect.height());
    }

    public RectF getViewport() {
        return mViewportRect;
    }

    
    public RectF getClampedViewport() {
        RectF clampedViewport = new RectF(mViewportRect);

        
        
        
        if (clampedViewport.right > mPageSize.width)
            clampedViewport.offset(mPageSize.width - clampedViewport.right, 0);
        if (clampedViewport.left < 0)
            clampedViewport.offset(-clampedViewport.left, 0);

        if (clampedViewport.bottom > mPageSize.height)
            clampedViewport.offset(0, mPageSize.height - clampedViewport.bottom);
        if (clampedViewport.top < 0)
            clampedViewport.offset(0, -clampedViewport.top);

        return clampedViewport;
    }

    public FloatSize getPageSize() {
        return mPageSize;
    }

    public float getZoomFactor() {
        return mZoomFactor;
    }

    public void setPageSize(FloatSize pageSize) {
        mPageSize = pageSize;
    }

    public void setViewport(RectF viewport) {
        mViewportRect = viewport;
    }

    public void setOrigin(PointF origin) {
        
        
        
        

        
        
        
        if (FloatUtils.fuzzyEquals(origin.x, mViewportRect.left))
            mViewportBias.x = 0;
        else
            mViewportBias.x = ((mViewportRect.left - origin.x) > 0) ? MAX_BIAS : -MAX_BIAS;
        if (FloatUtils.fuzzyEquals(origin.y, mViewportRect.top))
            mViewportBias.y = 0;
        else
            mViewportBias.y = ((mViewportRect.top - origin.y) > 0) ? MAX_BIAS : -MAX_BIAS;

        mViewportRect.set(origin.x, origin.y,
                          origin.x + mViewportRect.width(),
                          origin.y + mViewportRect.height());
    }

    public void setSize(FloatSize size) {
        mViewportRect.right = mViewportRect.left + size.width;
        mViewportRect.bottom = mViewportRect.top + size.height;
    }

    public void setZoomFactor(float zoomFactor) {
        mZoomFactor = zoomFactor;
    }

    



    public void scaleTo(float newZoomFactor, PointF focus) {
        float scaleFactor = newZoomFactor / mZoomFactor;

        mPageSize = mPageSize.scale(scaleFactor);

        PointF origin = getOrigin();
        origin.offset(focus.x, focus.y);
        origin = PointUtils.scale(origin, scaleFactor);
        origin.offset(-focus.x, -focus.y);
        setOrigin(origin);

        mZoomFactor = newZoomFactor;

        
        
        
        
        
        
        mViewportBias.set(((focus.x / mViewportRect.width()) * (2.0f * MAX_BIAS)) - MAX_BIAS,
                          ((focus.y / mViewportRect.height()) * (2.0f * MAX_BIAS)) - MAX_BIAS);
    }

    




    public ViewportMetrics interpolate(ViewportMetrics to, float t) {
        ViewportMetrics result = new ViewportMetrics();
        result.mPageSize = mPageSize.interpolate(to.mPageSize, t);
        result.mZoomFactor = FloatUtils.interpolate(mZoomFactor, to.mZoomFactor, t);
        result.mViewportRect = RectUtils.interpolate(mViewportRect, to.mViewportRect, t);
        return result;
    }

    public boolean fuzzyEquals(ViewportMetrics other) {
        return mPageSize.fuzzyEquals(other.mPageSize)
            && RectUtils.fuzzyEquals(mViewportRect, other.mViewportRect)
            && FloatUtils.fuzzyEquals(mZoomFactor, other.mZoomFactor);
    }

    public String toJSON() {
        
        
        int height = Math.round(mViewportRect.height());
        int width = Math.round(mViewportRect.width());

        StringBuffer sb = new StringBuffer(256);
        sb.append("{ \"x\" : ").append(mViewportRect.left)
          .append(", \"y\" : ").append(mViewportRect.top)
          .append(", \"width\" : ").append(width)
          .append(", \"height\" : ").append(height)
          .append(", \"pageWidth\" : ").append(mPageSize.width)
          .append(", \"pageHeight\" : ").append(mPageSize.height)
          .append(", \"zoom\" : ").append(mZoomFactor)
          .append(" }");
        return sb.toString();
    }

    @Override
    public String toString() {
        StringBuffer buff = new StringBuffer(128);
        buff.append("v=").append(mViewportRect.toString())
            .append(" p=").append(mPageSize.toString())
            .append(" z=").append(mZoomFactor);
        return buff.toString();
    }
}

