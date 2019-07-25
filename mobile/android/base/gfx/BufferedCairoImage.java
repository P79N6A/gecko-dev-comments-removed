




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import android.graphics.Bitmap;
import java.nio.ByteBuffer;


public class BufferedCairoImage extends CairoImage {
    private ByteBuffer mBuffer;
    private IntSize mSize;
    private int mFormat;
    private boolean mNeedToFreeBuffer = false;

    
    public BufferedCairoImage(ByteBuffer inBuffer, int inWidth, int inHeight, int inFormat) {
        mBuffer = inBuffer; mSize = new IntSize(inWidth, inHeight); mFormat = inFormat;
    }

    
    public BufferedCairoImage(Bitmap bitmap) {
        mFormat = CairoUtils.bitmapConfigToCairoFormat(bitmap.getConfig());
        mSize = new IntSize(bitmap.getWidth(), bitmap.getHeight());
        mNeedToFreeBuffer = true;
        
        mBuffer = GeckoAppShell.allocateDirectBuffer(mSize.getArea() * 4);
        bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }

     protected void finalize() throws Throwable {
        try {
            if (mNeedToFreeBuffer && mBuffer != null)
                GeckoAppShell.freeDirectBuffer(mBuffer);
            mNeedToFreeBuffer = false;
            mBuffer = null;
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
}

