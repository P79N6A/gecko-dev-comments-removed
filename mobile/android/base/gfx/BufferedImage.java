




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.DirectBufferAllocator;

import android.graphics.Bitmap;
import android.util.Log;

import java.nio.ByteBuffer;


public class BufferedImage {
    private ByteBuffer mBuffer;
    private Bitmap mBitmap;
    private IntSize mSize;
    private int mFormat;

    private static final String LOGTAG = "GeckoBufferedImage";

    
    public BufferedImage() {
        mSize = new IntSize(0, 0);
    }

    
    public BufferedImage(Bitmap bitmap) {
        mFormat = bitmapConfigToFormat(bitmap.getConfig());
        mSize = new IntSize(bitmap.getWidth(), bitmap.getHeight());
        mBitmap = bitmap;
    }

    private synchronized void freeBuffer() {
        if (mBuffer != null) {
            mBuffer = DirectBufferAllocator.free(mBuffer);
        }
    }

    public void destroy() {
        try {
            freeBuffer();
        } catch (Exception ex) {
            Log.e(LOGTAG, "error clearing buffer: ", ex);
        }
    }

    public ByteBuffer getBuffer() {
        if (mBuffer == null) {
            int bpp = bitsPerPixelForFormat(mFormat);
            mBuffer = DirectBufferAllocator.allocate(mSize.getArea() * bpp);
            mBitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
            mBitmap = null;
        }
        return mBuffer;
    }

    public IntSize getSize() { return mSize; }
    public int getFormat() { return mFormat; }

    public static final int FORMAT_INVALID = -1;
    public static final int FORMAT_ARGB32 = 0;
    public static final int FORMAT_RGB24 = 1;
    public static final int FORMAT_A8 = 2;
    public static final int FORMAT_A1 = 3;
    public static final int FORMAT_RGB16_565 = 4;

    private static int bitsPerPixelForFormat(int format) {
        switch (format) {
        case FORMAT_A1:          return 1;
        case FORMAT_A8:          return 8;
        case FORMAT_RGB16_565:   return 16;
        case FORMAT_RGB24:       return 24;
        case FORMAT_ARGB32:      return 32;
        default:
            throw new RuntimeException("Unknown Cairo format");
        }
    }

    private static int bitmapConfigToFormat(Bitmap.Config config) {
        if (config == null)
            return FORMAT_ARGB32;    

        switch (config) {
        case ALPHA_8:   return FORMAT_A8;
        case ARGB_4444: throw new RuntimeException("ARGB_444 unsupported");
        case ARGB_8888: return FORMAT_ARGB32;
        case RGB_565:   return FORMAT_RGB16_565;
        default:        throw new RuntimeException("Unknown Skia bitmap config");
        }
    }
}
