




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.ui.PanZoomTarget;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventResponder;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

public class GeckoLayerClient
        implements GeckoEventResponder, LayerView.Listener, PanZoomTarget
{
    private static final String LOGTAG = "GeckoLayerClient";

    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    private final EventDispatcher mEventDispatcher;
    private Context mContext;
    private IntSize mScreenSize;
    private IntSize mWindowSize;
    private DisplayPortMetrics mDisplayPort;
    private DisplayPortMetrics mReturnDisplayPort;

    private boolean mRecordDrawTimes;
    private final DrawTimingQueue mDrawTimingQueue;

    private VirtualLayer mRootLayer;

    







    private ViewportMetrics mGeckoViewport;

    



    private ImmutableViewportMetrics mFrameMetrics;

    
    private DrawListener mDrawListener;

    
    private final ViewTransform mCurrentViewTransform;

    
    private volatile boolean mCompositorCreated;

    private boolean mForceRedraw;

    











    private volatile ImmutableViewportMetrics mViewportMetrics;

    private ZoomConstraints mZoomConstraints;

    private boolean mGeckoIsReady;

    
    private int mCheckerboardColor;
    private boolean mCheckerboardShouldShowChecks;

    private final PanZoomController mPanZoomController;
    private LayerView mView;

    public GeckoLayerClient(Context context, EventDispatcher eventDispatcher) {
        
        
        mEventDispatcher = eventDispatcher;
        mContext = context;
        mScreenSize = new IntSize(0, 0);
        mWindowSize = new IntSize(0, 0);
        mDisplayPort = new DisplayPortMetrics();
        mRecordDrawTimes = true;
        mDrawTimingQueue = new DrawTimingQueue();
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
        mCompositorCreated = false;

        mForceRedraw = true;
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        mViewportMetrics = new ImmutableViewportMetrics(new ViewportMetrics(displayMetrics));
        mZoomConstraints = new ZoomConstraints(false);
        mCheckerboardColor = Color.WHITE;
        mCheckerboardShouldShowChecks = true;

        mPanZoomController = new PanZoomController(this, mEventDispatcher);
    }

    public void setView(LayerView v) {
        mView = v;
        mView.connect(this);
    }

    
    public void notifyGeckoReady() {
        mGeckoIsReady = true;

        mRootLayer = new VirtualLayer(new IntSize(mView.getWidth(), mView.getHeight()));
        mLayerRenderer = new LayerRenderer(mView);

        registerEventListener("Viewport:Update");
        registerEventListener("Viewport:PageSize");
        registerEventListener("Viewport:CalculateDisplayPort");
        registerEventListener("Checkerboard:Toggle");
        registerEventListener("Preferences:Data");

        mView.setListener(this);
        mView.setLayerRenderer(mLayerRenderer);

        sendResizeEventIfNecessary(true);

        JSONArray prefs = new JSONArray();
        DisplayPortCalculator.addPrefNames(prefs);
        PluginLayer.addPrefNames(prefs);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Preferences:Get", prefs.toString()));
    }

    public void destroy() {
        mPanZoomController.destroy();
        unregisterEventListener("Viewport:Update");
        unregisterEventListener("Viewport:PageSize");
        unregisterEventListener("Viewport:CalculateDisplayPort");
        unregisterEventListener("Checkerboard:Toggle");
        unregisterEventListener("Preferences:Data");
    }

    private void registerEventListener(String event) {
        mEventDispatcher.registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        mEventDispatcher.unregisterEventListener(event, this);
    }

    



    private boolean getRedrawHint() {
        if (mForceRedraw) {
            mForceRedraw = false;
            return true;
        }

        if (!mPanZoomController.getRedrawHint()) {
            return false;
        }

        return DisplayPortCalculator.aboutToCheckerboard(mViewportMetrics,
                mPanZoomController.getVelocityVector(), mDisplayPort);
    }

    Layer getRoot() {
        return mGeckoIsReady ? mRootLayer : null;
    }

    public LayerView getView() {
        return mView;
    }

    public FloatSize getViewportSize() {
        return mViewportMetrics.getSize();
    }

    







    void setViewportSize(FloatSize size) {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.setSize(size);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        if (mGeckoIsReady) {
            
            
            
            sendResizeEventIfNecessary(true);
            
            
            
            GeckoAppShell.viewSizeChanged();
        }
    }

    public PanZoomController getPanZoomController() {
        return mPanZoomController;
    }

    
    private void sendResizeEventIfNecessary(boolean force) {
        DisplayMetrics metrics = mContext.getResources().getDisplayMetrics();

        IntSize newScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        IntSize newWindowSize = new IntSize(mView.getWidth(), mView.getHeight());

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
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Window:Resize", ""));
    }

    
    private void setPageRect(RectF rect, RectF cssRect) {
        
        
        
        if (mViewportMetrics.getCssPageRect().equals(cssRect))
            return;

        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.setPageRect(rect, cssRect);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        
        

        post(new Runnable() {
            public void run() {
                mPanZoomController.pageRectUpdated();
                mView.requestRender();
            }
        });
    }

    private void adjustViewport(DisplayPortMetrics displayPort) {
        ImmutableViewportMetrics metrics = getViewportMetrics();

        ViewportMetrics clampedMetrics = new ViewportMetrics(metrics);
        clampedMetrics.setViewport(clampedMetrics.getClampedViewport());

        if (displayPort == null) {
            displayPort = DisplayPortCalculator.calculate(metrics, mPanZoomController.getVelocityVector());
        }

        mDisplayPort = displayPort;
        mGeckoViewport = clampedMetrics;

        if (mRecordDrawTimes) {
            mDrawTimingQueue.add(displayPort);
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(clampedMetrics, displayPort));
    }

    
    private void abortPanZoomAnimation() {
        if (mPanZoomController != null) {
            post(new Runnable() {
                public void run() {
                    mPanZoomController.abortAnimation();
                }
            });
        }
    }

    




    private enum ViewportMessageType {
        UPDATE,       
        PAGE_SIZE     
    }

    
    private void handleViewportMessage(JSONObject message, ViewportMessageType type) throws JSONException {
        ViewportMetrics messageMetrics = new ViewportMetrics(message);
        synchronized (this) {
            final ViewportMetrics newMetrics;
            ImmutableViewportMetrics oldMetrics = getViewportMetrics();

            switch (type) {
            default:
            case UPDATE:
                newMetrics = messageMetrics;
                
                newMetrics.setSize(oldMetrics.getSize());
                abortPanZoomAnimation();
                break;
            case PAGE_SIZE:
                
                
                
                float scaleFactor = oldMetrics.zoomFactor / messageMetrics.getZoomFactor();
                newMetrics = new ViewportMetrics(oldMetrics);
                newMetrics.setPageRect(RectUtils.scale(messageMetrics.getPageRect(), scaleFactor), messageMetrics.getCssPageRect());
                break;
            }

            post(new Runnable() {
                public void run() {
                    mGeckoViewport = newMetrics;
                }
            });
            setViewportMetrics(newMetrics);
            mDisplayPort = DisplayPortCalculator.calculate(getViewportMetrics(), null);
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
                mCheckerboardShouldShowChecks = message.getBoolean("value");
                mView.requestRender();
                Log.i(LOGTAG, "Showing checks: " + mCheckerboardShouldShowChecks);
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
                
                
                
                if (DisplayPortCalculator.setStrategy(prefValues) && PluginLayer.setUsePlaceholder(prefValues)) {
                    unregisterEventListener("Preferences:Data");
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

    boolean checkerboardShouldShowChecks() {
        return mCheckerboardShouldShowChecks;
    }

    int getCheckerboardColor() {
        return mCheckerboardColor;
    }

    public void setCheckerboardColor(int newColor) {
        mCheckerboardColor = newColor;
        mView.requestRender();
    }

    public void setZoomConstraints(ZoomConstraints constraints) {
        mZoomConstraints = constraints;
    }

    






    public void setFirstPaintViewport(float offsetX, float offsetY, float zoom,
            float pageLeft, float pageTop, float pageRight, float pageBottom,
            float cssPageLeft, float cssPageTop, float cssPageRight, float cssPageBottom) {
        synchronized (this) {
            final ViewportMetrics currentMetrics = new ViewportMetrics(getViewportMetrics());
            currentMetrics.setOrigin(new PointF(offsetX, offsetY));
            currentMetrics.setZoomFactor(zoom);
            currentMetrics.setPageRect(new RectF(pageLeft, pageTop, pageRight, pageBottom),
                                       new RectF(cssPageLeft, cssPageTop, cssPageRight, cssPageBottom));
            
            
            
            
            post(new Runnable() {
                public void run() {
                    mGeckoViewport = currentMetrics;
                }
            });
            setViewportMetrics(currentMetrics);

            Tab tab = Tabs.getInstance().getSelectedTab();
            setCheckerboardColor(tab.getCheckerboardColor());
            setZoomConstraints(tab.getZoomConstraints());

            
            
            
            
            
            
            
            abortPanZoomAnimation();
            mView.setPaintState(LayerView.PAINT_BEFORE_FIRST);
        }
        DisplayPortCalculator.resetPageState();
        mDrawTimingQueue.reset();
        mView.getRenderer().resetCheckerboard();
        GeckoAppShell.screenshotWholePage(Tabs.getInstance().getSelectedTab());
    }

    





    public void setPageRect(float cssPageLeft, float cssPageTop, float cssPageRight, float cssPageBottom) {
        synchronized (this) {
            RectF cssPageRect = new RectF(cssPageLeft, cssPageTop, cssPageRight, cssPageBottom);
            float ourZoom = getViewportMetrics().zoomFactor;
            setPageRect(RectUtils.scale(cssPageRect, ourZoom), cssPageRect);
            
            
            
            
        }
    }

    








    public ViewTransform syncViewportInfo(int x, int y, int width, int height, float resolution, boolean layersUpdated) {
        
        
        
        
        
        
        mFrameMetrics = getViewportMetrics();

        mCurrentViewTransform.x = mFrameMetrics.viewportRectLeft;
        mCurrentViewTransform.y = mFrameMetrics.viewportRectTop;
        mCurrentViewTransform.scale = mFrameMetrics.zoomFactor;

        mRootLayer.setPositionAndResolution(x, y, x + width, y + height, resolution);

        if (layersUpdated && mRecordDrawTimes) {
            
            
            
            DisplayPortMetrics drawn = new DisplayPortMetrics(x, y, x + width, y + height, resolution);
            long time = mDrawTimingQueue.findTimeFor(drawn);
            if (time >= 0) {
                long now = SystemClock.uptimeMillis();
                time = now - time;
                mRecordDrawTimes = DisplayPortCalculator.drawTimeUpdate(time, width * height);
            }
        }

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

    private void geometryChanged() {
        
        sendResizeEventIfNecessary(false);
        if (getRedrawHint()) {
            adjustViewport(null);
        }
    }

    
    public void renderRequested() {
        GeckoAppShell.scheduleComposite();
    }

    
    public void compositionPauseRequested() {
        
        
        
        
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createCompositorPauseEvent());
        }
    }

    
    public void compositionResumeRequested(int width, int height) {
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.scheduleResumeComposition(width, height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createCompositorResumeEvent());
        }
    }

    
    public void surfaceChanged(int width, int height) {
        setViewportSize(new FloatSize(width, height));

        
        
        
        compositionResumeRequested(width, height);
        renderRequested();
    }

    
    public void compositorCreated() {
        mCompositorCreated = true;
    }

    
    public ImmutableViewportMetrics getViewportMetrics() {
        return mViewportMetrics;
    }

    
    public ZoomConstraints getZoomConstraints() {
        return mZoomConstraints;
    }

    
    public void setAnimationTarget(ViewportMetrics viewport) {
        if (mGeckoIsReady) {
            
            
            
            
            ImmutableViewportMetrics metrics = new ImmutableViewportMetrics(viewport);
            DisplayPortMetrics displayPort = DisplayPortCalculator.calculate(metrics, null);
            adjustViewport(displayPort);
        }
    }

    


    public void setViewportMetrics(ViewportMetrics viewport) {
        mViewportMetrics = new ImmutableViewportMetrics(viewport);
        mView.requestRender();
        if (mGeckoIsReady) {
            geometryChanged();
        }
    }

    
    public void setForceRedraw() {
        mForceRedraw = true;
        if (mGeckoIsReady) {
            geometryChanged();
        }
    }

    
    public boolean post(Runnable action) {
        return mView.post(action);
    }

    
    public Object getLock() {
        return this;
    }

    






    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (!mGeckoIsReady) {
            return null;
        }

        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        PointF origin = viewportMetrics.getOrigin();
        float zoom = viewportMetrics.zoomFactor;
        ViewportMetrics geckoViewport = mGeckoViewport;
        PointF geckoOrigin = geckoViewport.getOrigin();
        float geckoZoom = geckoViewport.getZoomFactor();

        
        
        
        
        
        PointF layerPoint = new PointF(
                ((viewPoint.x + origin.x) / zoom) - (geckoOrigin.x / geckoZoom),
                ((viewPoint.y + origin.y) / zoom) - (geckoOrigin.y / geckoZoom));

        return layerPoint;
    }

    
    public void setDrawListener(DrawListener listener) {
        mDrawListener = listener;
    }

    
    public static interface DrawListener {
        public void drawFinished();
    }
}
