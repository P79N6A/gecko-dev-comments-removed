




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.TileLayer;
import android.graphics.PointF;
import android.graphics.RectF;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.opengles.GL10;






public class SingleTileLayer extends TileLayer {
    public SingleTileLayer(CairoImage image) { this(false, image); }

    public SingleTileLayer(boolean repeat, CairoImage image) {
        super(repeat, image);
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        GLES11.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());

        RectF bounds;
        int[] cropRect;
        IntSize size = getSize();
        RectF viewport = context.viewport;

        if (repeats()) {
            bounds = new RectF(0.0f, 0.0f, viewport.width(), viewport.height());
            int width = (int)Math.round(viewport.width());
            int height = (int)Math.round(-viewport.height());
            cropRect = new int[] { 0, size.height, width, height };
        } else {
            bounds = getBounds(context, new FloatSize(size));
            cropRect = new int[] { 0, size.height, size.width, -size.height };
        }

        GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, cropRect,
                                0);

        float height = bounds.height();
        float left = bounds.left - viewport.left;
        float top = viewport.height() - (bounds.top + height - viewport.top);

        GLES11Ext.glDrawTexfOES(left, top, 0.0f, bounds.width(), height);
    }
}

