




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import android.graphics.Bitmap;
import java.nio.ByteBuffer;


public final class BufferedCairoImage extends CairoImage {
    private ByteBuffer mBuffer;
    private IntSize mSize;
    private int mFormat;
    private boolean mNeedToFreeBuffer;

    
    public BufferedCairoImage(ByteBuffer inBuffer, int inWidth, int inHeight, int inFormat) {
        setBuffer(inBuffer, inWidth, inHeight, inFormat);
    }

    
    public BufferedCairoImage(Bitmap bitmap) {
        setBitmap(bitmap);
    }

    private void freeBuffer() {
        if (mNeedToFreeBuffer && mBuffer != null)
            GeckoAppShell.freeDirectBuffer(mBuffer);
        mNeedToFreeBuffer = false;
        mBuffer = null;
    }

    protected void finalize() throws Throwable {
        try {
            freeBuffer();
        } finally {
            super.finalize();
        }
    }

    @Override
    public ByteBuffer getBuffer() { return mBuffer; }
    @Override
    public IntSize getSize() { return mSize; }
    @Override
    public int getFormat() { return mFormat; }


    public void setBuffer(ByteBuffer buffer, int width, int height, int format) {
        freeBuffer();
        mBuffer = buffer;
        mSize = new IntSize(width, height);
        mFormat = format;
    }

    public void setBitmap(Bitmap bitmap) {
        mFormat = CairoUtils.bitmapConfigToCairoFormat(bitmap.getConfig());
        mSize = new IntSize(bitmap.getWidth(), bitmap.getHeight());
        mNeedToFreeBuffer = true;

        int bpp = CairoUtils.bitsPerPixelForCairoFormat(mFormat);
        mBuffer = GeckoAppShell.allocateDirectBuffer(mSize.getArea() * bpp);
        bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }
}
