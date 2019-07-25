





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.DisplayMetrics;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.nio.ByteBuffer;







public class GeckoSoftwareLayerClient extends LayerClient implements GeckoEventListener {
    private static final String LOGTAG = "GeckoSoftwareLayerClient";

    private Context mContext;
    private int mFormat;
    private IntSize mScreenSize, mViewportSize;
    private IntSize mBufferSize;
    private ByteBuffer mBuffer;
    private final SingleTileLayer mTileLayer;

    
    private ViewportMetrics mGeckoViewport;

    private CairoImage mCairoImage;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 350L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;
    private boolean mViewportSizeChanged;

    
    
    
    
    
    private boolean mUpdateViewportOnEndDraw;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
        mFormat = CairoImage.FORMAT_RGB16_565;

        mCairoImage = new CairoImage() {
            @Override
            public ByteBuffer getBuffer() { return mBuffer; }
            @Override
            public IntSize getSize() { return mBufferSize; }
            @Override
            public int getFormat() { return mFormat; }
        };

        mTileLayer = new SingleTileLayer(mCairoImage);
    }


    protected void finalize() throws Throwable {
        try {
            if (mBuffer != null)
                GeckoAppShell.freeDirectBuffer(mBuffer);
            mBuffer = null;
        } finally {
            super.finalize();
        }
    }

    
    @Override
    public void setLayerController(LayerController layerController) {
        super.setLayerController(layerController);

        layerController.setRoot(mTileLayer);
        if (mGeckoViewport != null) {
            layerController.setViewportMetrics(mGeckoViewport);
            layerController.notifyPanZoomControllerOfGeometryChange(false);
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:Update", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateLater", this);
    }

    public void beginDrawing() {
        beginTransaction(mTileLayer);
    }

    private void updateViewport(String viewportDescription, final boolean onlyUpdatePageSize) {
        try {
            JSONObject viewportObject = new JSONObject(viewportDescription);

            
            
            
            
            FloatSize viewportSize = getLayerController().getViewportSize();
            mGeckoViewport = new ViewportMetrics(viewportObject);
            mGeckoViewport.setSize(viewportSize);

            LayerController controller = getLayerController();
            synchronized (controller) {
                PointF displayportOrigin = mGeckoViewport.getDisplayportOrigin();
                mTileLayer.setOrigin(PointUtils.round(displayportOrigin));
                mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

                if (onlyUpdatePageSize) {
                    
                    
                    if (FloatUtils.fuzzyEquals(controller.getZoomFactor(),
                            mGeckoViewport.getZoomFactor()))
                        controller.setPageSize(mGeckoViewport.getPageSize());
                } else {
                    controller.setViewportMetrics(mGeckoViewport);
                    controller.notifyPanZoomControllerOfGeometryChange(true);
                }
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Bad viewport description: " + viewportDescription);
            throw new RuntimeException(e);
        }
    }

    



    public void endDrawing(int x, int y, int width, int height, String metadata) {
        try {
            updateViewport(metadata, !mUpdateViewportOnEndDraw);
            mUpdateViewportOnEndDraw = false;
            Rect rect = new Rect(x, y, x + width, y + height);
            mTileLayer.invalidate(rect);
        } finally {
            endTransaction(mTileLayer);
        }
    }

    public ViewportMetrics getGeckoViewportMetrics() {
        
        if (mGeckoViewport != null)
            return new ViewportMetrics(mGeckoViewport);
        return null;
    }

    public Bitmap getBitmap() {
        try {
            Bitmap b = Bitmap.createBitmap(mBufferSize.width, mBufferSize.height,
                                           CairoUtils.cairoFormatTobitmapConfig(mFormat));
            b.copyPixelsFromBuffer(mBuffer.asIntBuffer());
            return b;
        } catch (OutOfMemoryError oom) {
            Log.w(LOGTAG, "Unable to create bitmap", oom);
            return null;
        }
    }

    
    public ByteBuffer lockBuffer() {
        return mBuffer;
    }

    



    public void unlockBuffer() {
        
    }

    @Override
    public void geometryChanged() {
        
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        if (metrics.widthPixels != mScreenSize.width ||
            metrics.heightPixels != mScreenSize.height) {
            mScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
            int maxSize = getLayerController().getView().getMaxTextureSize();

            
            if (mScreenSize.width > maxSize || mScreenSize.height > maxSize)
                throw new RuntimeException("Screen size of " + mScreenSize + " larger than maximum texture size of " + maxSize);

            
            mBufferSize = new IntSize(Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.width + LayerController.MIN_BUFFER.width)),
                                      Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.height + LayerController.MIN_BUFFER.height)));

            
            if (mBuffer != null) {
                GeckoAppShell.freeDirectBuffer(mBuffer);
                mBuffer = null;
            }

            
            mBuffer = GeckoAppShell.allocateDirectBuffer(mBufferSize.getArea() * 2);

            Log.i(LOGTAG, "Screen-size changed to " + mScreenSize);
            GeckoEvent event = new GeckoEvent(GeckoEvent.SIZE_CHANGED,
                                              mBufferSize.width, mBufferSize.height,
                                              metrics.widthPixels, metrics.heightPixels);
            GeckoAppShell.sendEventToGecko(event);
        }

        render();
    }

    @Override
    public void viewportSizeChanged() {
        mViewportSizeChanged = true;
    }

    @Override
    public void render() {
        adjustViewportWithThrottling();
    }

    private void adjustViewportWithThrottling() {
        if (!getLayerController().getRedrawHint())
            return;

        if (mPendingViewportAdjust)
            return;

        long timeDelta = System.currentTimeMillis() - mLastViewportChangeTime;
        if (timeDelta < MIN_VIEWPORT_CHANGE_DELAY) {
            getLayerController().getView().postDelayed(
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

    private void adjustViewport() {
        Log.i(LOGTAG, "Adjusting viewport");
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(getLayerController().getViewportMetrics());

        PointF viewportOffset = viewportMetrics.getOptimumViewportOffset(mBufferSize);
        viewportMetrics.setViewportOffset(viewportOffset);
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoEvent event = new GeckoEvent("Viewport:Change", viewportMetrics.toJSON());
        GeckoAppShell.sendEventToGecko(event);
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }

        mLastViewportChangeTime = System.currentTimeMillis();
    }

    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:Update".equals(event)) {
            beginTransaction(mTileLayer);
            try {
                updateViewport(message.getString("viewport"), false);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Unable to update viewport", e);
            } finally {
                endTransaction(mTileLayer);
            }
        } else if ("Viewport:UpdateLater".equals(event)) {
            if (!mTileLayer.inTransaction()) {
                Log.e(LOGTAG, "Viewport:UpdateLater called while not in transaction. You should be using Viewport:Update instead!");
            }
            mUpdateViewportOnEndDraw = true;
        }
    }
}

