




































package org.mozilla.gecko.gfx;

import android.graphics.Rect;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;





public abstract class TileLayer extends Layer {
    private static final String LOGTAG = "GeckoTileLayer";

    private final ArrayList<Rect> mDirtyRects;
    private final CairoImage mImage;
    private final boolean mRepeat;
    private final IntSize mSize;
    private int[] mTextureIDs;

    public TileLayer(boolean repeat, CairoImage image) {
        mRepeat = repeat;
        mImage = image;
        mSize = new IntSize(image.getWidth(), image.getHeight());
        mDirtyRects = new ArrayList<Rect>();

        



        int width = mImage.getWidth(), height = mImage.getHeight();
        if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
            throw new RuntimeException("TileLayer: NPOT images are unsupported (dimensions are " +
                                       width + "x" + height + ")");
        }
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
        
        
        if (mImage == null || mTextureIDs == null)
            return;

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        gl.glPushMatrix();

        onTileDraw(gl);

        gl.glPopMatrix();
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
    }

    



    public void invalidate(Rect rect) {
        if (!inTransaction())
            throw new RuntimeException("invalidate() is only valid inside a transaction");
        mDirtyRects.add(rect);
    }

    public void invalidate() {
        invalidate(new Rect(0, 0, mSize.width, mSize.height));
    }

    @Override
    protected void performUpdates(GL10 gl) {
        super.performUpdates(gl);

        if (mTextureIDs == null) {
            uploadFullTexture(gl);
        } else {
            for (Rect dirtyRect : mDirtyRects)
                uploadDirtyRect(gl, dirtyRect);
        }

        mDirtyRects.clear();
    }

    private void uploadFullTexture(GL10 gl) {
        mTextureIDs = new int[1];
        gl.glGenTextures(mTextureIDs.length, mTextureIDs, 0);

        int cairoFormat = mImage.getFormat();
        CairoGLInfo glInfo = new CairoGLInfo(cairoFormat);

        bindAndSetGLParameters(gl);

        gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width, mSize.height,
                        0, glInfo.format, glInfo.type, mImage.getBuffer());
    }

    private void uploadDirtyRect(GL10 gl, Rect dirtyRect) {
        if (mTextureIDs == null)
            throw new RuntimeException("uploadDirtyRect() called with null texture ID!");

        int width = mSize.width;
        int cairoFormat = mImage.getFormat();
        CairoGLInfo glInfo = new CairoGLInfo(cairoFormat);

        bindAndSetGLParameters(gl);

        




        Buffer viewBuffer = mImage.getBuffer().slice();
        int bpp = CairoUtils.bitsPerPixelForCairoFormat(cairoFormat) / 8;
        int position = dirtyRect.top * width * bpp;
        if (position > viewBuffer.limit()) {
            Log.e(LOGTAG, "### Position outside tile! " + dirtyRect.top);
            return;
        }

        viewBuffer.position(position);
        gl.glTexSubImage2D(gl.GL_TEXTURE_2D, 0, 0, dirtyRect.top, width, dirtyRect.height(),
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

