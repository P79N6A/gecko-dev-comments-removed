




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoAppShell;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.graphics.RectF;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;




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
        GLES11.glBindTexture(GL10.GL_TEXTURE_2D, mTextureIDs[0]);
        GLES11.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
        GLES11.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
    }

    @Override
    protected void finalize() throws Throwable {
        if (mTextureIDs != null)
            TextureReaper.get().add(mTextureIDs);
    }

    @Override
    protected boolean performUpdates(GL10 gl, RenderContext context) {
        super.performUpdates(gl, context);

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            GLES11.glGenTextures(1, mTextureIDs, 0);
        }

        bindAndSetGLParameters();
        GeckoAppShell.bindWidgetTexture();

        return true;
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        GLES11.glBindTexture(GL10.GL_TEXTURE_2D, mTextureIDs[0]);

        RectF bounds;
        int[] cropRect;
        IntSize size = getSize();
        RectF viewport = context.viewport;

        bounds = getBounds(context, new FloatSize(size));
        cropRect = new int[] { 0, size.height, size.width, -size.height };
        bounds.offset(-viewport.left, -viewport.top);

        GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, cropRect,
                                0);

        float top = viewport.height() - (bounds.top + bounds.height());

        
        
        while (GLES11.glGetError() != GLES11.GL_NO_ERROR);

        GLES11Ext.glDrawTexfOES(bounds.left, top, 0.0f, bounds.width(), bounds.height());
        int error = GLES11.glGetError();
        if (error != GLES11.GL_NO_ERROR) {
            Log.i(LOGTAG, "Failed to draw texture: " + error);
        }
    }
}

