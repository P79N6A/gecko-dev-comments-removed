




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.TextureReaper;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;





public abstract class TileLayer extends Layer {
    private CairoImage mImage;
    private boolean mRepeat;
    private IntSize mSize;
    private int[] mTextureIDs;

    private IntRect mTextureUploadRect;
    

    public TileLayer(boolean repeat) {
        super();
        mRepeat = repeat;
        mTextureUploadRect = null;
    }

    public IntSize getSize() { return mSize; }

    protected boolean repeats() { return mRepeat; }
    protected int getTextureID() { return mTextureIDs[0]; }

    @Override
    protected void finalize() throws Throwable {
        if (mTextureIDs != null)
            TextureReaper.get().add(mTextureIDs);
    }

    




    protected abstract void onTileDraw(GL10 gl);

    @Override
    protected void onDraw(GL10 gl) {
        if (mImage == null)
            return;
        if (mTextureUploadRect != null)
            uploadTexture(gl);

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        gl.glPushMatrix();

        onTileDraw(gl);

        gl.glPopMatrix();
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
    }

    public void paintSubimage(CairoImage image, IntRect rect) {
        mImage = image;
        mTextureUploadRect = rect;

        



        int width = mImage.getWidth(), height = mImage.getHeight();
        assert (width & (width - 1)) == 0;
        assert (height & (height - 1)) == 0;
    }

    public void paintImage(CairoImage image) {
        paintSubimage(image, new IntRect(0, 0, image.getWidth(), image.getHeight()));
    }

    private void uploadTexture(GL10 gl) {
        boolean newTexture = mTextureIDs == null;
        if (newTexture) {
            mTextureIDs = new int[1];
            gl.glGenTextures(mTextureIDs.length, mTextureIDs, 0);
        }

        int width = mImage.getWidth(), height = mImage.getHeight();
        mSize = new IntSize(width, height);

        int cairoFormat = mImage.getFormat();
        int internalFormat = CairoUtils.cairoFormatToGLInternalFormat(cairoFormat);
        int format = CairoUtils.cairoFormatToGLFormat(cairoFormat);
        int type = CairoUtils.cairoFormatToGLType(cairoFormat);

        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureIDs[0]);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);

        int repeatMode = mRepeat ? GL10.GL_REPEAT : GL10.GL_CLAMP_TO_EDGE;
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, repeatMode);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, repeatMode);

        ByteBuffer buffer = mImage.lockBuffer();
        try {
            if (newTexture) {
                
                gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, internalFormat, mSize.width, mSize.height, 0,
                                format, type, buffer);
            } else {
                




                Buffer viewBuffer = buffer.slice();
                int bpp = CairoUtils.bitsPerPixelForCairoFormat(cairoFormat) / 8;
                viewBuffer.position(mTextureUploadRect.y * width * bpp);

                gl.glTexSubImage2D(gl.GL_TEXTURE_2D,
                                   0, 0, mTextureUploadRect.y, width, mTextureUploadRect.height,
                                   format, type, viewBuffer);
            }
        } finally {
            mImage.unlockBuffer();
        }

        mTextureUploadRect = null;
    }

    protected static FloatBuffer createBuffer(float[] values) {
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(values.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());

        FloatBuffer floatBuffer = byteBuffer.asFloatBuffer();
        floatBuffer.put(values);
        floatBuffer.position(0);
        return floatBuffer;
    }

    protected static void drawTriangles(GL10 gl, FloatBuffer vertexBuffer,
                                        FloatBuffer texCoordBuffer, int count) {
        gl.glVertexPointer(3, GL10.GL_FLOAT, 0, vertexBuffer);
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, texCoordBuffer);
        gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, count);
    }
}

