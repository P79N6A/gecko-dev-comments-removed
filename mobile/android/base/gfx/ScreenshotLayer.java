




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.SingleTileLayer;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.RegionIterator;
import android.opengl.GLES20;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.nio.FloatBuffer;

public final class ScreenshotLayer extends SingleTileLayer {
    private static final int SCREENSHOT_SIZE_LIMIT = 1048576;
    private static final int BYTES_FOR_16BPP = 2;
    private ScreenshotImage mImage;
    
    private IntSize mBufferSize;
    
    
    private IntSize mImageSize;
    
    private boolean mHasImage;

    private static final String LOGTAG = "GeckoScreenshot";

    public static int getMaxNumPixels() {
        return SCREENSHOT_SIZE_LIMIT;
    }

    public void reset() {
        mHasImage = false;
    }

    void setBitmap(ByteBuffer data, int width, int height, Rect rect) {
        mImageSize = new IntSize(width, height);
        if (IntSize.isPowerOfTwo(width) && IntSize.isPowerOfTwo(height)) {
            mBufferSize = mImageSize;
            mHasImage = true;
            mImage.setBitmap(data, width, height, CairoImage.FORMAT_RGB16_565, rect);
        } else {
            Bitmap b = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
            b.copyPixelsFromBuffer(data);
            setBitmap(b);
        }
    }
    
    void setBitmap(Bitmap bitmap) {
        mImageSize = new IntSize(bitmap.getWidth(), bitmap.getHeight());
        int width = IntSize.nextPowerOfTwo(bitmap.getWidth());
        int height = IntSize.nextPowerOfTwo(bitmap.getHeight());
        mBufferSize = new IntSize(width, height);
        mImage.setBitmap(bitmap, width, height, CairoImage.FORMAT_RGB16_565);
        mHasImage = true;
    }

    public void updateBitmap(Bitmap bitmap, float x, float y, float width, float height) {
        mImage.updateBitmap(bitmap, x, y, width, height);
    }

    public static ScreenshotLayer create() {
        return ScreenshotLayer.create(new IntSize(4, 4));
    }

    public static ScreenshotLayer create(IntSize size) {
        Bitmap bitmap = Bitmap.createBitmap(size.width, size.height, Bitmap.Config.RGB_565);
        ScreenshotLayer sl = create(bitmap);
        sl.reset();
        return sl;
    }

    public static ScreenshotLayer create(Bitmap bitmap) {
        IntSize size = new IntSize(bitmap.getWidth(), bitmap.getHeight());
        
        ByteBuffer buffer = GeckoAppShell.allocateDirectBuffer(SCREENSHOT_SIZE_LIMIT * BYTES_FOR_16BPP);
        
        ScreenshotLayer sl =  new ScreenshotLayer(new ScreenshotImage(buffer, size.width, size.height, CairoImage.FORMAT_RGB16_565), size);
        
        sl.setBitmap(bitmap);
        return sl;
    }

    private ScreenshotLayer(ScreenshotImage image, IntSize size) {
        super(image, TileLayer.PaintMode.NORMAL);
        mBufferSize = size;
        mImage = image;
    }

    @Override
    public void draw(RenderContext context) {
        if (mHasImage)
            super.draw(context);
    }

    
    private static final class ScreenshotImage extends CairoImage {
        private ByteBuffer mBuffer;
        private IntSize mSize;
        private int mFormat;

        
        public ScreenshotImage(ByteBuffer inBuffer, int inWidth, int inHeight, int inFormat) {
            mBuffer = inBuffer;
            mSize = new IntSize(inWidth, inHeight);
            mFormat = inFormat;
        }

        @Override
        protected void finalize() throws Throwable {
            try {
                if (mBuffer != null) {
                    GeckoAppShell.freeDirectBuffer(mBuffer);
                    mBuffer = null;
                }
            } finally {
                super.finalize();
            }
        }

        void copyBuffer(ByteBuffer src, ByteBuffer dst, Rect rect, int stride) {
            int start = (rect.top * stride) + (rect.left * BYTES_FOR_16BPP);
            int end = ((rect.bottom - 1) * stride) + (rect.right * BYTES_FOR_16BPP);
            
            start = Math.max(0, Math.min(dst.limit(), Math.min(src.limit(), start)));
            end = Math.max(start, Math.min(dst.limit(), Math.min(src.capacity(), end)));
            dst.position(start);
            src.position(start).limit(end);
            dst.put(src);
        }

        synchronized void setBitmap(ByteBuffer data, int width, int height, int format, Rect rect) {
            mSize = new IntSize(width, height);
            mFormat = format;
            copyBuffer(data.asReadOnlyBuffer(), mBuffer.duplicate(), rect, width * BYTES_FOR_16BPP);
        }

        synchronized void setBitmap(Bitmap bitmap, int width, int height, int format) {
            Bitmap tmp;
            mSize = new IntSize(width, height);
            mFormat = format;
            if (width == bitmap.getWidth() && height == bitmap.getHeight()) {
                tmp = bitmap;
            } else {
                tmp = Bitmap.createBitmap(width, height, CairoUtils.cairoFormatTobitmapConfig(mFormat));
                new Canvas(tmp).drawBitmap(bitmap, 0.0f, 0.0f, new Paint());
            }
            tmp.copyPixelsToBuffer(mBuffer.asIntBuffer());
        }

        public void updateBitmap(Bitmap bitmap, float x, float y, float width, float height) {
            Bitmap tmp = Bitmap.createBitmap(mSize.width, mSize.height, CairoUtils.cairoFormatTobitmapConfig(mFormat));
            tmp.copyPixelsFromBuffer(mBuffer.asIntBuffer());
            Canvas c = new Canvas(tmp);
            c.drawBitmap(bitmap, x, y, new Paint());
            tmp.copyPixelsToBuffer(mBuffer.asIntBuffer());
        }

        @Override
        synchronized public ByteBuffer getBuffer() { return mBuffer; }
        @Override
        synchronized public IntSize getSize() { return mSize; }
        @Override
        synchronized public int getFormat() { return mFormat; }
    }
}
