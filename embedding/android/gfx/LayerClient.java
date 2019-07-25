




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerController;




public abstract class LayerClient {
    private LayerController mLayerController;
    protected float mZoomFactor;

    public abstract void geometryChanged();
    public abstract IntSize getPageSize();

    
    public abstract void setPageSize(IntSize pageSize);

    public abstract void init();
    protected abstract void render();

    public LayerClient() {
        mZoomFactor = 1.0f;
    }

    public LayerController getLayerController() { return mLayerController; }
    public void setLayerController(LayerController layerController) {
        mLayerController = layerController;
    }
}

