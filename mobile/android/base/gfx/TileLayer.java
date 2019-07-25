




































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.opengl.GLES20;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11Ext;
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

    private void validateTexture(GL10 gl) {
        






        IntSize bufferSize = mImage.getSize();
        IntSize textureSize = bufferSize;

        textureSize = bufferSize.nextPowerOfTwo();

        if (!textureSize.equals(mSize)) {
            mSize = textureSize;

            
            if (mTextureIDs != null) {
                TextureReaper.get().add(mTextureIDs);
                mTextureIDs = null;

                
                
                TextureReaper.get().reap(gl);
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
    protected boolean performUpdates(GL10 gl, RenderContext context) {
        super.performUpdates(gl, context);

        if (mSkipTextureUpdate) {
            return false;
        }

        
        validateTexture(gl);

        
        if (!mImage.getSize().isPositive())
            return true;

        
        if (mTextureIDs == null) {
            uploadFullTexture(gl);
        } else {
            uploadDirtyRect(gl, mDirtyRect);
        }

        mDirtyRect.setEmpty();

        return true;
    }

    private void uploadFullTexture(GL10 gl) {
        IntSize bufferSize = mImage.getSize();
        uploadDirtyRect(gl, new Rect(0, 0, bufferSize.width, bufferSize.height));
    }

    private void uploadDirtyRect(GL10 gl, Rect dirtyRect) {
        
        if (dirtyRect.isEmpty())
            return;

        
        ByteBuffer imageBuffer = mImage.getBuffer();
        if (imageBuffer == null)
            return;

        boolean newlyCreated = false;

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            gl.glGenTextures(mTextureIDs.length, mTextureIDs, 0);
            newlyCreated = true;
        }

        IntSize bufferSize = mImage.getSize();
        Rect bufferRect = new Rect(0, 0, bufferSize.width, bufferSize.height);

        int cairoFormat = mImage.getFormat();
        CairoGLInfo glInfo = new CairoGLInfo(cairoFormat);

        bindAndSetGLParameters(gl);

        if (newlyCreated || dirtyRect.contains(bufferRect)) {
            if (mSize.equals(bufferSize)) {
                gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width, mSize.height,
                                0, glInfo.format, glInfo.type, imageBuffer);
                return;
            } else {
                gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width, mSize.height,
                                0, glInfo.format, glInfo.type, null);
                gl.glTexSubImage2D(gl.GL_TEXTURE_2D, 0, 0, 0, bufferSize.width, bufferSize.height,
                                   glInfo.format, glInfo.type, imageBuffer);
                return;
            }
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
        gl.glTexSubImage2D(gl.GL_TEXTURE_2D, 0, 0, dirtyRect.top, bufferSize.width,
                           Math.min(bufferSize.height - dirtyRect.top, dirtyRect.height()),
                           glInfo.format, glInfo.type, viewBuffer);
    }

    private void bindAndSetGLParameters(GL10 gl) {
        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureIDs[0]);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);

        int repeatMode = mRepeat ? GL10.GL_REPEAT : GL10.GL_CLAMP_TO_EDGE;
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, repeatMode);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, repeatMode);
    }
}

