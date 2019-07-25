




































package org.mozilla.gecko.ui;

import org.mozilla.gecko.gfx.FloatPoint;
import org.mozilla.gecko.gfx.FloatRect;
import org.mozilla.gecko.gfx.IntRect;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;


public class ViewportController {
    private IntSize mPageSize;
    private FloatRect mVisibleRect;

    public ViewportController(IntSize pageSize, FloatRect visibleRect) {
        mPageSize = pageSize;
        mVisibleRect = visibleRect;
    }

    private float clamp(float min, float value, float max) {
        if (max < min)
            return min;
        return (value < min) ? min : (value > max) ? max : value;
    }

    
    public FloatRect clampRect(FloatRect rect) {
        float x = clamp(0, rect.x, mPageSize.width - LayerController.TILE_WIDTH);
        float y = clamp(0, rect.y, mPageSize.height - LayerController.TILE_HEIGHT);
        return new FloatRect(x, y, rect.width, rect.height);
    }

    
    public static FloatRect widenRect(FloatRect rect) {
        FloatPoint center = rect.getCenter();
        return new FloatRect(center.x - LayerController.TILE_WIDTH / 2,
                             center.y - LayerController.TILE_HEIGHT / 2,
                             LayerController.TILE_WIDTH,
                             LayerController.TILE_HEIGHT);
    }

    



    public float getZoomFactor(FloatRect layerVisibleRect, IntSize layerPageSize,
                               IntSize screenSize) {
        FloatRect transformed = transformVisibleRect(layerVisibleRect, layerPageSize);
        return (float)screenSize.width / transformed.width;
    }

    



    public FloatRect transformVisibleRect(FloatRect layerVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return layerVisibleRect.scaleAll(1.0f / zoomFactor);
    }

    



    public FloatRect untransformVisibleRect(FloatRect viewportVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return viewportVisibleRect.scaleAll(zoomFactor);
    }

    public IntSize getPageSize() { return mPageSize; }
    public void setPageSize(IntSize pageSize) { mPageSize = pageSize; }
    public FloatRect getVisibleRect() { return mVisibleRect; }
    public void setVisibleRect(FloatRect visibleRect) { mVisibleRect = visibleRect; }
}

