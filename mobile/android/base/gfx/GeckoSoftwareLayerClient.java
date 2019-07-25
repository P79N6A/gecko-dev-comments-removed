





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.MultiTileLayer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.WidgetTileLayer;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
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
    private Layer mTileLayer;

    
    private ViewportMetrics mGeckoViewport;

    
    private ViewportMetrics mNewGeckoViewport;

    
    private Point mRenderOffset;

    private CairoImage mCairoImage;

    private static final IntSize TILE_SIZE = new IntSize(256, 256);

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 350L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;
    private boolean mViewportSizeChanged;

    
    private boolean mHasDirectTexture;

    
    
    
    
    
    private boolean mUpdateViewportOnEndDraw;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
        mFormat = CairoImage.FORMAT_RGB16_565;
        mRenderOffset = new Point(0, 0);

        mCairoImage = new CairoImage() {
            @Override
            public ByteBuffer getBuffer() { return mBuffer; }
            @Override
            public IntSize getSize() { return mBufferSize; }
            @Override
            public int getFormat() { return mFormat; }
        };
    }

    public int getWidth() {
        return mBufferSize.width;
    }

    public int getHeight() {
        return mBufferSize.height;
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
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateAndDraw", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateLater", this);
        GeckoAppShell.registerGeckoEventListener("Document:Shown", this);
        GeckoAppShell.registerGeckoEventListener("Tab:Selected", this);

        
        
        
        if (mTileLayer instanceof MultiTileLayer) {
            GeckoEvent event = new GeckoEvent(GeckoEvent.TILE_SIZE, TILE_SIZE);
            GeckoAppShell.sendEventToGecko(event);
        }

        sendResizeEventIfNecessary();
    }

    private boolean setHasDirectTexture(boolean hasDirectTexture) {
        if (mTileLayer != null && hasDirectTexture == mHasDirectTexture)
            return false;

        mHasDirectTexture = hasDirectTexture;

        IntSize tileSize;
        if (mHasDirectTexture) {
            mTileLayer = new WidgetTileLayer(mCairoImage);
            tileSize = new IntSize(0, 0);
            mRenderOffset.set(0, 0);
        } else {
            mTileLayer = new MultiTileLayer(mCairoImage, TILE_SIZE);
            tileSize = TILE_SIZE;
        }

        getLayerController().setRoot(mTileLayer);

        GeckoEvent event = new GeckoEvent(GeckoEvent.TILE_SIZE, tileSize);
        GeckoAppShell.sendEventToGecko(event);

        
        
        sendResizeEventIfNecessary(true);

        return true;
    }

    public boolean beginDrawing(int width, int height, String metadata, boolean hasDirectTexture) {
        
        if (setHasDirectTexture(hasDirectTexture))
            return false;

        beginTransaction(mTileLayer);

        try {
            JSONObject viewportObject = new JSONObject(metadata);
            mNewGeckoViewport = new ViewportMetrics(viewportObject);

            
            
            
            if (!(mTileLayer instanceof MultiTileLayer)) {
                if (mBufferSize.width != width || mBufferSize.height != height)
                    mBufferSize = new IntSize(width, height);
                return true;
            }

            
            
            boolean originChanged = true;
            Point origin = PointUtils.round(mNewGeckoViewport.getDisplayportOrigin());

            if (mGeckoViewport != null) {
                Point oldOrigin = PointUtils.round(mGeckoViewport.getDisplayportOrigin());
                originChanged = !origin.equals(oldOrigin);
            }

            if (originChanged) {
                Point tileOrigin = new Point((origin.x / TILE_SIZE.width) * TILE_SIZE.width,
                                             (origin.y / TILE_SIZE.height) * TILE_SIZE.height);
                mRenderOffset.set(origin.x - tileOrigin.x, origin.y - tileOrigin.y);
                ((MultiTileLayer)mTileLayer).invalidateBuffer();
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Bad viewport description: " + metadata);
            throw new RuntimeException(e);
        }

        if (mBufferSize.width != width || mBufferSize.height != height) {
            mBufferSize = new IntSize(width, height);

            
            
            IntSize realBufferSize = new IntSize(width + TILE_SIZE.width,
                                                 height + TILE_SIZE.height);

            
            int bpp = CairoUtils.bitsPerPixelForCairoFormat(mFormat) / 8;
            int size = realBufferSize.getArea() * bpp;
            if (mBuffer == null || mBuffer.capacity() != size) {
                
                if (mBuffer != null) {
                    GeckoAppShell.freeDirectBuffer(mBuffer);
                    mBuffer = null;
                }

                mBuffer = GeckoAppShell.allocateDirectBuffer(size);
            }
        }

        return true;
    }

    private void updateViewport(final boolean onlyUpdatePageSize) {
        
        
        
        
        FloatSize viewportSize = getLayerController().getViewportSize();
        mGeckoViewport = mNewGeckoViewport;
        mGeckoViewport.setSize(viewportSize);

        LayerController controller = getLayerController();
        PointF displayportOrigin = mGeckoViewport.getDisplayportOrigin();
        Point tileOrigin = PointUtils.round(displayportOrigin);
        tileOrigin.offset(-mRenderOffset.x, -mRenderOffset.y);
        mTileLayer.setOrigin(tileOrigin);
        mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

        if (onlyUpdatePageSize) {
            
            
            if (FloatUtils.fuzzyEquals(controller.getZoomFactor(),
                    mGeckoViewport.getZoomFactor()))
                controller.setPageSize(mGeckoViewport.getPageSize());
        } else {
            controller.setViewportMetrics(mGeckoViewport);
            controller.abortPanZoomAnimation();
        }
    }

    



    public void endDrawing(int x, int y, int width, int height) {
        synchronized (getLayerController()) {
            try {
                updateViewport(!mUpdateViewportOnEndDraw);
                mUpdateViewportOnEndDraw = false;

                if (mTileLayer instanceof MultiTileLayer) {
                    Rect rect = new Rect(x, y, x + width, y + height);
                    rect.offset(mRenderOffset.x, mRenderOffset.y);
                    ((MultiTileLayer)mTileLayer).invalidate(rect);
                }
            } finally {
                endTransaction(mTileLayer);
            }
        }
    }

    public ViewportMetrics getGeckoViewportMetrics() {
        
        if (mGeckoViewport != null)
            return new ViewportMetrics(mGeckoViewport);
        return null;
    }

    public void copyPixelsFromMultiTileLayer(Bitmap target) {
        Canvas c = new Canvas(target);
        ByteBuffer tileBuffer = mBuffer.slice();
        int bpp = CairoUtils.bitsPerPixelForCairoFormat(mFormat) / 8;

        for (int y = 0; y <= mBufferSize.height; y += TILE_SIZE.height) {
            for (int x = 0; x <= mBufferSize.width; x += TILE_SIZE.width) {
                
                Bitmap tile = Bitmap.createBitmap(TILE_SIZE.width, TILE_SIZE.height,
                                                  CairoUtils.cairoFormatTobitmapConfig(mFormat));
                tile.copyPixelsFromBuffer(tileBuffer.asIntBuffer());

                
                c.drawBitmap(tile, x - mRenderOffset.x, y - mRenderOffset.y, null);
                tile.recycle();

                
                tileBuffer.position(TILE_SIZE.getArea() * bpp);
                tileBuffer = tileBuffer.slice();
            }
        }
    }

    public Bitmap getBitmap() {
        if (mTileLayer == null)
            return null;

        
        
        beginTransaction(mTileLayer);
        try {
            if (mBuffer == null || mBufferSize.width <= 0 || mBufferSize.height <= 0)
                return null;
            try {
                Bitmap b = null;

                if (mTileLayer instanceof MultiTileLayer) {
                    b = Bitmap.createBitmap(mBufferSize.width, mBufferSize.height,
                                            CairoUtils.cairoFormatTobitmapConfig(mFormat));
                    copyPixelsFromMultiTileLayer(b);
                } else {
                    Log.w(LOGTAG, "getBitmap() called on a layer (" + mTileLayer + ") we don't know how to get a bitmap from");
                }

                return b;
            } catch (OutOfMemoryError oom) {
                Log.w(LOGTAG, "Unable to create bitmap", oom);
                return null;
            }
        } finally {
            endTransaction(mTileLayer);
        }
    }

    
    public ByteBuffer lockBuffer() {
        return mBuffer;
    }

    public Point getRenderOffset() {
        return mRenderOffset;
    }

    



    public void unlockBuffer() {
        
    }

    @Override
    public void geometryChanged() {
        
        sendResizeEventIfNecessary();
        render();
    }

    private void sendResizeEventIfNecessary() {
        sendResizeEventIfNecessary(false);
    }

    
    private void sendResizeEventIfNecessary(boolean force) {
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        if (!force && metrics.widthPixels == mScreenSize.width &&
            metrics.heightPixels == mScreenSize.height) {
            return;
        }

        mScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        IntSize bufferSize;

        
        if (mTileLayer instanceof MultiTileLayer) {
            
            bufferSize = new IntSize(((mScreenSize.width + LayerController.MIN_BUFFER.width - 1) / TILE_SIZE.width + 1) * TILE_SIZE.width,
                                     ((mScreenSize.height + LayerController.MIN_BUFFER.height - 1) / TILE_SIZE.height + 1) * TILE_SIZE.height);

        } else {
            int maxSize = getLayerController().getView().getMaxTextureSize();

            
            if (mScreenSize.width > maxSize || mScreenSize.height > maxSize)
                throw new RuntimeException("Screen size of " + mScreenSize + " larger than maximum texture size of " + maxSize);

            
            bufferSize = new IntSize(Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.width + LayerController.MIN_BUFFER.width)),
                                     Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.height + LayerController.MIN_BUFFER.height)));
        }

        Log.i(LOGTAG, "Screen-size changed to " + mScreenSize);
        GeckoEvent event = new GeckoEvent(GeckoEvent.SIZE_CHANGED,
                                          bufferSize.width, bufferSize.height,
                                          metrics.widthPixels, metrics.heightPixels);
        GeckoAppShell.sendEventToGecko(event);
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
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(getLayerController().getViewportMetrics());

        PointF viewportOffset = viewportMetrics.getOptimumViewportOffset(mBufferSize);
        viewportMetrics.setViewportOffset(viewportOffset);
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoAppShell.sendEventToGecko(new GeckoEvent(viewportMetrics));
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }

        mLastViewportChangeTime = System.currentTimeMillis();
    }

    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:UpdateAndDraw".equals(event)) {
            mUpdateViewportOnEndDraw = true;

            
            Rect rect = new Rect(0, 0, mBufferSize.width, mBufferSize.height);
            GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.DRAW, rect));
        } else if ("Viewport:UpdateLater".equals(event)) {
            mUpdateViewportOnEndDraw = true;
        } else if (("Document:Shown".equals(event) ||
                    "Tab:Selected".equals(event)) &&
                   (mTileLayer instanceof MultiTileLayer)) {
            beginTransaction(mTileLayer);
            try {
                ((MultiTileLayer)mTileLayer).invalidateTiles();
                ((MultiTileLayer)mTileLayer).invalidateBuffer();
            } finally {
                endTransaction(mTileLayer);
            }
        }
    }
}

