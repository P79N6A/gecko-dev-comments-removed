





































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
    private int mWidth, mHeight, mFormat;
    private IntSize mScreenSize, mViewportSize;
    private ByteBuffer mBuffer;
    private final SingleTileLayer mTileLayer;

    
    private ViewportMetrics mGeckoViewport;

    private CairoImage mCairoImage;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 350L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mWidth = LayerController.TILE_WIDTH;
        mHeight = LayerController.TILE_HEIGHT;
        mFormat = CairoImage.FORMAT_RGB16_565;

        mScreenSize = new IntSize(1, 1);

        mBuffer = ByteBuffer.allocateDirect(mWidth * mHeight * 2);

        mCairoImage = new CairoImage() {
            @Override
            public ByteBuffer getBuffer() { return mBuffer; }
            @Override
            public int getWidth() { return mWidth; }
            @Override
            public int getHeight() { return mHeight; }
            @Override
            public int getFormat() { return mFormat; }
        };

        mTileLayer = new SingleTileLayer(mCairoImage);
    }

    
    @Override
    public void setLayerController(LayerController layerController) {
        super.setLayerController(layerController);

        layerController.setRoot(mTileLayer);
        if (mGeckoViewport != null)
            layerController.setViewportMetrics(mGeckoViewport);
        geometryChanged();
        GeckoAppShell.registerGeckoEventListener("Viewport:Update", this);
    }

    public void beginDrawing() {
        beginTransaction(mTileLayer);
    }

    private void updateViewport(String viewportDescription, final boolean onlyUpdatePageSize) {
        try {
            JSONObject viewportObject = new JSONObject(viewportDescription);
            mGeckoViewport = new ViewportMetrics(viewportObject);

            mTileLayer.setOrigin(PointUtils.round(mGeckoViewport.getDisplayportOrigin()));
            mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

            
            
            final LayerController controller = getLayerController();

            if (controller != null) {
                controller.post(new Runnable() {
                    @Override
                    public void run() {
                        if (onlyUpdatePageSize) {
                            
                            
                            if (FloatUtils.fuzzyEquals(controller.getZoomFactor(), mGeckoViewport.getZoomFactor()))
                                controller.setPageSize(mGeckoViewport.getPageSize());
                        } else {
                            controller.setViewportMetrics(mGeckoViewport);
                        }
                    }
                });
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Bad viewport description: " + viewportDescription);
            throw new RuntimeException(e);
        }
    }

    



    public void endDrawing(int x, int y, int width, int height, String metadata) {
        try {
            updateViewport(metadata, true);
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
        Bitmap b = Bitmap.createBitmap(mWidth, mHeight,
                                       CairoUtils.cairoFormatTobitmapConfig(mFormat));
        b.copyPixelsFromBuffer(mBuffer.asIntBuffer());
        return b;
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
            Log.i(LOGTAG, "Screen-size changed to " + mScreenSize);
            GeckoEvent event = new GeckoEvent(GeckoEvent.SIZE_CHANGED,
                                              LayerController.TILE_WIDTH, LayerController.TILE_HEIGHT,
                                              metrics.widthPixels, metrics.heightPixels);
            GeckoAppShell.sendEventToGecko(event);
        }

        render();
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

        PointF viewportOffset = viewportMetrics.getOptimumViewportOffset();
        viewportMetrics.setViewportOffset(viewportOffset);
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoEvent event = new GeckoEvent("Viewport:Change", viewportMetrics.toJSON());
        GeckoAppShell.sendEventToGecko(event);

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
        }
    }
}

