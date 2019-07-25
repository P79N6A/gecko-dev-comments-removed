





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.MultiTileLayer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.WidgetTileLayer;
import org.mozilla.gecko.GeckoAppShell;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import java.nio.ByteBuffer;







public class GeckoSoftwareLayerClient extends GeckoLayerClient {
    private static final String LOGTAG = "GeckoSoftwareLayerClient";

    private int mFormat;
    private IntSize mViewportSize;
    private ByteBuffer mBuffer;

    
    private Point mRenderOffset;

    private CairoImage mCairoImage;

    private static final IntSize TILE_SIZE = new IntSize(256, 256);

    
    private boolean mHasDirectTexture;

    public GeckoSoftwareLayerClient(Context context) {
        super(context);

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
    protected boolean handleDirectTextureChange(boolean hasDirectTexture) {
        if (mTileLayer != null && hasDirectTexture == mHasDirectTexture)
            return false;

        mHasDirectTexture = hasDirectTexture;

        if (mHasDirectTexture) {
            Log.i(LOGTAG, "Creating WidgetTileLayer");
            mTileLayer = new WidgetTileLayer(mCairoImage);
            mRenderOffset.set(0, 0);
        } else {
            Log.i(LOGTAG, "Creating MultiTileLayer");
            mTileLayer = new MultiTileLayer(mCairoImage, TILE_SIZE);
        }

        getLayerController().setRoot(mTileLayer);

        
        
        sendResizeEventIfNecessary(true);

        return true;
    }

    @Override
    protected boolean shouldDrawProceed(int tileWidth, int tileHeight) {
        
        
        if (mHasDirectTexture) {
            if (tileWidth != 0 || tileHeight != 0) {
                Log.e(LOGTAG, "Aborting draw, incorrect tile size of " + tileWidth + "x" +
                      tileHeight);
                return false;
            }
        } else {
            if (tileWidth != TILE_SIZE.width || tileHeight != TILE_SIZE.height) {
                Log.e(LOGTAG, "Aborting draw, incorrect tile size of " + tileWidth + "x" +
                      tileHeight);
                return false;
            }
        }

        return true;
    }

    @Override
    public boolean beginDrawing(int width, int height, int tileWidth, int tileHeight,
                                String metadata, boolean hasDirectTexture) {
        if (!super.beginDrawing(width, height, tileWidth, tileHeight, metadata,
                                hasDirectTexture)) {
            return false;
        }

        
        
        
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

    @Override
    protected void updateLayerAfterDraw(Rect updatedRect) {
        if (!(mTileLayer instanceof MultiTileLayer)) {
            return;
        }

        updatedRect.offset(mRenderOffset.x, mRenderOffset.y);
        ((MultiTileLayer)mTileLayer).invalidate(updatedRect);
        ((MultiTileLayer)mTileLayer).setRenderOffset(mRenderOffset);
    }

    private void copyPixelsFromMultiTileLayer(Bitmap target) {
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

    @Override
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
    public int getType() {
        return LAYER_CLIENT_TYPE_SOFTWARE;
    }

    @Override
    protected IntSize getBufferSize() {
        
        if (!mHasDirectTexture) {
            
            return new IntSize(((mScreenSize.width + LayerController.MIN_BUFFER.width - 1) /
                                    TILE_SIZE.width + 1) * TILE_SIZE.width,
                               ((mScreenSize.height + LayerController.MIN_BUFFER.height - 1) /
                                    TILE_SIZE.height + 1) * TILE_SIZE.height);
        }

        int maxSize = getLayerController().getView().getMaxTextureSize();

        
        if (mScreenSize.width > maxSize || mScreenSize.height > maxSize) {
            throw new RuntimeException("Screen size of " + mScreenSize +
                                       " larger than maximum texture size of " + maxSize);
        }

        
        
        return new IntSize(Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.width +
                                             LayerController.MIN_BUFFER.width)),
                           Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.height +
                                             LayerController.MIN_BUFFER.height)));
    }

    @Override
    protected IntSize getTileSize() {
        
        return !mHasDirectTexture ? TILE_SIZE : new IntSize(0, 0);
    }
}

