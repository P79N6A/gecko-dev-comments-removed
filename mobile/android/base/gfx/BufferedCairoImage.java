




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import android.graphics.Bitmap;
import java.nio.ByteBuffer;


public class BufferedCairoImage extends CairoImage {
    private ByteBuffer mBuffer;
    private int mWidth, mHeight, mFormat;
    private boolean mNeedToFreeBuffer = false;

    
    public BufferedCairoImage(ByteBuffer inBuffer, int inWidth, int inHeight, int inFormat) {
        mBuffer = inBuffer; mWidth = inWidth; mHeight = inHeight; mFormat = inFormat;
    }

    
    public BufferedCairoImage(Bitmap bitmap) {
        mFormat = CairoUtils.bitmapConfigToCairoFormat(bitmap.getConfig());
        mWidth = bitmap.getWidth();
        mHeight = bitmap.getHeight();
        mNeedToFreeBuffer = true;
        mBuffer = GeckoAppShell.allocateDirectBuffer(mWidth * mHeight * 4);
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
    public int getWidth() { return mWidth; }
    @Override
    public int getHeight() { return mHeight; }
    @Override
    public int getFormat() { return mFormat; }
}

