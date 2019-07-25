





































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
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class GeckoLayerClient implements GeckoEventListener {
    private static final String LOGTAG = "GeckoLayerClient";

    protected LayerController mLayerController;

    protected IntSize mScreenSize;
    protected IntSize mBufferSize;

    protected Layer mTileLayer;

    
    protected ViewportMetrics mGeckoViewport;

    
    protected ViewportMetrics mNewGeckoViewport;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 25L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;
    private boolean mViewportSizeChanged;

    
    
    
    
    
    private boolean mUpdateViewportOnEndDraw;

    private String mLastCheckerboardColor;

    private static Pattern sColorPattern;

    
    private DrawListener mDrawListener;

    protected abstract boolean handleDirectTextureChange(boolean hasDirectTexture);
    protected abstract boolean shouldDrawProceed(int tileWidth, int tileHeight);
    protected abstract void updateLayerAfterDraw(Rect updatedRect);
    protected abstract IntSize getBufferSize();
    protected abstract IntSize getTileSize();
    protected abstract void tileLayerUpdated();
    public abstract Bitmap getBitmap();

    public GeckoLayerClient(Context context) {
        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
    }

    
    public void setLayerController(LayerController layerController) {
        mLayerController = layerController;

        layerController.setRoot(mTileLayer);
        if (mGeckoViewport != null) {
            layerController.setViewportMetrics(mGeckoViewport);
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateAndDraw", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateLater", this);

        sendResizeEventIfNecessary();
    }

    public Rect beginDrawing(int width, int height, int tileWidth, int tileHeight,
                             String metadata, boolean hasDirectTexture) {
        Log.e(LOGTAG, "### beginDrawing " + width + " " + height + " " + tileWidth + " " +
              tileHeight + " " + hasDirectTexture);

        
        if (handleDirectTextureChange(hasDirectTexture)) {
            Log.e(LOGTAG, "### Cancelling draw due to direct texture change");
            return null;
        }

        if (!shouldDrawProceed(tileWidth, tileHeight)) {
            Log.e(LOGTAG, "### Cancelling draw due to shouldDrawProceed()");
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
            PointF viewportOffset = currentMetrics.getOptimumViewportOffset(new IntSize(width, height));
            currentBestOrigin.offset(-viewportOffset.x, -viewportOffset.y);

            Rect currentRect = RectUtils.round(new RectF(currentBestOrigin.x, currentBestOrigin.y,
                                                         currentBestOrigin.x + width, currentBestOrigin.y + height));

            
            PointF currentOrigin = mNewGeckoViewport.getDisplayportOrigin();
            bufferRect = RectUtils.round(new RectF(currentOrigin.x, currentOrigin.y,
                                                   currentOrigin.x + width, currentOrigin.y + height));


            
            if (!bufferRect.intersect(currentRect)) {
                
                
                mTileLayer.beginTransaction(mLayerController.getView());
                try {
                    updateViewport(true);
                } finally {
                    mTileLayer.endTransaction();
                }
                return null;
            }
            bufferRect.offset(Math.round(-currentOrigin.x), Math.round(-currentOrigin.y));
        }

        mTileLayer.beginTransaction(mLayerController.getView());
        return bufferRect;
    }

    



    public void endDrawing(int x, int y, int width, int height) {
        synchronized (mLayerController) {
            try {
                updateViewport(!mUpdateViewportOnEndDraw);
                mUpdateViewportOnEndDraw = false;

                Rect rect = new Rect(x, y, x + width, y + height);
                updateLayerAfterDraw(rect);
            } finally {
                mTileLayer.endTransaction();
            }
        }
        Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - endDrawing");

        
        if (mDrawListener != null) {
            mDrawListener.drawFinished(x, y, width, height);
        }
    }

    protected void updateViewport(boolean onlyUpdatePageSize) {
        
        
        
        
        FloatSize viewportSize = mLayerController.getViewportSize();
        mGeckoViewport = mNewGeckoViewport;
        mGeckoViewport.setSize(viewportSize);

        PointF displayportOrigin = mGeckoViewport.getDisplayportOrigin();
        mTileLayer.setOrigin(PointUtils.round(displayportOrigin));
        mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

        this.tileLayerUpdated();
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

    
    protected void sendResizeEventIfNecessary(boolean force) {
        Log.e(LOGTAG, "### sendResizeEventIfNecessary " + force);

        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        
        
        
        boolean screenSizeChanged = (metrics.widthPixels != mScreenSize.width ||
                                     metrics.heightPixels != mScreenSize.height);
        boolean viewportSizeValid = (mLayerController != null &&
                                     mLayerController.getViewportSize().isPositive());
        if (!(force || (screenSizeChanged && viewportSizeValid))) {
            return;
        }

        mScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        IntSize bufferSize = getBufferSize(), tileSize = getTileSize();

        Log.e(LOGTAG, "### Screen-size changed to " + mScreenSize);
        GeckoEvent event = GeckoEvent.createSizeChangedEvent(bufferSize.width, bufferSize.height,
                                                             metrics.widthPixels, metrics.heightPixels,
                                                             tileSize.width, tileSize.height);
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

    public void render() {
        adjustViewportWithThrottling();
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

    public void viewportSizeChanged() {
        mViewportSizeChanged = true;
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(mLayerController.getViewportMetrics());

        PointF viewportOffset = viewportMetrics.getOptimumViewportOffset(mBufferSize);
        viewportMetrics.setViewportOffset(viewportOffset);
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

            
            Rect rect = new Rect(0, 0, mBufferSize.width, mBufferSize.height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createDrawEvent(rect));
        } else if ("Viewport:UpdateLater".equals(event)) {
            Log.e(LOGTAG, "### Java side Viewport:UpdateLater()!");
            mUpdateViewportOnEndDraw = true;
        }
    }

    public void geometryChanged() {
        
        sendResizeEventIfNecessary();
        render();
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

    private void sendResizeEventIfNecessary() {
        sendResizeEventIfNecessary(false);
    }

    
    public void setDrawListener(DrawListener listener) {
        mDrawListener = listener;
    }

    
    public interface DrawListener {
        public void drawFinished(int x, int y, int width, int height);
    }
}

