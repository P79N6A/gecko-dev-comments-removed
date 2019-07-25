




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.TileLayer;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.opengles.GL10;






public class SingleTileLayer extends TileLayer {
    private static final String LOGTAG = "GeckoSingleTileLayer";

    public SingleTileLayer(CairoImage image) { this(false, image); }

    public SingleTileLayer(boolean repeat, CairoImage image) {
        super(repeat, image);
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        
        Rect validTexture = getValidTextureArea();
        if (validTexture.isEmpty())
            return;

        GLES11.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());

        RectF bounds;
        int[] cropRect;
        IntSize size = getSize();
        RectF viewport = context.viewport;

        if (repeats()) {
            if (!validTexture.equals(new Rect(0, 0, size.width, size.height))) {
                Log.e(LOGTAG, "Drawing partial repeating textures is unsupported!");
            }

            bounds = new RectF(0.0f, 0.0f, viewport.width(), viewport.height());
            int width = Math.round(viewport.width());
            int height = Math.round(-viewport.height());
            cropRect = new int[] { 0, size.height, width, height };
        } else {
            bounds = getBounds(context, new FloatSize(size));

            float scaleFactor = bounds.width() / (float)size.width;
            bounds.left += validTexture.left * scaleFactor;
            bounds.top += validTexture.top * scaleFactor;
            bounds.right -= (size.width - validTexture.right) * scaleFactor;
            bounds.bottom -= (size.height - validTexture.bottom) * scaleFactor;

            cropRect = new int[] { validTexture.left,
                                   validTexture.bottom,
                                   validTexture.width(),
                                   -validTexture.height() };
        }

        GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, cropRect,
                                0);

        float height = bounds.height();
        float left = bounds.left - viewport.left;
        float top = viewport.height() - (bounds.top + height - viewport.top);

        GLES11Ext.glDrawTexfOES(left, top, 0.0f, bounds.width(), height);
    }
}

