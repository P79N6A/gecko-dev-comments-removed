




































package org.mozilla.gecko.gfx;




public abstract class LayerClient {
    private LayerController mLayerController;

    public abstract void geometryChanged();
    protected abstract void render();

    public LayerController getLayerController() { return mLayerController; }
    public void setLayerController(LayerController layerController) {
        mLayerController = layerController;
    }
}

