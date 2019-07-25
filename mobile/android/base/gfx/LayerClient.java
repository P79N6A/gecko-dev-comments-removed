




































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

    



    public void beginTransaction(TileLayer aTileLayer) {
        if (mLayerController != null) {
            LayerView view = mLayerController.getView();
            if (view != null) {
                aTileLayer.beginTransaction(view);
                return;
            }
        }

        aTileLayer.beginTransaction();
    }

    
    public void endTransaction(TileLayer aTileLayer) {
        aTileLayer.endTransaction();
    }
}

