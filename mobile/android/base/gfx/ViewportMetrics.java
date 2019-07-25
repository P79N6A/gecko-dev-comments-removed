





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.RectUtils;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONStringer;
import android.util.Log;





public class ViewportMetrics {
    private static final String LOGTAG = "GeckoViewportMetrics";

    private FloatSize mPageSize;
    private RectF mViewportRect;
    private PointF mViewportOffset;
    private float mZoomFactor;

    public ViewportMetrics() {
        mPageSize = new FloatSize(LayerController.TILE_WIDTH,
                                  LayerController.TILE_HEIGHT);
        mViewportRect = new RectF(0, 0, 1, 1);
        mViewportOffset = new PointF(0, 0);
        mZoomFactor = 1.0f;
    }

    public ViewportMetrics(ViewportMetrics viewport) {
        mPageSize = new FloatSize(viewport.getPageSize());
        mViewportRect = new RectF(viewport.getViewport());
        PointF offset = viewport.getViewportOffset();
        mViewportOffset = new PointF(offset.x, offset.y);
        mZoomFactor = viewport.getZoomFactor();
    }

    public ViewportMetrics(JSONObject json) throws JSONException {
        float x = (float)json.getDouble("x");
        float y = (float)json.getDouble("y");
        float width = (float)json.getDouble("width");
        float height = (float)json.getDouble("height");
        float pageWidth = (float)json.getDouble("pageWidth");
        float pageHeight = (float)json.getDouble("pageHeight");
        float offsetX = (float)json.getDouble("offsetX");
        float offsetY = (float)json.getDouble("offsetY");
        float zoom = (float)json.getDouble("zoom");

        mPageSize = new FloatSize(pageWidth, pageHeight);
        mViewportRect = new RectF(x, y, x + width, y + height);
        mViewportOffset = new PointF(offsetX, offsetY);
        mZoomFactor = zoom;
    }

    public PointF getOptimumViewportOffset() {
        
        
        
        Point optimumOffset =
            new Point((int)Math.round((LayerController.TILE_WIDTH - mViewportRect.width()) / 2),
                      (int)Math.round((LayerController.TILE_HEIGHT - mViewportRect.height()) / 2));

        



        Rect viewport = RectUtils.round(getClampedViewport());

        
        
        
        if (viewport.left - optimumOffset.x < 0)
          optimumOffset.x = viewport.left;
        else if (optimumOffset.x + viewport.right > mPageSize.width)
          optimumOffset.x -= (mPageSize.width - (optimumOffset.x + viewport.right));

        if (viewport.top - optimumOffset.y < 0)
          optimumOffset.y = viewport.top;
        else if (optimumOffset.y + viewport.bottom > mPageSize.height)
          optimumOffset.y -= (mPageSize.height - (optimumOffset.y + viewport.bottom));

        return new PointF(optimumOffset);
    }

    public PointF getOrigin() {
        return new PointF(mViewportRect.left, mViewportRect.top);
    }

    public PointF getDisplayportOrigin() {
        return new PointF(mViewportRect.left - mViewportOffset.x,
                          mViewportRect.top - mViewportOffset.y);
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

    public PointF getViewportOffset() {
        return mViewportOffset;
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
        mViewportRect.set(origin.x, origin.y,
                          origin.x + mViewportRect.width(),
                          origin.y + mViewportRect.height());
    }

    public void setSize(FloatSize size) {
        mViewportRect.right = mViewportRect.left + size.width;
        mViewportRect.bottom = mViewportRect.top + size.height;
    }

    public void setViewportOffset(PointF offset) {
        mViewportOffset = offset;
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
    }

    




    public ViewportMetrics interpolate(ViewportMetrics to, float t) {
        ViewportMetrics result = new ViewportMetrics();
        result.mPageSize = mPageSize.interpolate(to.mPageSize, t);
        result.mZoomFactor = FloatUtils.interpolate(mZoomFactor, to.mZoomFactor, t);
        result.mViewportRect = RectUtils.interpolate(mViewportRect, to.mViewportRect, t);
        result.mViewportOffset = PointUtils.interpolate(mViewportOffset, to.mViewportOffset, t);
        return result;
    }

    public String toJSON() {
        try {
            return new JSONStringer().object()
                .key("x").value(mViewportRect.left)
                .key("y").value(mViewportRect.top)
                .key("width").value(mViewportRect.width())
                .key("height").value(mViewportRect.height())
                .key("pageWidth").value(mPageSize.width)
                .key("pageHeight").value(mPageSize.height)
                .key("offsetX").value(mViewportOffset.x)
                .key("offsetY").value(mViewportOffset.y)
                .key("zoom").value(mZoomFactor)
                .endObject().toString();
        } catch (JSONException je) {
            Log.e(LOGTAG, "Error serializing viewportmetrics", je);
            return "";
        }
    }
}

