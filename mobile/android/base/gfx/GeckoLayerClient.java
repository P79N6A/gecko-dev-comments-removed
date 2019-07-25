





































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
    private Rect mDisplayPortMargins;

    private VirtualLayer mRootLayer;

    
    private ViewportMetrics mGeckoViewport;

    
    private ViewportMetrics mNewGeckoViewport;

    private boolean mViewportSizeChanged;
    private boolean mIgnorePaintsPendingViewportSizeChange;
    private boolean mFirstPaint = true;

    
    
    
    
    
    private boolean mUpdateViewportOnEndDraw;

    private String mLastCheckerboardColor;

    
    private DrawListener mDrawListener;

    
    private ViewTransform mCurrentViewTransform;

    public GeckoLayerClient(Context context) {
        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
        mDisplayPortMargins = new Rect(DEFAULT_DISPLAY_PORT_MARGIN,
                                       DEFAULT_DISPLAY_PORT_MARGIN,
                                       DEFAULT_DISPLAY_PORT_MARGIN,
                                       DEFAULT_DISPLAY_PORT_MARGIN);
        
        
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
    }

    
    void setLayerController(LayerController layerController) {
        mLayerController = layerController;

        layerController.setRoot(mRootLayer);
        if (mGeckoViewport != null) {
            layerController.setViewportMetrics(mGeckoViewport);
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateAndDraw", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateLater", this);

        sendResizeEventIfNecessary(false);

        LayerView view = layerController.getView();
        view.setListener(this);

        mLayerRenderer = new LayerRenderer(view);
    }

    
    public boolean beginDrawing(int width, int height, String metadata) {
        
        
        
        if (!mFirstPaint && mIgnorePaintsPendingViewportSizeChange) {
            return false;
        }
        mFirstPaint = false;

        
        if (initializeVirtualLayer()) {
            Log.e(LOGTAG, "### Cancelling draw due to virtual layer initialization");
            return false;
        }

        try {
            JSONObject viewportObject = new JSONObject(metadata);
            mNewGeckoViewport = new ViewportMetrics(viewportObject);
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
            
            
            
            
            FloatSize viewportSize = mLayerController.getViewportSize();
            mGeckoViewport = mNewGeckoViewport;
            mGeckoViewport.setSize(viewportSize);

            RectF position = mGeckoViewport.getViewport();
            mRootLayer.setPositionAndResolution(RectUtils.round(position), mGeckoViewport.getZoomFactor());

            if (mUpdateViewportOnEndDraw) {
                mLayerController.setViewportMetrics(mGeckoViewport);
                mLayerController.abortPanZoomAnimation();
                mUpdateViewportOnEndDraw = false;
            } else {
                
                
                if (FloatUtils.fuzzyEquals(mLayerController.getZoomFactor(),
                        mGeckoViewport.getZoomFactor()))
                    mLayerController.setPageSize(mGeckoViewport.getPageSize());
            }
        }

        Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - endDrawing");
        
        if (mDrawListener != null) {
            mDrawListener.drawFinished();
        }
    }

    RectF getDisplayPort() {
        RectF displayPort = new RectF(mRootLayer.getPosition());
        displayPort.left -= mDisplayPortMargins.left;
        displayPort.top -= mDisplayPortMargins.top;
        displayPort.right += mDisplayPortMargins.right;
        displayPort.bottom += mDisplayPortMargins.bottom;
        return displayPort;
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
        mIgnorePaintsPendingViewportSizeChange = true;
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(mLayerController.getViewportMetrics());

        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(viewportMetrics, mDisplayPortMargins));
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }
    }

    
    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:UpdateAndDraw".equals(event)) {
            mUpdateViewportOnEndDraw = true;
            mIgnorePaintsPendingViewportSizeChange = false;

            
            Rect rect = new Rect(0, 0, mBufferSize.width, mBufferSize.height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createDrawEvent(rect));
        } else if ("Viewport:UpdateLater".equals(event)) {
            mUpdateViewportOnEndDraw = true;
            mIgnorePaintsPendingViewportSizeChange = false;
        }
    }

    
    public String getResponse() {
        
        
        
        
        return RectUtils.toJSON(mDisplayPortMargins);
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
            mLayerController.abortPanZoomAnimation();
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

