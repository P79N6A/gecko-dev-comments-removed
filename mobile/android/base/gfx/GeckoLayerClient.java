




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;

public class GeckoLayerClient implements LayerView.Listener, PanZoomTarget
{
    private static final String LOGTAG = "GeckoLayerClient";

    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    private Context mContext;
    private IntSize mScreenSize;
    private IntSize mWindowSize;
    private DisplayPortMetrics mDisplayPort;

    private boolean mRecordDrawTimes;
    private final DrawTimingQueue mDrawTimingQueue;

    private VirtualLayer mRootLayer;

    







    private ImmutableViewportMetrics mGeckoViewport;

    



    private ImmutableViewportMetrics mFrameMetrics;

    
    private DrawListener mDrawListener;

    
    private final ViewTransform mCurrentViewTransform;
    private final RectF mCurrentViewTransformMargins;

    
    private final ProgressiveUpdateData mProgressiveUpdateData;
    private DisplayPortMetrics mProgressiveUpdateDisplayPort;
    private boolean mLastProgressiveUpdateWasLowPrecision;
    private boolean mProgressiveUpdateWasInDanger;

    private boolean mForceRedraw;

    











    private volatile ImmutableViewportMetrics mViewportMetrics;
    private OnMetricsChangedListener mViewportChangeListener;

    private ZoomConstraints mZoomConstraints;

    private boolean mGeckoIsReady;

    private final PanZoomController mPanZoomController;
    private final LayerMarginsAnimator mMarginsAnimator;
    private LayerView mView;

    






    private volatile boolean mContentDocumentIsDisplayed;

    public GeckoLayerClient(Context context, LayerView view, EventDispatcher eventDispatcher) {
        
        
        mContext = context;
        mScreenSize = new IntSize(0, 0);
        mWindowSize = new IntSize(0, 0);
        mDisplayPort = new DisplayPortMetrics();
        mRecordDrawTimes = true;
        mDrawTimingQueue = new DrawTimingQueue();
        mCurrentViewTransform = new ViewTransform(0, 0, 1);
        mCurrentViewTransformMargins = new RectF();
        mProgressiveUpdateData = new ProgressiveUpdateData();
        mProgressiveUpdateDisplayPort = new DisplayPortMetrics();
        mLastProgressiveUpdateWasLowPrecision = false;
        mProgressiveUpdateWasInDanger = false;

        mForceRedraw = true;
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        mViewportMetrics = new ImmutableViewportMetrics(displayMetrics)
                           .setViewportSize(view.getWidth(), view.getHeight());
        mFrameMetrics = mViewportMetrics;
        mZoomConstraints = new ZoomConstraints(false);

        mPanZoomController = PanZoomController.Factory.create(this, view, eventDispatcher);
        mMarginsAnimator = new LayerMarginsAnimator(this, view);
        mView = view;
        mView.setListener(this);
        mContentDocumentIsDisplayed = true;
    }

    
    public void notifyGeckoReady() {
        mGeckoIsReady = true;

        mRootLayer = new VirtualLayer(new IntSize(mView.getWidth(), mView.getHeight()));
        mLayerRenderer = mView.getRenderer();

        sendResizeEventIfNecessary(true);

        DisplayPortCalculator.initPrefs();

        
        
        
        
        
        mView.post(new Runnable() {
            @Override
            public void run() {
                mView.getGLController().createCompositor();
            }
        });
    }

    public void destroy() {
        mPanZoomController.destroy();
        mMarginsAnimator.destroy();
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

    







    void setViewportSize(int width, int height) {
        mViewportMetrics = mViewportMetrics.setViewportSize(width, height);

        if (mGeckoIsReady) {
            
            
            
            sendResizeEventIfNecessary(true);
            
            
            
            GeckoAppShell.viewSizeChanged();
        }
    }

    PanZoomController getPanZoomController() {
        return mPanZoomController;
    }

    LayerMarginsAnimator getLayerMarginsAnimator() {
        return mMarginsAnimator;
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

        mViewportMetrics = mViewportMetrics.setPageRect(rect, cssRect);

        
        

        post(new Runnable() {
            @Override
            public void run() {
                mPanZoomController.pageRectUpdated();
                mView.requestRender();
            }
        });
    }

    



    private void getFixedMargins(ImmutableViewportMetrics metrics, RectF fixedMargins) {
        fixedMargins.left = 0;
        fixedMargins.top = 0;
        fixedMargins.right = 0;
        fixedMargins.bottom = 0;

        
        float maxMarginWidth = Math.max(0, metrics.getPageWidth() - metrics.getWidthWithoutMargins());
        float maxMarginHeight = Math.max(0, metrics.getPageHeight() - metrics.getHeightWithoutMargins());

        PointF offset = metrics.getMarginOffset();
        RectF overscroll = metrics.getOverscroll();
        if (offset.x >= 0) {
            fixedMargins.right = Math.max(0, Math.min(offset.x - overscroll.right, maxMarginWidth));
        } else {
            fixedMargins.left = Math.max(0, Math.min(-offset.x - overscroll.left, maxMarginWidth));
        }
        if (offset.y >= 0) {
            fixedMargins.bottom = Math.max(0, Math.min(offset.y - overscroll.bottom, maxMarginHeight));
        } else {
            fixedMargins.top = Math.max(0, Math.min(-offset.y - overscroll.top, maxMarginHeight));
        }

        
        
        
        if (overscroll.left > 0) {
            fixedMargins.right = Math.min(maxMarginWidth - fixedMargins.left,
                                          fixedMargins.right + overscroll.left);
        } else if (overscroll.right > 0) {
            fixedMargins.left = Math.min(maxMarginWidth - fixedMargins.right,
                                         fixedMargins.left + overscroll.right);
        }
        if (overscroll.top > 0) {
            fixedMargins.bottom = Math.min(maxMarginHeight - fixedMargins.top,
                                           fixedMargins.bottom + overscroll.top);
        } else if (overscroll.bottom > 0) {
            fixedMargins.top = Math.min(maxMarginHeight - fixedMargins.bottom,
                                        fixedMargins.top + overscroll.bottom);
        }
    }

    private void adjustViewport(DisplayPortMetrics displayPort) {
        ImmutableViewportMetrics metrics = getViewportMetrics();
        ImmutableViewportMetrics clampedMetrics = metrics.clamp();

        RectF margins = new RectF();
        getFixedMargins(metrics, margins);
        clampedMetrics = clampedMetrics.setMargins(
            margins.left, margins.top, margins.right, margins.bottom);

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
                @Override
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

    
    private DisplayPortMetrics handleViewportMessage(ImmutableViewportMetrics messageMetrics, ViewportMessageType type) {
        synchronized (getLock()) {
            ImmutableViewportMetrics newMetrics;
            ImmutableViewportMetrics oldMetrics = getViewportMetrics();

            switch (type) {
            default:
            case UPDATE:
                
                newMetrics = messageMetrics.setViewportSize(oldMetrics.getWidth(), oldMetrics.getHeight());
                if (!oldMetrics.fuzzyEquals(newMetrics)) {
                    abortPanZoomAnimation();
                }
                break;
            case PAGE_SIZE:
                
                
                
                float scaleFactor = oldMetrics.zoomFactor / messageMetrics.zoomFactor;
                newMetrics = oldMetrics.setPageRect(RectUtils.scale(messageMetrics.getPageRect(), scaleFactor), messageMetrics.getCssPageRect());
                break;
            }

            
            
            final ImmutableViewportMetrics geckoMetrics = newMetrics.clamp();
            post(new Runnable() {
                @Override
                public void run() {
                    mGeckoViewport = geckoMetrics;
                }
            });

            setViewportMetrics(newMetrics, type == ViewportMessageType.UPDATE);
            mDisplayPort = DisplayPortCalculator.calculate(getViewportMetrics(), null);
        }
        return mDisplayPort;
    }

    
    DisplayPortMetrics getDisplayPort(boolean pageSizeUpdate, boolean isBrowserContentDisplayed, int tabId, ImmutableViewportMetrics metrics) {
        Tabs tabs = Tabs.getInstance();
        if (tabs.isSelectedTab(tabs.getTab(tabId)) && isBrowserContentDisplayed) {
            
            
            
            return handleViewportMessage(metrics, pageSizeUpdate ? ViewportMessageType.PAGE_SIZE : ViewportMessageType.UPDATE);
        } else {
            
            
            
            
            return DisplayPortCalculator.calculate(metrics, null);
        }
    }

    
    void contentDocumentChanged() {
        mContentDocumentIsDisplayed = false;
    }

    
    boolean isContentDocumentDisplayed() {
        return mContentDocumentIsDisplayed;
    }

    
    
    
    
    
    public ProgressiveUpdateData progressiveUpdateCallback(boolean aHasPendingNewThebesContent,
                                                           float x, float y, float width, float height,
                                                           float resolution, boolean lowPrecision) {
        
        
        if (lowPrecision && !mLastProgressiveUpdateWasLowPrecision) {
            
            if (!mProgressiveUpdateWasInDanger) {
                mProgressiveUpdateData.abort = true;
                return mProgressiveUpdateData;
            }
            mProgressiveUpdateWasInDanger = false;
        }
        mLastProgressiveUpdateWasLowPrecision = lowPrecision;

        
        
        DisplayPortMetrics displayPort = mDisplayPort;
        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        mProgressiveUpdateData.setViewport(viewportMetrics);
        mProgressiveUpdateData.abort = false;

        
        
        if (!FloatUtils.fuzzyEquals(resolution, viewportMetrics.zoomFactor)) {
            Log.d(LOGTAG, "Aborting draw due to resolution change");
            mProgressiveUpdateData.abort = true;
            return mProgressiveUpdateData;
        }

        
        
        if (!lowPrecision) {
            if (!FloatUtils.fuzzyEquals(resolution, mProgressiveUpdateDisplayPort.resolution) ||
                !FloatUtils.fuzzyEquals(x, mProgressiveUpdateDisplayPort.getLeft()) ||
                !FloatUtils.fuzzyEquals(y, mProgressiveUpdateDisplayPort.getTop()) ||
                !FloatUtils.fuzzyEquals(x + width, mProgressiveUpdateDisplayPort.getRight()) ||
                !FloatUtils.fuzzyEquals(y + height, mProgressiveUpdateDisplayPort.getBottom())) {
                mProgressiveUpdateDisplayPort =
                    new DisplayPortMetrics(x, y, x+width, y+height, resolution);
            }
        }

        
        
        if (!lowPrecision && !mProgressiveUpdateWasInDanger) {
            if (DisplayPortCalculator.aboutToCheckerboard(viewportMetrics,
                  mPanZoomController.getVelocityVector(), mProgressiveUpdateDisplayPort)) {
                mProgressiveUpdateWasInDanger = true;
            }
        }

        
        
        
        
        

        
        
        
        
        if (Math.abs(displayPort.getLeft() - mProgressiveUpdateDisplayPort.getLeft()) <= 2 &&
            Math.abs(displayPort.getTop() - mProgressiveUpdateDisplayPort.getTop()) <= 2 &&
            Math.abs(displayPort.getBottom() - mProgressiveUpdateDisplayPort.getBottom()) <= 2 &&
            Math.abs(displayPort.getRight() - mProgressiveUpdateDisplayPort.getRight()) <= 2) {
            return mProgressiveUpdateData;
        }

        
        
        
        
        
        if (Math.max(viewportMetrics.viewportRectLeft, viewportMetrics.pageRectLeft) + 1 < x ||
            Math.max(viewportMetrics.viewportRectTop, viewportMetrics.pageRectTop) + 1 < y ||
            Math.min(viewportMetrics.viewportRectRight, viewportMetrics.pageRectRight) - 1 > x + width ||
            Math.min(viewportMetrics.viewportRectBottom, viewportMetrics.pageRectBottom) - 1 > y + height) {
            Log.d(LOGTAG, "Aborting update due to viewport not in display-port");
            mProgressiveUpdateData.abort = true;

            
            
            mProgressiveUpdateWasInDanger = true;

            return mProgressiveUpdateData;
        }

        
        
        if (lowPrecision && !aHasPendingNewThebesContent) {
          mProgressiveUpdateData.abort = true;
        }
        return mProgressiveUpdateData;
    }

    void setZoomConstraints(ZoomConstraints constraints) {
        mZoomConstraints = constraints;
    }

    void setIsRTL(boolean aIsRTL) {
        synchronized (getLock()) {
            ImmutableViewportMetrics newMetrics = getViewportMetrics().setIsRTL(aIsRTL);
            setViewportMetrics(newMetrics, false);
        }
    }

    






    public void setFirstPaintViewport(float offsetX, float offsetY, float zoom,
            float pageLeft, float pageTop, float pageRight, float pageBottom,
            float cssPageLeft, float cssPageTop, float cssPageRight, float cssPageBottom) {
        synchronized (getLock()) {
            ImmutableViewportMetrics currentMetrics = getViewportMetrics();

            Tab tab = Tabs.getInstance().getSelectedTab();

            final ImmutableViewportMetrics newMetrics = currentMetrics
                .setViewportOrigin(offsetX, offsetY)
                .setZoomFactor(zoom)
                .setPageRect(new RectF(pageLeft, pageTop, pageRight, pageBottom),
                             new RectF(cssPageLeft, cssPageTop, cssPageRight, cssPageBottom))
                .setIsRTL(tab.getIsRTL());
            
            
            
            
            post(new Runnable() {
                @Override
                public void run() {
                    mGeckoViewport = newMetrics;
                }
            });

            setViewportMetrics(newMetrics);

            mView.setBackgroundColor(tab.getBackgroundColor());
            setZoomConstraints(tab.getZoomConstraints());

            
            
            
            
            
            
            
            abortPanZoomAnimation();

            
            
            if (mView.getPaintState() == LayerView.PAINT_START) {
                mView.setPaintState(LayerView.PAINT_BEFORE_FIRST);
            }
        }
        DisplayPortCalculator.resetPageState();
        mDrawTimingQueue.reset();

        mContentDocumentIsDisplayed = true;
    }

    





    public void setPageRect(float cssPageLeft, float cssPageTop, float cssPageRight, float cssPageBottom) {
        synchronized (getLock()) {
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

        
        getFixedMargins(mFrameMetrics, mCurrentViewTransformMargins);
        mCurrentViewTransform.fixedLayerMarginLeft = mCurrentViewTransformMargins.left;
        mCurrentViewTransform.fixedLayerMarginTop = mCurrentViewTransformMargins.top;
        mCurrentViewTransform.fixedLayerMarginRight = mCurrentViewTransformMargins.right;
        mCurrentViewTransform.fixedLayerMarginBottom = mCurrentViewTransformMargins.bottom;

        
        PointF offset = mFrameMetrics.getMarginOffset();
        mCurrentViewTransform.offsetX = offset.x;
        mCurrentViewTransform.offsetY = offset.y;

        mRootLayer.setPositionAndResolution(
            Math.round(x + mCurrentViewTransform.offsetX),
            Math.round(y + mCurrentViewTransform.offsetY),
            Math.round(x + width + mCurrentViewTransform.offsetX),
            Math.round(y + height + mCurrentViewTransform.offsetY),
            resolution);

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

    
    public ViewTransform syncFrameMetrics(float offsetX, float offsetY, float zoom,
                float cssPageLeft, float cssPageTop, float cssPageRight, float cssPageBottom,
                boolean layersUpdated, int x, int y, int width, int height, float resolution,
                boolean isFirstPaint)
    {
        if (isFirstPaint) {
            RectF pageRect = RectUtils.scale(new RectF(cssPageLeft, cssPageTop, cssPageRight, cssPageBottom), zoom);
            setFirstPaintViewport(offsetX, offsetY, zoom, pageRect.left, pageRect.top, pageRect.right,
                    pageRect.bottom, cssPageLeft, cssPageTop, cssPageRight, cssPageBottom);
        }

        return syncViewportInfo(x, y, width, height, resolution, layersUpdated);
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

    private void geometryChanged(DisplayPortMetrics displayPort) {
        
        sendResizeEventIfNecessary(false);
        if (getRedrawHint()) {
            adjustViewport(displayPort);
        }
    }

    
    @Override
    public void renderRequested() {
        try {
            GeckoAppShell.scheduleComposite();
        } catch (UnsupportedOperationException uoe) {
            
            
            Log.d(LOGTAG, "Dropping renderRequested call before libxul load.");
        }
    }

    
    @Override
    public void sizeChanged(int width, int height) {
        
        
        
        mView.getGLController().resumeCompositor(mWindowSize.width, mWindowSize.height);
    }

    
    @Override
    public void surfaceChanged(int width, int height) {
        setViewportSize(width, height);
    }

    
    @Override
    public ImmutableViewportMetrics getViewportMetrics() {
        return mViewportMetrics;
    }

    
    @Override
    public ZoomConstraints getZoomConstraints() {
        return mZoomConstraints;
    }

    
    @Override
    public boolean isFullScreen() {
        return mView.isFullScreen();
    }

    
    @Override
    public void setAnimationTarget(ImmutableViewportMetrics metrics) {
        if (mGeckoIsReady) {
            
            
            
            
            DisplayPortMetrics displayPort = DisplayPortCalculator.calculate(metrics, null);
            adjustViewport(displayPort);
        }
    }

    


    @Override
    public void setViewportMetrics(ImmutableViewportMetrics metrics) {
        setViewportMetrics(metrics, true);
    }

    


    private void setViewportMetrics(ImmutableViewportMetrics metrics, boolean notifyGecko) {
        
        
        
        
        
        metrics = metrics.setViewportSize(mViewportMetrics.getWidth(), mViewportMetrics.getHeight());
        metrics = metrics.setMarginsFrom(mViewportMetrics);
        mViewportMetrics = metrics;

        viewportMetricsChanged(notifyGecko);
    }

    


    private void viewportMetricsChanged(boolean notifyGecko) {
        if (mViewportChangeListener != null) {
            mViewportChangeListener.onMetricsChanged(mViewportMetrics);
        }

        mView.requestRender();
        if (notifyGecko && mGeckoIsReady) {
            geometryChanged(null);
        }
        setShadowVisibility();
    }

    




    void forceViewportMetrics(ImmutableViewportMetrics metrics, boolean notifyGecko, boolean forceRedraw) {
        if (forceRedraw) {
            mForceRedraw = true;
        }
        mViewportMetrics = metrics;
        viewportMetricsChanged(notifyGecko);
    }

    








    @Override
    public void scrollBy(float dx, float dy) {
        
        mViewportMetrics = mMarginsAnimator.scrollBy(mViewportMetrics, dx, dy);
        viewportMetricsChanged(true);
    }

    
    @Override
    public void panZoomStopped() {
        if (mViewportChangeListener != null) {
            mViewportChangeListener.onPanZoomStopped();
        }
    }

    public interface OnMetricsChangedListener {
        public void onMetricsChanged(ImmutableViewportMetrics viewport);
        public void onPanZoomStopped();
    }

    private void setShadowVisibility() {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (BrowserApp.mBrowserToolbar == null) {
                    return;
                }
                ImmutableViewportMetrics m = mViewportMetrics;
                BrowserApp.mBrowserToolbar.setShadowVisibility(m.viewportRectTop >= m.pageRectTop);
            }
        });
    }

    
    @Override
    public void forceRedraw(DisplayPortMetrics displayPort) {
        mForceRedraw = true;
        if (mGeckoIsReady) {
            geometryChanged(displayPort);
        }
    }

    
    @Override
    public boolean post(Runnable action) {
        return mView.post(action);
    }

    
    @Override
    public boolean postDelayed(Runnable action, long delayMillis) {
        return mView.postDelayed(action, delayMillis);
    }

    
    @Override
    public Object getLock() {
        return this;
    }

    






    @Override
    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (!mGeckoIsReady) {
            return null;
        }

        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        PointF origin = viewportMetrics.getOrigin();
        PointF offset = viewportMetrics.getMarginOffset();
        origin.offset(-offset.x, -offset.y);
        float zoom = viewportMetrics.zoomFactor;
        ImmutableViewportMetrics geckoViewport = mGeckoViewport;
        PointF geckoOrigin = geckoViewport.getOrigin();
        float geckoZoom = geckoViewport.zoomFactor;

        
        
        
        
        
        PointF layerPoint = new PointF(
                ((viewPoint.x + origin.x) / zoom) - (geckoOrigin.x / geckoZoom),
                ((viewPoint.y + origin.y) / zoom) - (geckoOrigin.y / geckoZoom));

        return layerPoint;
    }

    public void setOnMetricsChangedListener(OnMetricsChangedListener listener) {
        mViewportChangeListener = listener;
    }

    
    public void setDrawListener(DrawListener listener) {
        mDrawListener = listener;
    }

    
    public static interface DrawListener {
        public void drawFinished();
    }
}
