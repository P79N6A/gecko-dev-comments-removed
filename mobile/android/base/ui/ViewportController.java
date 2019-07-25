




































package org.mozilla.gecko.ui;

import android.graphics.PointF;
import android.graphics.RectF;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.RectUtils;


public class ViewportController {
    private IntSize mPageSize;
    private RectF mVisibleRect;

    public ViewportController(IntSize pageSize, RectF visibleRect) {
        mPageSize = pageSize;
        mVisibleRect = visibleRect;
    }

    private float clamp(float min, float value, float max) {
        if (max < min)
            return min;
        return (value < min) ? min : (value > max) ? max : value;
    }

    
    public RectF clampRect(RectF rect) {
        float x = clamp(0, rect.left, mPageSize.width - LayerController.TILE_WIDTH);
        float y = clamp(0, rect.top, mPageSize.height - LayerController.TILE_HEIGHT);
        return new RectF(x, y, x + rect.width(), x + rect.height());
    }

    
    public static RectF widenRect(RectF rect) {
        return new RectF(rect.centerX() - LayerController.TILE_WIDTH / 2,
                         rect.centerY() - LayerController.TILE_HEIGHT / 2,
                         rect.centerX() + LayerController.TILE_WIDTH / 2,
                         rect.centerY() + LayerController.TILE_HEIGHT / 2);
    }

    



    public float getZoomFactor(RectF layerVisibleRect, IntSize layerPageSize,
                               IntSize screenSize) {
        RectF transformed = transformVisibleRect(layerVisibleRect, layerPageSize);
        return (float)screenSize.width / transformed.width();
    }

    



    public RectF transformVisibleRect(RectF layerVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return RectUtils.scale(layerVisibleRect, 1.0f / zoomFactor);
    }

    



    public RectF untransformVisibleRect(RectF viewportVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return RectUtils.scale(viewportVisibleRect, zoomFactor);
    }

    public IntSize getPageSize() { return mPageSize; }
    public void setPageSize(IntSize pageSize) { mPageSize = pageSize; }
    public RectF getVisibleRect() { return mVisibleRect; }
    public void setVisibleRect(RectF visibleRect) { mVisibleRect = visibleRect; }
}

