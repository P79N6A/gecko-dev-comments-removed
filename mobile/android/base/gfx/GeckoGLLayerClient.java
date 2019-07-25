




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.View;

public class GeckoGLLayerClient extends GeckoLayerClient
                                implements FlexibleGLSurfaceView.Listener, VirtualLayer.Listener {
    private static final String LOGTAG = "GeckoGLLayerClient";

    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    public GeckoGLLayerClient(Context context) {
        super(context);
    }

    @Override
    public boolean beginDrawing(int width, int height, int tileWidth, int tileHeight,
                                String metadata, boolean hasDirectTexture) {
        if (!super.beginDrawing(width, height, tileWidth, tileHeight, metadata,
                                hasDirectTexture)) {
            return false;
        }

        
        
        
        if (mBufferSize.width != width || mBufferSize.height != height) {
            mBufferSize = new IntSize(width, height);
        }

        return true;
    }

    @Override
    protected boolean handleDirectTextureChange(boolean hasDirectTexture) {
        Log.e(LOGTAG, "### handleDirectTextureChange");
        if (mTileLayer != null) {
            return false;
        }

        Log.e(LOGTAG, "### Creating virtual layer");
        VirtualLayer virtualLayer = new VirtualLayer();
        virtualLayer.setListener(this);
        virtualLayer.setSize(getBufferSize());
        getLayerController().setRoot(virtualLayer);
        mTileLayer = virtualLayer;

        sendResizeEventIfNecessary(true);
        return true;
    }

    @Override
    public void setLayerController(LayerController layerController) {
        super.setLayerController(layerController);

        LayerView view = layerController.getView();
        view.setListener(this);

        mLayerRenderer = new LayerRenderer(view);
    }

    @Override
    protected boolean shouldDrawProceed(int tileWidth, int tileHeight) {
        Log.e(LOGTAG, "### shouldDrawProceed");
        
        return true;
    }

    @Override
    protected void updateLayerAfterDraw(Rect updatedRect) {
        Log.e(LOGTAG, "### updateLayerAfterDraw");
        
    }

    @Override
    protected IntSize getBufferSize() {
        View view = (View)getLayerController().getView();
        IntSize size = new IntSize(view.getWidth(), view.getHeight());
        Log.e(LOGTAG, "### getBufferSize " + size);
        return size;
    }

    @Override
    protected IntSize getTileSize() {
        Log.e(LOGTAG, "### getTileSize " + getBufferSize());
        return getBufferSize();
    }

    @Override
    protected void tileLayerUpdated() {
        
        mTileLayer.performUpdates(null);
    }

    @Override
    public Bitmap getBitmap() {
        Log.e(LOGTAG, "### getBitmap");
        IntSize size = getBufferSize();
        try {
            return Bitmap.createBitmap(size.width, size.height, Bitmap.Config.RGB_565);
        } catch (OutOfMemoryError oom) {
            Log.e(LOGTAG, "Unable to create bitmap", oom);
            return null;
        }
    }

    @Override
    public int getType() {
        Log.e(LOGTAG, "### getType");
        return LAYER_CLIENT_TYPE_GL;
    }

    public void dimensionsChanged(Point newOrigin, float newResolution) {
        Log.e(LOGTAG, "### dimensionsChanged " + newOrigin + " " + newResolution);
    }

    
    @Override
    protected void sendResizeEventIfNecessary(boolean force) {
        Log.e(LOGTAG, "### sendResizeEventIfNecessary " + force);

        IntSize newSize = getBufferSize();
        if (!force && mScreenSize != null && mScreenSize.equals(newSize)) {
            return;
        }

        mScreenSize = newSize;

        Log.e(LOGTAG, "### Screen-size changed to " + mScreenSize);
        GeckoEvent event = new GeckoEvent(GeckoEvent.SIZE_CHANGED,
                                          mScreenSize.width, mScreenSize.height,
                                          mScreenSize.width, mScreenSize.height,
                                          mScreenSize.width, mScreenSize.height);
        GeckoAppShell.sendEventToGecko(event);
    }

    
    public ViewTransform getViewTransform() {
        Log.e(LOGTAG, "### getViewTransform()");

        
        

        LayerController layerController = getLayerController();
        synchronized (layerController) {
            ViewportMetrics viewportMetrics = layerController.getViewportMetrics();
            PointF viewportOrigin = viewportMetrics.getOrigin();
            Point tileOrigin = mTileLayer.getOrigin();
            float scrollX = viewportOrigin.x; 
            float scrollY = viewportOrigin.y;
            float zoomFactor = viewportMetrics.getZoomFactor();
            Log.e(LOGTAG, "### Viewport metrics = " + viewportMetrics + " tile reso = " +
                  mTileLayer.getResolution());
            return new ViewTransform(scrollX, scrollY, zoomFactor);
        }
    }

    public void renderRequested() {
        Log.e(LOGTAG, "### Render requested, scheduling composite");
        GeckoAppShell.scheduleComposite();
    }

    public void compositionPauseRequested() {
        Log.e(LOGTAG, "### Scheduling PauseComposition");
        GeckoAppShell.schedulePauseComposition();
    }

    public void compositionResumeRequested() {
        Log.e(LOGTAG, "### Scheduling ResumeComposition");
        GeckoAppShell.scheduleResumeComposition();
    }

    public void surfaceChanged(int width, int height) {
        compositionPauseRequested();
        LayerController layerController = getLayerController();
        layerController.setViewportSize(new FloatSize(width, height));
        compositionResumeRequested();
        renderRequested();
    }

    
    public LayerRenderer.Frame createFrame() {
        
        if (!mLayerRendererInitialized) {
            mLayerRenderer.createProgram();
            mLayerRendererInitialized = true;
        }

        
        Layer.RenderContext pageContext = mLayerRenderer.createPageContext();
        Layer.RenderContext screenContext = mLayerRenderer.createScreenContext();
        return mLayerRenderer.createFrame(pageContext, screenContext);
    }

    
    public void activateProgram() {
        mLayerRenderer.activateProgram();
    }

    
    public void deactivateProgram() {
        mLayerRenderer.deactivateProgram();
    }
}

