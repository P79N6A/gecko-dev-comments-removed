




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.ui.ViewportController;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.nio.ByteBuffer;
import java.util.Timer;
import java.util.TimerTask;







public class GeckoSoftwareLayerClient extends LayerClient {
    private Context mContext;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;
    private final SingleTileLayer mTileLayer;
    private ViewportController mViewportController;

    
    private RectF mGeckoVisibleRect;

    private CairoImage mCairoImage;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 350L;
    private long mLastViewportChangeTime;
    private Timer mViewportRedrawTimer;

    
    private static final int PAGE_WIDTH = 980;      
    private static final int PAGE_HEIGHT = 1500;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mViewportController = new ViewportController(new IntSize(PAGE_WIDTH, PAGE_HEIGHT),
                                                     new RectF(0, 0, 1, 1));

        mWidth = LayerController.TILE_WIDTH;
        mHeight = LayerController.TILE_HEIGHT;
        mFormat = CairoImage.FORMAT_RGB16_565;

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
    public void init() {
        getLayerController().setRoot(mTileLayer);
        render();
    }

    public void beginDrawing() {
        mTileLayer.beginTransaction();
    }

    



    public void endDrawing(int x, int y, int width, int height, String metadata) {
        try {
            LayerController controller = getLayerController();
            if (controller == null)
                return;
            controller.notifyViewOfGeometryChange();

            try {
                JSONObject metadataObject = new JSONObject(metadata);
                float originX = (float)metadataObject.getDouble("x");
                float originY = (float)metadataObject.getDouble("y");
                mTileLayer.setOrigin(new PointF(originX, originY));
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }

            Rect rect = new Rect(x, y, x + width, y + height);
            mTileLayer.invalidate(rect);
        } finally {
            mTileLayer.endTransaction();
        }
    }

    
    public ByteBuffer lockBuffer() {
        return mBuffer;
    }

    



    public void unlockBuffer() {
        
    }

    
    public void setPageSize(IntSize pageSize) {
        mViewportController.setPageSize(pageSize);
        getLayerController().setPageSize(pageSize);
    }

    @Override
    public void geometryChanged() {
        mViewportController.setVisibleRect(getTransformedVisibleRect());
        render();
    }

    @Override
    public IntSize getPageSize() { return mViewportController.getPageSize(); }

    @Override
    public void render() {
        adjustViewportWithThrottling();
    }

    private void adjustViewportWithThrottling() {
        if (!getLayerController().getRedrawHint())
            return;

        if (System.currentTimeMillis() < mLastViewportChangeTime + MIN_VIEWPORT_CHANGE_DELAY) {
            if (mViewportRedrawTimer != null)
                return;

            mViewportRedrawTimer = new Timer();
            mViewportRedrawTimer.schedule(new TimerTask() {
                @Override
                public void run() {
                    
                    getLayerController().getView().post(new Runnable() {
                        @Override
                        public void run() {
                            mViewportRedrawTimer = null;
                            adjustViewportWithThrottling();
                        }
                    });
                }
            }, MIN_VIEWPORT_CHANGE_DELAY);
            return;
        }

        adjustViewport();
    }

    private void adjustViewport() {
        LayerController layerController = getLayerController();
        RectF visibleRect = layerController.getVisibleRect();
        RectF tileRect = mViewportController.widenRect(visibleRect);
        tileRect = mViewportController.clampRect(tileRect);

        int x = (int)Math.round(tileRect.left), y = (int)Math.round(tileRect.top);
        GeckoEvent event = new GeckoEvent("Viewport:Change", "{\"x\": " + x +
                                          ", \"y\": " + y + "}");
        GeckoAppShell.sendEventToGecko(event);

        mLastViewportChangeTime = System.currentTimeMillis();
    }

    
    private RectF getTransformedVisibleRect() {
        LayerController layerController = getLayerController();
        return mViewportController.transformVisibleRect(layerController.getVisibleRect(),
                                                        layerController.getPageSize());
    }
}

