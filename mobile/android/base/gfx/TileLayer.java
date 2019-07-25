





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.opengl.GLES20;
import android.util.Log;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;





public abstract class TileLayer extends Layer {
    private static final String LOGTAG = "GeckoTileLayer";

    private final Rect mDirtyRect;
    private final CairoImage mImage;
    private final boolean mRepeat;
    private IntSize mSize;
    private boolean mSkipTextureUpdate;
    private int[] mTextureIDs;

    public TileLayer(boolean repeat, CairoImage image) {
        mRepeat = repeat;
        mImage = image;
        mSize = new IntSize(0, 0);
        mSkipTextureUpdate = false;

        IntSize bufferSize = mImage.getSize();
        mDirtyRect = new Rect();
    }

    @Override
    public IntSize getSize() { return mImage.getSize(); }

    protected boolean repeats() { return mRepeat; }
    protected int getTextureID() { return mTextureIDs[0]; }
    protected boolean initialized() { return mImage != null && mTextureIDs != null; }

    @Override
    protected void finalize() throws Throwable {
        if (mTextureIDs != null)
            TextureReaper.get().add(mTextureIDs);
    }

    



    public void invalidate(Rect rect) {
        if (!inTransaction())
            throw new RuntimeException("invalidate() is only valid inside a transaction");
        mDirtyRect.union(rect);
    }

    public void invalidate() {
        IntSize bufferSize = mImage.getSize();
        invalidate(new Rect(0, 0, bufferSize.width, bufferSize.height));
    }

    public boolean isDirty() {
        return mImage.getSize().isPositive() && (mTextureIDs == null || !mDirtyRect.isEmpty());
    }

    private void validateTexture() {
        






        IntSize bufferSize = mImage.getSize();
        IntSize textureSize = bufferSize;

        textureSize = bufferSize.nextPowerOfTwo();

        if (!textureSize.equals(mSize)) {
            mSize = textureSize;

            
            if (mTextureIDs != null) {
                TextureReaper.get().add(mTextureIDs);
                mTextureIDs = null;

                
                
                TextureReaper.get().reap();
            }
        }
    }

    
    public void setSkipTextureUpdate(boolean skip) {
        mSkipTextureUpdate = skip;
    }

    public boolean getSkipTextureUpdate() {
        return mSkipTextureUpdate;
    }

    @Override
    protected boolean performUpdates(RenderContext context) {
        super.performUpdates(context);

        if (mSkipTextureUpdate) {
            return false;
        }

        
        validateTexture();

        
        if (!mImage.getSize().isPositive())
            return true;

        
        if (mTextureIDs == null) {
            uploadFullTexture();
        } else {
            uploadDirtyRect(mDirtyRect);
        }

        mDirtyRect.setEmpty();

        return true;
    }

    private void uploadFullTexture() {
        IntSize bufferSize = mImage.getSize();
        uploadDirtyRect(new Rect(0, 0, bufferSize.width, bufferSize.height));
    }

    private void uploadDirtyRect(Rect dirtyRect) {
        
        if (dirtyRect.isEmpty())
            return;

        
        ByteBuffer imageBuffer = mImage.getBuffer();
        if (imageBuffer == null)
            return;

        boolean newlyCreated = false;

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            GLES20.glGenTextures(mTextureIDs.length, mTextureIDs, 0);
            newlyCreated = true;
        }

        IntSize bufferSize = mImage.getSize();
        Rect bufferRect = new Rect(0, 0, bufferSize.width, bufferSize.height);

        int cairoFormat = mImage.getFormat();
        CairoGLInfo glInfo = new CairoGLInfo(cairoFormat);

        bindAndSetGLParameters();

        if (newlyCreated || dirtyRect.contains(bufferRect)) {
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width,
                                mSize.height, 0, glInfo.format, glInfo.type, imageBuffer);
            return;
        }

        
        
        if (!Rect.intersects(dirtyRect, bufferRect)) {
            return;
        }

        






        Buffer viewBuffer = imageBuffer.slice();
        int bpp = CairoUtils.bitsPerPixelForCairoFormat(cairoFormat) / 8;
        int position = dirtyRect.top * bufferSize.width * bpp;
        if (position > viewBuffer.limit()) {
            Log.e(LOGTAG, "### Position outside tile! " + dirtyRect.top);
            return;
        }

        viewBuffer.position(position);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, dirtyRect.top, bufferSize.width,
                               Math.min(bufferSize.height - dirtyRect.top, dirtyRect.height()),
                               glInfo.format, glInfo.type, viewBuffer);
    }

    private void bindAndSetGLParameters() {
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                               GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                               GLES20.GL_LINEAR);

        int repeatMode = mRepeat ? GLES20.GL_REPEAT : GLES20.GL_CLAMP_TO_EDGE;
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, repeatMode);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, repeatMode);
    }
}

