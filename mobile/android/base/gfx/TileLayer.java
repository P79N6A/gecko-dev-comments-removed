





































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
    private IntSize mSize;
    private int[] mTextureIDs;

    public enum PaintMode { NORMAL, REPEAT, STRETCH };
    private PaintMode mPaintMode;

    public TileLayer(CairoImage image, PaintMode paintMode) {
        super(image.getSize());

        mPaintMode = paintMode;
        mImage = image;
        mSize = new IntSize(0, 0);
        mDirtyRect = new Rect();
    }

    protected boolean repeats() { return mPaintMode == PaintMode.REPEAT; }
    protected boolean stretches() { return mPaintMode == PaintMode.STRETCH; }
    protected int getTextureID() { return mTextureIDs[0]; }
    protected boolean initialized() { return mImage != null && mTextureIDs != null; }

    @Override
    protected void finalize() throws Throwable {
        if (mTextureIDs != null)
            TextureReaper.get().add(mTextureIDs);
    }

    @Override
    public void setPosition(Rect newPosition) {
        if (newPosition.width() != mImage.getSize().width || newPosition.height() != mImage.getSize().height) {
            throw new RuntimeException("Error: changing the size of a tile layer is not allowed!");
        }
        super.setPosition(newPosition);
    }

    




    public void invalidate() {
        if (!inTransaction())
            throw new RuntimeException("invalidate() is only valid inside a transaction");
        IntSize bufferSize = mImage.getSize();
        mDirtyRect.set(0, 0, bufferSize.width, bufferSize.height);
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

    @Override
    protected void performUpdates(RenderContext context) {
        super.performUpdates(context);

        
        validateTexture();

        
        if (!mImage.getSize().isPositive())
            return;

        
        if (mTextureIDs == null) {
            uploadFullTexture();
        } else {
            uploadDirtyRect(mDirtyRect);
        }

        mDirtyRect.setEmpty();
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

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            GLES20.glGenTextures(mTextureIDs.length, mTextureIDs, 0);
        }

        int cairoFormat = mImage.getFormat();
        CairoGLInfo glInfo = new CairoGLInfo(cairoFormat);

        bindAndSetGLParameters();

        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width,
                            mSize.height, 0, glInfo.format, glInfo.type, imageBuffer);
    }

    private void bindAndSetGLParameters() {
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                               GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                               GLES20.GL_LINEAR);

        int repeatMode = repeats() ? GLES20.GL_REPEAT : GLES20.GL_CLAMP_TO_EDGE;
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, repeatMode);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, repeatMode);
    }
}

