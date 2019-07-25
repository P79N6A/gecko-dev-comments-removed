





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import org.json.JSONException;
import org.json.JSONObject;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GeckoLayerClient implements GeckoEventListener,
                                         FlexibleGLSurfaceView.Listener {
    private static final String LOGTAG = "GeckoLayerClient";

    private LayerController mLayerController;
    private LayerRenderer mLayerRenderer;
    private boolean mLayerRendererInitialized;

    private IntSize mScreenSize;
    private IntSize mWindowSize;
    private IntSize mBufferSize;

    private Layer mTileLayer;

    
    private ViewportMetrics mGeckoViewport;

    
    private ViewportMetrics mNewGeckoViewport;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 25L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;
    private boolean mViewportSizeChanged;
    private boolean mIgnorePaintsPendingViewportSizeChange;
    private boolean mFirstPaint = true;

    
    
    
    
    
    private boolean mUpdateViewportOnEndDraw;

    private String mLastCheckerboardColor;

    private static Pattern sColorPattern;

    
    private DrawListener mDrawListener;

    public GeckoLayerClient(Context context) {
        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
    }

    
    void setLayerController(LayerController layerController) {
        mLayerController = layerController;

        layerController.setRoot(mTileLayer);
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

    
    public Rect beginDrawing(int width, int height, int tileWidth, int tileHeight,
                             String metadata, boolean hasDirectTexture) {
        Log.e(LOGTAG, "### beginDrawing " + width + " " + height + " " + tileWidth + " " +
              tileHeight + " " + hasDirectTexture);

        
        
        
        if (!mFirstPaint && mIgnorePaintsPendingViewportSizeChange) {
            return null;
        }
        mFirstPaint = false;

        
        if (handleDirectTextureChange(hasDirectTexture)) {
            Log.e(LOGTAG, "### Cancelling draw due to direct texture change");
            return null;
        }

        try {
            JSONObject viewportObject = new JSONObject(metadata);
            mNewGeckoViewport = new ViewportMetrics(viewportObject);

            Log.e(LOGTAG, "### beginDrawing new Gecko viewport " + mNewGeckoViewport);

            
            String backgroundColorString = viewportObject.optString("backgroundColor");
            if (backgroundColorString != null && !backgroundColorString.equals(mLastCheckerboardColor)) {
                mLastCheckerboardColor = backgroundColorString;
                mLayerController.setCheckerboardColor(parseColorFromGecko(backgroundColorString));
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Aborting draw, bad viewport description: " + metadata);
            return null;
        }


        
        
        Rect bufferRect = new Rect(0, 0, width, height);

        if (!mUpdateViewportOnEndDraw) {
            
            
            ViewportMetrics currentMetrics = mLayerController.getViewportMetrics();
            PointF currentBestOrigin = RectUtils.getOrigin(currentMetrics.getClampedViewport());

            Rect currentRect = RectUtils.round(new RectF(currentBestOrigin.x, currentBestOrigin.y,
                                                         currentBestOrigin.x + width, currentBestOrigin.y + height));

            
            PointF currentOrigin = mNewGeckoViewport.getDisplayportOrigin();
            bufferRect = RectUtils.round(new RectF(currentOrigin.x, currentOrigin.y,
                                                   currentOrigin.x + width, currentOrigin.y + height));

            int area = width * height;

            
            if (!bufferRect.intersect(currentRect)) {
                Log.w(LOGTAG, "Prediction would avoid useless paint of " + area + " pixels (100.0%)");
                
                
                mTileLayer.beginTransaction();
                try {
                    updateViewport(true);
                } finally {
                    mTileLayer.endTransaction();
                }
                return null;
            }

            int wasted = area - (bufferRect.width() * bufferRect.height());
            Log.w(LOGTAG, "Prediction would avoid useless paint of " + wasted + " pixels (" + ((float)wasted * 100.0f / area) + "%)");

            bufferRect.offset(Math.round(-currentOrigin.x), Math.round(-currentOrigin.y));
        }

        mTileLayer.beginTransaction();

        if (mBufferSize.width != width || mBufferSize.height != height) {
            mBufferSize = new IntSize(width, height);
        }

        return bufferRect;
    }

    




    public void endDrawing(int x, int y, int width, int height) {
        synchronized (mLayerController) {
            try {
                updateViewport(!mUpdateViewportOnEndDraw);
                mUpdateViewportOnEndDraw = false;
            } finally {
                mTileLayer.endTransaction();
            }
        }
        Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - endDrawing");

        
        if (mDrawListener != null) {
            mDrawListener.drawFinished(x, y, width, height);
        }
    }

    private void updateViewport(boolean onlyUpdatePageSize) {
        
        
        
        
        FloatSize viewportSize = mLayerController.getViewportSize();
        mGeckoViewport = mNewGeckoViewport;
        mGeckoViewport.setSize(viewportSize);

        PointF displayportOrigin = mGeckoViewport.getDisplayportOrigin();
        mTileLayer.setOrigin(PointUtils.round(displayportOrigin));
        mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

        
        mTileLayer.performUpdates(null);

        Log.e(LOGTAG, "### updateViewport onlyUpdatePageSize=" + onlyUpdatePageSize +
              " getTileViewport " + mGeckoViewport);

        if (onlyUpdatePageSize) {
            
            
            if (FloatUtils.fuzzyEquals(mLayerController.getZoomFactor(),
                    mGeckoViewport.getZoomFactor()))
                mLayerController.setPageSize(mGeckoViewport.getPageSize());
        } else {
            mLayerController.setViewportMetrics(mGeckoViewport);
            mLayerController.abortPanZoomAnimation();
        }
    }

    
    private void sendResizeEventIfNecessary(boolean force) {
        Log.d(LOGTAG, "### sendResizeEventIfNecessary " + force);

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

        IntSize bufferSize = getBufferSize();
        GeckoEvent event = GeckoEvent.createSizeChangedEvent(mWindowSize.width, mWindowSize.height, 
                                                             mScreenSize.width, mScreenSize.height, 
                                                             0, 0);                                 
        GeckoAppShell.sendEventToGecko(event);
    }

    
    
    private static int parseColorFromGecko(String string) {
        if (sColorPattern == null) {
            sColorPattern = Pattern.compile("rgb\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)");
        }

        Matcher matcher = sColorPattern.matcher(string);
        if (!matcher.matches()) {
            return Color.WHITE;
        }

        int r = Integer.parseInt(matcher.group(1));
        int g = Integer.parseInt(matcher.group(2));
        int b = Integer.parseInt(matcher.group(3));
        return Color.rgb(r, g, b);
    }

    private boolean handleDirectTextureChange(boolean hasDirectTexture) {
        if (mTileLayer != null) {
            return false;
        }

        Log.e(LOGTAG, "### Creating virtual layer");
        VirtualLayer virtualLayer = new VirtualLayer();
        virtualLayer.setSize(getBufferSize());
        mLayerController.setRoot(virtualLayer);
        mTileLayer = virtualLayer;

        sendResizeEventIfNecessary(true);
        return true;
    }

    private IntSize getBufferSize() {
        View view = mLayerController.getView();
        IntSize size = new IntSize(view.getWidth(), view.getHeight());
        Log.e(LOGTAG, "### getBufferSize " + size);
        return size;
    }

    public Bitmap getBitmap() {
        return null;
    }

    private void adjustViewportWithThrottling() {
        if (!mLayerController.getRedrawHint())
            return;

        if (mPendingViewportAdjust)
            return;

        long timeDelta = System.currentTimeMillis() - mLastViewportChangeTime;
        if (timeDelta < MIN_VIEWPORT_CHANGE_DELAY) {
            mLayerController.getView().postDelayed(
                new Runnable() {
                    public void run() {
                        mPendingViewportAdjust = false;
                        adjustViewport();
                    }
                }, MIN_VIEWPORT_CHANGE_DELAY - timeDelta);
            mPendingViewportAdjust = true;
            return;
        }

        adjustViewport();
    }

    void viewportSizeChanged() {
        mViewportSizeChanged = true;
        mIgnorePaintsPendingViewportSizeChange = true;
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(mLayerController.getViewportMetrics());

        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoAppShell.sendEventToGecko(GeckoEvent.createViewportEvent(viewportMetrics));
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }

        mLastViewportChangeTime = System.currentTimeMillis();
    }

    
    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:UpdateAndDraw".equals(event)) {
            Log.e(LOGTAG, "### Java side Viewport:UpdateAndDraw()!");
            mUpdateViewportOnEndDraw = true;
            mIgnorePaintsPendingViewportSizeChange = false;

            
            Rect rect = new Rect(0, 0, mBufferSize.width, mBufferSize.height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createDrawEvent(rect));
        } else if ("Viewport:UpdateLater".equals(event)) {
            Log.e(LOGTAG, "### Java side Viewport:UpdateLater()!");
            mUpdateViewportOnEndDraw = true;
            mIgnorePaintsPendingViewportSizeChange = false;
        }
    }

    void geometryChanged() {
        
        sendResizeEventIfNecessary(false);
        adjustViewportWithThrottling();
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

    
    public ViewTransform getViewTransform() {
        Log.e(LOGTAG, "### getViewTransform()");

        
        

        synchronized (mLayerController) {
            ViewportMetrics viewportMetrics = mLayerController.getViewportMetrics();
            PointF viewportOrigin = viewportMetrics.getOrigin();
            float scrollX = viewportOrigin.x; 
            float scrollY = viewportOrigin.y;
            float zoomFactor = viewportMetrics.getZoomFactor();
            Log.e(LOGTAG, "### Viewport metrics = " + viewportMetrics + " tile reso = " +
                  mTileLayer.getResolution());
            return new ViewTransform(scrollX, scrollY, zoomFactor);
        }
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
        mLayerController.setViewportSize(new FloatSize(width, height));
        compositionResumeRequested();
        renderRequested();
    }

    
    public void setDrawListener(DrawListener listener) {
        mDrawListener = listener;
    }

    
    public interface DrawListener {
        public void drawFinished(int x, int y, int width, int height);
    }
}

