





































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
    private RectF mDisplayPort;
    private RectF mReturnDisplayPort;

    private VirtualLayer mRootLayer;

    
    private ViewportMetrics mGeckoViewport;

    



    private ImmutableViewportMetrics mFrameMetrics;

    private String mLastCheckerboardColor;

    
    private DrawListener mDrawListener;

    
    private ViewTransform mCurrentViewTransform;

    public GeckoLayerClient(Context context) {
        
        
        mScreenSize = new IntSize(0, 0);
        mWindowSize = new IntSize(0, 0);
        mDisplayPort = new RectF();
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
    }

    
    void setLayerController(LayerController layerController) {
        LayerView view = layerController.getView();

        mLayerController = layerController;

        mRootLayer = new VirtualLayer(new IntSize(view.getWidth(), view.getHeight()));
        mLayerRenderer = new LayerRenderer(view);

        GeckoAppShell.registerGeckoEventListener("Viewport:Update", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:CalculateDisplayPort", this);
        GeckoAppShell.registerGeckoEventListener("Checkerboard:Toggle", this);

        view.setListener(this);
        view.setLayerRenderer(mLayerRenderer);
        layerController.setRoot(mRootLayer);

        sendResizeEventIfNecessary(true);
    }

    RectF getDisplayPort() {
        return mDisplayPort;
    }

    
    private void sendResizeEventIfNecessary(boolean force) {
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        View view = mLayerController.getView();

        IntSize newScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        IntSize newWindowSize = new IntSize(view.getWidth(), view.getHeight());

        boolean screenSizeChanged = !mScreenSize.equals(newScreenSize);
        boolean windowSizeChanged = !mWindowSize.equals(newWindowSize);

        if (!force && !screenSizeChanged && !windowSizeChanged) {
            return;
        }

        mScreenSize = newScreenSize;
        mWindowSize = newWindowSize;

        if (screenSizeChanged) {
            Log.d(LOGTAG, "Screen-size changed to " + mScreenSize);
        }

        if (windowSizeChanged) {
            Log.d(LOGTAG, "Window-size changed to " + mWindowSize);
        }

        GeckoEvent event = GeckoEvent.createSizeChangedEvent(mWindowSize.width, mWindowSize.height,
                                                             mScreenSize.width, mScreenSize.height);
        GeckoAppShell.sendEventToGecko(event);
    }

    public Bitmap getBitmap() {
        return null;
    }

    void viewportSizeChanged() {
        
        
        
        sendResizeEventIfNecessary(true);
        
        
        
        GeckoAppShell.viewSizeChanged();
    }

    private static RectF calculateDisplayPort(ImmutableViewportMetrics metrics) {
        float desiredXMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;
        float desiredYMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;

        
        
        

        
        float xBufferAmount = Math.min(desiredXMargins, metrics.pageSizeWidth - metrics.getWidth());
        
        
        float savedPixels = (desiredXMargins - xBufferAmount) * (metrics.getHeight() + desiredYMargins);
        float extraYAmount = (float)Math.floor(savedPixels / (metrics.getWidth() + xBufferAmount));
        float yBufferAmount = Math.min(desiredYMargins + extraYAmount, metrics.pageSizeHeight - metrics.getHeight());
        
        if (xBufferAmount == desiredXMargins && yBufferAmount < desiredYMargins) {
            savedPixels = (desiredYMargins - yBufferAmount) * (metrics.getWidth() + xBufferAmount);
            float extraXAmount = (float)Math.floor(savedPixels / (metrics.getHeight() + yBufferAmount));
            xBufferAmount = Math.min(xBufferAmount + extraXAmount, metrics.pageSizeWidth - metrics.getWidth());
        }

        
        
        
        
        
        float leftMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.viewportRectLeft);
        float rightMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.pageSizeWidth - (metrics.viewportRectLeft + metrics.getWidth()));
        if (leftMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            rightMargin = xBufferAmount - leftMargin;
        } else if (rightMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            leftMargin = xBufferAmount - rightMargin;
        } else if (!FloatUtils.fuzzyEquals(leftMargin + rightMargin, xBufferAmount)) {
            float delta = xBufferAmount - leftMargin - rightMargin;
            leftMargin += delta / 2;
            rightMargin += delta / 2;
        }

        float topMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.viewportRectTop);
        float bottomMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.pageSizeHeight - (metrics.viewportRectTop + metrics.getHeight()));
        if (topMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            bottomMargin = yBufferAmount - topMargin;
        } else if (bottomMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
            topMargin = yBufferAmount - bottomMargin;
        } else if (!FloatUtils.fuzzyEquals(topMargin + bottomMargin, yBufferAmount)) {
            float delta = yBufferAmount - topMargin - bottomMargin;
            topMargin += delta / 2;
            bottomMargin += delta / 2;
        }

        
        
        
        
        return new RectF(metrics.viewportRectLeft - leftMargin,
                         metrics.viewportRectTop - topMargin,
                         metrics.viewportRectRight + rightMargin,
                         metrics.viewportRectBottom + bottomMargin);
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(mLayerController.getViewportMetrics());

        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        mDisplayPort = calculateDisplayPort(mLayerController.getViewportMetrics());
        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(viewportMetrics, mDisplayPort));
        mGeckoViewport = viewportMetrics;
    }

    
    public void handleMessage(String event, JSONObject message) {
        try {
            if ("Viewport:Update".equals(event)) {
                final ViewportMetrics newMetrics = new ViewportMetrics(message);
                synchronized (mLayerController) {
                    
                    ImmutableViewportMetrics oldMetrics = mLayerController.getViewportMetrics();
                    newMetrics.setSize(oldMetrics.getSize());
                    mLayerController.post(new Runnable() {
                        public void run() {
                            mGeckoViewport = newMetrics;
                        }
                    });
                    mLayerController.setViewportMetrics(newMetrics);
                    mLayerController.abortPanZoomAnimation();
                    mDisplayPort = calculateDisplayPort(mLayerController.getViewportMetrics());
                    mReturnDisplayPort = mDisplayPort;
                }
            } else if ("Viewport:CalculateDisplayPort".equals(event)) {
                ImmutableViewportMetrics newMetrics = new ImmutableViewportMetrics(new ViewportMetrics(message));
                mReturnDisplayPort = calculateDisplayPort(newMetrics);
            } else if ("Checkerboard:Toggle".equals(event)) {
                try {
                    boolean showChecks = message.getBoolean("value");
                    mLayerController.setCheckerboardShowChecks(showChecks);
                    Log.i(LOGTAG, "Showing checks: " + showChecks);
                } catch(JSONException ex) {
                    Log.e(LOGTAG, "Error decoding JSON", ex);
                }
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Unable to create viewport metrics in " + event + " handler", e);
        }
    }

    
    public String getResponse() {
        
        
        
        
        
        if (mReturnDisplayPort == null) {
            return "";
        }
        try {
            return RectUtils.toJSON(mReturnDisplayPort);
        } finally {
            mReturnDisplayPort = null;
        }
    }

    void geometryChanged() {
        
        sendResizeEventIfNecessary(false);
        if (mLayerController.getRedrawHint())
            adjustViewport();
    }

    







    public ViewportMetrics getGeckoViewportMetrics() {
        return mGeckoViewport;
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

    








    public ViewTransform syncViewportInfo(int x, int y, int width, int height, float resolution, boolean layersUpdated) {
        
        
        
        
        
        
        
        mFrameMetrics = mLayerController.getViewportMetrics();

        mCurrentViewTransform.x = mFrameMetrics.viewportRectLeft;
        mCurrentViewTransform.y = mFrameMetrics.viewportRectTop;
        mCurrentViewTransform.scale = mFrameMetrics.zoomFactor;

        mRootLayer.setPositionAndResolution(x, y, x + width, y + height, resolution);

        if (layersUpdated && mDrawListener != null) {
            
            mDrawListener.drawFinished();
        }

        return mCurrentViewTransform;
    }

    
    public LayerRenderer.Frame createFrame() {
        
        if (!mLayerRendererInitialized) {
            mLayerRenderer.checkMonitoringEnabled();
            mLayerRenderer.createDefaultProgram();
            mLayerRendererInitialized = true;
        }

        
        Layer.RenderContext pageContext = mLayerRenderer.createPageContext(mFrameMetrics);
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

