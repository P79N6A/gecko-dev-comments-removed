




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.ScreenshotHandler;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.ui.PanZoomTarget;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.GeckoEventResponder;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
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

    private final PanZoomController mPanZoomController;
    private LayerView mView;

    public GeckoLayerClient(Context context, LayerView view, EventDispatcher eventDispatcher) {
        
        
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

        mPanZoomController = new PanZoomController(this, mEventDispatcher);
        mView = view;
    }

    
    public void notifyGeckoReady() {
        mGeckoIsReady = true;

        mRootLayer = new VirtualLayer(new IntSize(mView.getWidth(), mView.getHeight()));
        mLayerRenderer = new LayerRenderer(mView);

        registerEventListener("Checkerboard:Toggle");

        mView.setListener(this);
        mView.setLayerRenderer(mLayerRenderer);

        sendResizeEventIfNecessary(true);

        DisplayPortCalculator.initPrefs();
        PluginLayer.initPrefs();
    }

    public void destroy() {
        mPanZoomController.destroy();
        unregisterEventListener("Checkerboard:Toggle");
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

    PanZoomController getPanZoomController() {
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

    
    private DisplayPortMetrics handleViewportMessage(ViewportMetrics messageMetrics, ViewportMessageType type) {
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
            setViewportMetrics(newMetrics, type == ViewportMessageType.UPDATE);
            mDisplayPort = DisplayPortCalculator.calculate(getViewportMetrics(), null);
        }
        return mDisplayPort;
    }

    public DisplayPortMetrics getDisplayPort(boolean pageSizeUpdate, boolean isBrowserContentDisplayed, int tabId, ViewportMetrics metrics) {
        Tabs tabs = Tabs.getInstance();
        if (tabs.isSelectedTab(tabs.getTab(tabId)) && isBrowserContentDisplayed) {
            
            
            
            return handleViewportMessage(metrics, pageSizeUpdate ? ViewportMessageType.PAGE_SIZE : ViewportMessageType.UPDATE);
        } else {
            
            
            
            
            ImmutableViewportMetrics newMetrics = new ImmutableViewportMetrics(metrics);
            return DisplayPortCalculator.calculate(newMetrics, null);
        }
    }

    
    
    
    
    
    public boolean shouldAbortProgressiveUpdate(boolean aHasPendingNewThebesContent,
                                                float x, float y, float width, float height, float resolution) {
        
        
        DisplayPortMetrics displayPort = mDisplayPort;

        
        
        if (!FloatUtils.fuzzyEquals(resolution, displayPort.resolution)) {
            Log.d(LOGTAG, "Aborting draw due to resolution change");
            return true;
        }

        
        
        
        

        
        
        
        
        if (Math.abs(displayPort.getLeft() - x) <= 1 &&
            Math.abs(displayPort.getTop() - y) <= 1 &&
            Math.abs(displayPort.getBottom() - (y + height)) <= 1 &&
            Math.abs(displayPort.getRight() - (x + width)) <= 1) {
            return false;
        }

        
        
        
        
        
        
        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        if (Math.max(viewportMetrics.viewportRectLeft, viewportMetrics.pageRectLeft) + 1 < x ||
            Math.max(viewportMetrics.viewportRectTop, viewportMetrics.pageRectTop) + 1 < y ||
            Math.min(viewportMetrics.viewportRectRight, viewportMetrics.pageRectRight) - 1 > x + width ||
            Math.min(viewportMetrics.viewportRectBottom, viewportMetrics.pageRectBottom) - 1 > y + height) {
            Log.d(LOGTAG, "Aborting update due to viewport not in display-port");
            return true;
        }

        
        
        
        
        
        
        if (!aHasPendingNewThebesContent) {
            Log.d(LOGTAG, "Aborting update due to more relevant display-port in event queue");
            return true;
        }

        return false;
    }

    
    public void handleMessage(String event, JSONObject message) {
        try {
            if ("Checkerboard:Toggle".equals(event)) {
                mView.setCheckerboardShouldShowChecks(message.getBoolean("value"));
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

    void setZoomConstraints(ZoomConstraints constraints) {
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
            mView.setCheckerboardColor(tab.getCheckerboardColor());
            setZoomConstraints(tab.getZoomConstraints());

            
            
            
            
            
            
            
            abortPanZoomAnimation();
        }
        DisplayPortCalculator.resetPageState();
        mDrawTimingQueue.reset();
        mView.getRenderer().resetCheckerboard();
        ScreenshotHandler.screenshotWholePage(Tabs.getInstance().getSelectedTab());
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
        setViewportMetrics(viewport, true);
    }

    private void setViewportMetrics(ViewportMetrics viewport, boolean notifyGecko) {
        mViewportMetrics = new ImmutableViewportMetrics(viewport);
        mView.requestRender();
        if (notifyGecko && mGeckoIsReady) {
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
