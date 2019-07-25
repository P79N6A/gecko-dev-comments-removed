




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.IntRect;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;




public abstract class LayerClient {
    private LayerController mLayerController;

    public abstract void geometryChanged();
    public abstract IntSize getPageSize();

    
    public abstract void setPageSize(IntSize pageSize);

    public abstract void init();
    protected abstract void render();

    public LayerController getLayerController() { return mLayerController; }
    public void setLayerController(LayerController layerController) {
        mLayerController = layerController;
    }
}

