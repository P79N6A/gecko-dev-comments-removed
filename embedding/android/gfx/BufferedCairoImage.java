




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.CairoImage;
import org.mozilla.fennec.gfx.CairoUtils;
import android.graphics.Bitmap;
import java.nio.ByteBuffer;


public class BufferedCairoImage extends CairoImage {
    private ByteBuffer mBuffer;
    private int mWidth, mHeight, mFormat;

    
    public BufferedCairoImage(ByteBuffer inBuffer, int inWidth, int inHeight, int inFormat) {
        mBuffer = inBuffer; mWidth = inWidth; mHeight = inHeight; mFormat = inFormat;
    }

    
    public BufferedCairoImage(Bitmap bitmap) {
        mFormat = CairoUtils.bitmapConfigToCairoFormat(bitmap.getConfig());
        mWidth = bitmap.getWidth();
        mHeight = bitmap.getHeight();
        mBuffer = ByteBuffer.allocateDirect(mWidth * mHeight * 4);
        bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }

    @Override
    public ByteBuffer lockBuffer() { return mBuffer; }
    @Override
    public int getWidth() { return mWidth; }
    @Override
    public int getHeight() { return mHeight; }
    @Override
    public int getFormat() { return mFormat; }
}

