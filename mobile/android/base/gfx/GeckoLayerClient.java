





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventResponder;
import org.json.JSONException;
import org.json.JSONObject;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;

public class GeckoLayerClient implements GeckoEventResponder,
                                         FlexibleGLSurfaceView.Listener {
    private static final String LOGTAG = "GeckoLayerClient";

    private static final int DEFAULT_DISPLAY_PORT_MARGIN = 300;

    private LayerController mLayerController;
    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    private IntSize mScreenSize;
    private IntSize mWindowSize;
    private IntSize mBufferSize;
    private RectF mDisplayPort;

    private VirtualLayer mRootLayer;

    
    private ViewportMetrics mGeckoViewport;

    private boolean mViewportSizeChanged;

    private String mLastCheckerboardColor;

    
    private DrawListener mDrawListener;

    
    private ViewTransform mCurrentViewTransform;

    public GeckoLayerClient(Context context) {
        
        
        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
        mDisplayPort = new RectF();
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
    }

    
    void setLayerController(LayerController layerController) {
        mLayerController = layerController;

        layerController.setRoot(mRootLayer);
        if (mGeckoViewport != null) {
            layerController.setViewportMetrics(mGeckoViewport);
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:Update", this);

        sendResizeEventIfNecessary(false);

        LayerView view = layerController.getView();
        view.setListener(this);

        mLayerRenderer = new LayerRenderer(view);
    }

    
    public boolean beginDrawing(int width, int height, String metadata) {
        
        if (initializeVirtualLayer()) {
            Log.e(LOGTAG, "### Cancelling draw due to virtual layer initialization");
            return false;
        }

        try {
            JSONObject viewportObject = new JSONObject(metadata);
            mGeckoViewport = new ViewportMetrics(viewportObject);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Aborting draw, bad viewport description: " + metadata);
            return false;
        }

        if (mBufferSize.width != width || mBufferSize.height != height) {
            mBufferSize = new IntSize(width, height);
        }

        return true;
    }

    
    public void endDrawing() {
        synchronized (mLayerController) {
            RectF position = mGeckoViewport.getViewport();
            mRootLayer.setPositionAndResolution(RectUtils.round(position), mGeckoViewport.getZoomFactor());
        }

        Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - endDrawing");
        
        if (mDrawListener != null) {
            mDrawListener.drawFinished();
        }
    }

    RectF getDisplayPort() {
        return mDisplayPort;
    }

    
    private void sendResizeEventIfNecessary(boolean force) {
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        IntSize newScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        IntSize newWindowSize = getBufferSize();

        boolean screenSizeChanged = mScreenSize == null || !mScreenSize.equals(newScreenSize);
        boolean windowSizeChanged = mWindowSize == null || !mWindowSize.equals(newWindowSize);

        if (!force && !screenSizeChanged && !windowSizeChanged) {
            return;
        }

        mScreenSize = newScreenSize;
        mWindowSize = newWindowSize;

        if (screenSizeChanged) {
            Log.i(LOGTAG, "### Screen-size changed to " + mScreenSize);
        }

        if (windowSizeChanged) {
            Log.i(LOGTAG, "### Window-size changed to " + mWindowSize);
        }

        GeckoEvent event = GeckoEvent.createSizeChangedEvent(mWindowSize.width, mWindowSize.height,  
                                                             mScreenSize.width, mScreenSize.height); 
        GeckoAppShell.sendEventToGecko(event);
    }

    private boolean initializeVirtualLayer() {
        if (mRootLayer != null) {
            return false;
        }

        VirtualLayer virtualLayer = new VirtualLayer(getBufferSize());
        mLayerController.setRoot(virtualLayer);
        mRootLayer = virtualLayer;

        sendResizeEventIfNecessary(true);
        return true;
    }

    private IntSize getBufferSize() {
        View view = mLayerController.getView();
        IntSize size = new IntSize(view.getWidth(), view.getHeight());
        return size;
    }

    public Bitmap getBitmap() {
        return null;
    }

    void viewportSizeChanged() {
        mViewportSizeChanged = true;
    }

    private void updateDisplayPort() {
        float desiredXMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;
        float desiredYMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;

        ImmutableViewportMetrics metrics = mLayerController.getViewportMetrics(); 

        
        
        

        
        float xBufferAmount = Math.min(desiredXMargins, Math.max(0, metrics.pageSizeWidth - metrics.getWidth()));
        
        
        float savedPixels = (desiredXMargins - xBufferAmount) * (metrics.getHeight() + desiredYMargins);
        float extraYAmount = (float)Math.floor(savedPixels / (metrics.getWidth() + xBufferAmount));
        float yBufferAmount = Math.min(desiredYMargins + extraYAmount, Math.max(0, metrics.pageSizeHeight - metrics.getHeight()));
        
        if (xBufferAmount == desiredXMargins && yBufferAmount < desiredYMargins) {
            savedPixels = (desiredYMargins - yBufferAmount) * (metrics.getWidth() + xBufferAmount);
            float extraXAmount = (float)Math.floor(savedPixels / (metrics.getHeight() + yBufferAmount));
            xBufferAmount = Math.min(xBufferAmount + extraXAmount, Math.max(0, metrics.pageSizeWidth - metrics.getWidth()));
        }

        
        
        
        
        
        float leftMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, Math.max(0, metrics.viewportRectLeft));
        float rightMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, Math.max(0, metrics.pageSizeWidth - (metrics.viewportRectLeft + metrics.getWidth())));
        if (leftMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            rightMargin = xBufferAmount - leftMargin;
        } else if (rightMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            leftMargin = xBufferAmount - rightMargin;
        } else if (!FloatUtils.fuzzyEquals(leftMargin + rightMargin, xBufferAmount)) {
            float delta = xBufferAmount - leftMargin - rightMargin;
            leftMargin += delta / 2;
            rightMargin += delta / 2;
        }

        float topMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, Math.max(0, metrics.viewportRectTop));
        float bottomMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, Math.max(0, metrics.pageSizeHeight - (metrics.viewportRectTop + metrics.getHeight())));
        if (topMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            bottomMargin = yBufferAmount - topMargin;
        } else if (bottomMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            topMargin = yBufferAmount - bottomMargin;
        } else if (!FloatUtils.fuzzyEquals(topMargin + bottomMargin, yBufferAmount)) {
            float delta = yBufferAmount - topMargin - bottomMargin;
            topMargin += delta / 2;
            bottomMargin += delta / 2;
        }

        
        
        
        

        mDisplayPort.left = metrics.viewportRectLeft - leftMargin;
        mDisplayPort.top = metrics.viewportRectTop - topMargin;
        mDisplayPort.right = metrics.viewportRectRight + rightMargin;
        mDisplayPort.bottom = metrics.viewportRectBottom + bottomMargin;
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(mLayerController.getViewportMetrics());

        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        updateDisplayPort();
        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(viewportMetrics, mDisplayPort));
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }
    }

    
    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:Update".equals(event)) {
            try {
                ViewportMetrics newMetrics = new ViewportMetrics(message);
                synchronized (mLayerController) {
                    
                    ImmutableViewportMetrics oldMetrics = mLayerController.getViewportMetrics();
                    newMetrics.setSize(oldMetrics.getSize());
                    mLayerController.setViewportMetrics(newMetrics);
                    mLayerController.abortPanZoomAnimation(false);
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "Unable to create viewport metrics in " + event + " handler", e);
            }
        }
    }

    
    public String getResponse() {
        
        
        
        
        updateDisplayPort();
        return RectUtils.toJSON(mDisplayPort);
    }

    void geometryChanged() {
        
        sendResizeEventIfNecessary(false);
        if (mLayerController.getRedrawHint())
            adjustViewport();
    }

    public int getWidth() {
        return mBufferSize.width;
    }

    public int getHeight() {
        return mBufferSize.height;
    }

    public ViewportMetrics getGeckoViewportMetrics() {
        
        if (mGeckoViewport != null)
            return new ViewportMetrics(mGeckoViewport);
        return null;
    }

    
    public void setFirstPaintViewport(float offsetX, float offsetY, float zoom, float pageWidth, float pageHeight) {
        synchronized (mLayerController) {
            ViewportMetrics currentMetrics = new ViewportMetrics(mLayerController.getViewportMetrics());
            currentMetrics.setOrigin(new PointF(offsetX, offsetY));
            currentMetrics.setZoomFactor(zoom);
            currentMetrics.setPageSize(new FloatSize(pageWidth, pageHeight));
            mLayerController.setViewportMetrics(currentMetrics);
            
            
            
            
            
            
            
            mLayerController.abortPanZoomAnimation(true);
        }
    }

    
    public void setPageSize(float zoom, float pageWidth, float pageHeight) {
        synchronized (mLayerController) {
            
            
            
            float ourZoom = mLayerController.getZoomFactor();
            pageWidth = pageWidth * ourZoom / zoom;
            pageHeight = pageHeight * ourZoom /zoom;
            mLayerController.setPageSize(new FloatSize(pageWidth, pageHeight));
            
            
            
            
        }
    }

    
    




    public ViewTransform getViewTransform() {
        
        
        ImmutableViewportMetrics viewportMetrics = mLayerController.getViewportMetrics();
        mCurrentViewTransform.x = viewportMetrics.viewportRectLeft;
        mCurrentViewTransform.y = viewportMetrics.viewportRectTop;
        mCurrentViewTransform.scale = viewportMetrics.zoomFactor;
        return mCurrentViewTransform;
    }

    
    public LayerRenderer.Frame createFrame() {
        
        if (!mLayerRendererInitialized) {
            mLayerRenderer.checkMonitoringEnabled();
            mLayerRenderer.createDefaultProgram();
            mLayerRendererInitialized = true;
        }

        
        Layer.RenderContext pageContext = mLayerRenderer.createPageContext();
        Layer.RenderContext screenContext = mLayerRenderer.createScreenContext();
        return mLayerRenderer.createFrame(pageContext, screenContext);
    }

    
    public void activateProgram() {
        mLayerRenderer.activateDefaultProgram();
    }

    
    public void deactivateProgram() {
        mLayerRenderer.deactivateDefaultProgram();
    }

    
    public void renderRequested() {
        GeckoAppShell.scheduleComposite();
    }

    
    public void compositionPauseRequested() {
        GeckoAppShell.schedulePauseComposition();
    }

    
    public void compositionResumeRequested() {
        GeckoAppShell.scheduleResumeComposition();
    }

    
    public void surfaceChanged(int width, int height) {
        compositionPauseRequested();
        mLayerController.setViewportSize(new FloatSize(width, height));
        compositionResumeRequested();
        renderRequested();
    }

    
    public void setDrawListener(DrawListener listener) {
        mDrawListener = listener;
    }

    
    public interface DrawListener {
        public void drawFinished();
    }
}

