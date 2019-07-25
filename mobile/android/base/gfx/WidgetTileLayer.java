





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoAppShell;
import android.graphics.RectF;
import android.util.Log;
import android.opengl.GLES20;
import java.nio.FloatBuffer;




public class WidgetTileLayer extends Layer {
    private static final String LOGTAG = "WidgetTileLayer";

    private int[] mTextureIDs;
    private CairoImage mImage;

    public WidgetTileLayer(CairoImage image) {
        mImage = image;
    }

    protected boolean initialized() { return mTextureIDs != null; }

    @Override
    public IntSize getSize() { return mImage.getSize(); }

    protected void bindAndSetGLParameters() {
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
    }

    @Override
    protected void finalize() throws Throwable {
        if (mTextureIDs != null)
            TextureReaper.get().add(mTextureIDs);
    }

    @Override
    protected boolean performUpdates(RenderContext context) {
        super.performUpdates(context);

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            GLES20.glGenTextures(1, mTextureIDs, 0);
        }

        bindAndSetGLParameters();
        GeckoAppShell.bindWidgetTexture();

        return true;
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[0]);

        RectF bounds;
        int[] cropRect;
        IntSize size = getSize();
        RectF viewport = context.viewport;

        bounds = getBounds(context, new FloatSize(size));
        cropRect = new int[] { 0, 0, size.width, size.height };
        bounds.offset(-viewport.left, -viewport.top);

        float top = viewport.height() - (bounds.top + bounds.height());

        
        
        while (GLES20.glGetError() != GLES20.GL_NO_ERROR);

        float[] coords = {
            
            bounds.left/viewport.width(), top/viewport.height(), 0,
            cropRect[0]/size.width, cropRect[1]/size.height,

            bounds.left/viewport.width(), (top+bounds.height())/viewport.height(), 0,
            cropRect[0]/size.width, cropRect[3]/size.height,

            (bounds.left+bounds.width())/viewport.width(), top/viewport.height(), 0,
            cropRect[2]/size.width, cropRect[1]/size.height,

            (bounds.left+bounds.width())/viewport.width(), (top+bounds.height())/viewport.height(),
                0,
            cropRect[2]/size.width, cropRect[3]/size.height
        };

        
        FloatBuffer coordBuffer = context.coordBuffer;
        int positionHandle = context.positionHandle;
        int textureHandle = context.textureHandle;

        
        
        coordBuffer.position(0);
        coordBuffer.put(coords);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        int error = GLES20.glGetError();
        if (error != GLES20.GL_NO_ERROR) {
            Log.i(LOGTAG, "Failed to draw texture: " + error);
        }
    }
}

