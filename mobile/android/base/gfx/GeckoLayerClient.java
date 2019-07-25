





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventResponder;
import org.json.JSONArray;
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
import java.util.Map;
import java.util.HashMap;

public class GeckoLayerClient implements GeckoEventResponder,
                                         LayerView.Listener {
    private static final String LOGTAG = "GeckoLayerClient";

    private LayerController mLayerController;
    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    private IntSize mScreenSize;
    private IntSize mWindowSize;
    private DisplayPortMetrics mDisplayPort;
    private DisplayPortMetrics mReturnDisplayPort;

    private VirtualLayer mRootLayer;

    
    private ViewportMetrics mGeckoViewport;

    



    private ImmutableViewportMetrics mFrameMetrics;

    private String mLastCheckerboardColor;

    
    private DrawListener mDrawListener;

    
    private ViewTransform mCurrentViewTransform;

    public GeckoLayerClient(Context context) {
        
        
        mScreenSize = new IntSize(0, 0);
        mWindowSize = new IntSize(0, 0);
        mDisplayPort = new DisplayPortMetrics();
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
    }

    
    void setLayerController(LayerController layerController) {
        LayerView view = layerController.getView();

        mLayerController = layerController;

        mRootLayer = new VirtualLayer(new IntSize(view.getWidth(), view.getHeight()));
        mLayerRenderer = new LayerRenderer(view);

        GeckoAppShell.registerGeckoEventListener("Viewport:Update", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:PageSize", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:CalculateDisplayPort", this);
        GeckoAppShell.registerGeckoEventListener("Checkerboard:Toggle", this);
        GeckoAppShell.registerGeckoEventListener("Preferences:Data", this);

        view.setListener(this);
        view.setLayerRenderer(mLayerRenderer);
        layerController.setRoot(mRootLayer);

        sendResizeEventIfNecessary(true);

        JSONArray prefs = new JSONArray();
        DisplayPortCalculator.addPrefNames(prefs);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Preferences:Get", prefs.toString()));
    }

    DisplayPortMetrics getDisplayPort() {
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

    void adjustViewport(DisplayPortMetrics displayPort) {
        ImmutableViewportMetrics metrics = mLayerController.getViewportMetrics();

        ViewportMetrics clampedMetrics = new ViewportMetrics(metrics);
        clampedMetrics.setViewport(clampedMetrics.getClampedViewport());

        if (displayPort == null) {
            displayPort = DisplayPortCalculator.calculate(metrics,
                    mLayerController.getPanZoomController().getVelocityVector());
        }

        mDisplayPort = displayPort;
        mGeckoViewport = clampedMetrics;

        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(clampedMetrics, displayPort));
    }

    




    private enum ViewportMessageType {
        UPDATE,       
        PAGE_SIZE     
    }

    
    private void handleViewportMessage(JSONObject message, ViewportMessageType type) throws JSONException {
        ViewportMetrics messageMetrics = new ViewportMetrics(message);
        synchronized (mLayerController) {
            final ViewportMetrics newMetrics;
            ImmutableViewportMetrics oldMetrics = mLayerController.getViewportMetrics();

            switch (type) {
            default:
            case UPDATE:
                newMetrics = messageMetrics;
                
                newMetrics.setSize(oldMetrics.getSize());
                mLayerController.abortPanZoomAnimation();
                break;
            case PAGE_SIZE:
                
                
                
                float scaleFactor = oldMetrics.zoomFactor / messageMetrics.getZoomFactor();
                newMetrics = new ViewportMetrics(oldMetrics);
                newMetrics.setPageSize(messageMetrics.getPageSize().scale(scaleFactor));
                break;
            }

            mLayerController.post(new Runnable() {
                public void run() {
                    mGeckoViewport = newMetrics;
                }
            });
            mLayerController.setViewportMetrics(newMetrics);
            mDisplayPort = DisplayPortCalculator.calculate(mLayerController.getViewportMetrics(), null);
        }
        mReturnDisplayPort = mDisplayPort;
    }

    
    public void handleMessage(String event, JSONObject message) {
        try {
            if ("Viewport:Update".equals(event)) {
                handleViewportMessage(message, ViewportMessageType.UPDATE);
            } else if ("Viewport:PageSize".equals(event)) {
                handleViewportMessage(message, ViewportMessageType.PAGE_SIZE);
            } else if ("Viewport:CalculateDisplayPort".equals(event)) {
                ImmutableViewportMetrics newMetrics = new ImmutableViewportMetrics(new ViewportMetrics(message));
                mReturnDisplayPort = DisplayPortCalculator.calculate(newMetrics, null);
            } else if ("Checkerboard:Toggle".equals(event)) {
                boolean showChecks = message.getBoolean("value");
                mLayerController.setCheckerboardShowChecks(showChecks);
                Log.i(LOGTAG, "Showing checks: " + showChecks);
            } else if ("Preferences:Data".equals(event)) {
                JSONArray jsonPrefs = message.getJSONArray("preferences");
                Map<String, Integer> prefValues = new HashMap<String, Integer>();
                for (int i = jsonPrefs.length() - 1; i >= 0; i--) {
                    JSONObject pref = jsonPrefs.getJSONObject(i);
                    String name = pref.getString("name");
                    try {
                        prefValues.put(name, pref.getInt("value"));
                    } catch (JSONException je) {
                        
                        
                    }
                }
                
                
                
                if (DisplayPortCalculator.setStrategy(prefValues)) {
                    GeckoAppShell.unregisterGeckoEventListener("Preferences:Data", this);
                }
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error decoding JSON in " + event + " handler", e);
        }
    }

    
    public String getResponse() {
        
        
        
        
        
        if (mReturnDisplayPort == null) {
            return "";
        }
        try {
            return mReturnDisplayPort.toJSON();
        } finally {
            mReturnDisplayPort = null;
        }
    }

    void geometryChanged() {
        
        sendResizeEventIfNecessary(false);
        if (mLayerController.getRedrawHint())
            adjustViewport(null);
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
            mLayerController.getView().setPaintState(LayerView.PAINT_BEFORE_FIRST);
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

        return mLayerRenderer.createFrame(mFrameMetrics);
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
        
        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createCompositorPauseEvent());
    }

    
    public void compositionResumeRequested() {
        
        
        
        
        GeckoAppShell.scheduleResumeComposition();
        GeckoAppShell.sendEventToGecko(GeckoEvent.createCompositorResumeEvent());
    }

    
    public void surfaceChanged(int width, int height) {
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

