




































package org.mozilla.fennec.ui;

import org.mozilla.fennec.gfx.IntPoint;
import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerController;


public class ViewportController {
    private IntSize mPageSize;
    private IntRect mVisibleRect;

    public ViewportController(IntSize pageSize, IntRect visibleRect) {
        mPageSize = pageSize;
        mVisibleRect = visibleRect;
    }

    private int clamp(int min, int value, int max) {
        if (max < min)
            return min;
        return (value < min) ? min : (value > max) ? max : value;
    }

    
    public IntRect clampRect(IntRect rect) {
        int x = clamp(0, rect.x, mPageSize.width - LayerController.TILE_WIDTH);
        int y = clamp(0, rect.y, mPageSize.height - LayerController.TILE_HEIGHT);
        return new IntRect(x, y, rect.width, rect.height);
    }

    
    public static IntRect widenRect(IntRect rect) {
        IntPoint center = rect.getCenter();
        return new IntRect(center.x - LayerController.TILE_WIDTH / 2,
                           center.y - LayerController.TILE_HEIGHT / 2,
                           LayerController.TILE_WIDTH,
                           LayerController.TILE_HEIGHT);
    }

    



    public float getZoomFactor(IntRect layerVisibleRect, IntSize layerPageSize,
                               IntSize screenSize) {
        IntRect transformed = transformVisibleRect(layerVisibleRect, layerPageSize);
        return (float)screenSize.width / (float)transformed.width;
    }

    



    public IntRect transformVisibleRect(IntRect layerVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return layerVisibleRect.scaleAll(1.0f / zoomFactor);
    }

    



    public IntRect untransformVisibleRect(IntRect viewportVisibleRect, IntSize layerPageSize) {
        float zoomFactor = (float)layerPageSize.width / (float)mPageSize.width;
        return viewportVisibleRect.scaleAll(zoomFactor);
    }

    public IntSize getPageSize() { return mPageSize; }
    public void setPageSize(IntSize pageSize) { mPageSize = pageSize; }
    public IntRect getVisibleRect() { return mVisibleRect; }
    public void setVisibleRect(IntRect visibleRect) { mVisibleRect = visibleRect; }
}

