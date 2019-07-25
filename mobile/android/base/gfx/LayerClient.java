




































package org.mozilla.gecko.gfx;




public abstract class LayerClient {
    private LayerController mLayerController;

    public abstract void geometryChanged();
    public abstract void viewportSizeChanged();
    protected abstract void render();

    public LayerController getLayerController() { return mLayerController; }
    public void setLayerController(LayerController layerController) {
        mLayerController = layerController;
    }

    



    public void beginTransaction(Layer aLayer) {
        if (mLayerController != null) {
            LayerView view = mLayerController.getView();
            if (view != null) {
                aLayer.beginTransaction(view);
                return;
            }
        }

        aLayer.beginTransaction();
    }

    
    public void endTransaction(Layer aLayer) {
        aLayer.endTransaction();
    }
}

