





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.TileLayer;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.util.Log;
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

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());

        RectF bounds;
        int[] cropRect;
        Rect position = getPosition();
        RectF viewport = context.viewport;

        if (repeats()) {
            bounds = new RectF(0.0f, 0.0f, viewport.width(), viewport.height());
            int width = Math.round(viewport.width());
            int height = Math.round(viewport.height());
            cropRect = new int[] { 0, 0, width, height };
        } else {
            bounds = getBounds(context);
            cropRect = new int[] { 0, 0, position.width(), position.height() };
        }

        float height = bounds.height();
        float left = bounds.left - viewport.left;
        float top = viewport.height() - (bounds.top + height - viewport.top);

        float[] coords = {
            
            left/viewport.width(), top/viewport.height(), 0,
            cropRect[0]/(float)position.width(), cropRect[1]/(float)position.height(),

            left/viewport.width(), (top+height)/viewport.height(), 0,
            cropRect[0]/(float)position.width(), cropRect[3]/(float)position.height(),

            (left+bounds.width())/viewport.width(), top/viewport.height(), 0,
            cropRect[2]/(float)position.width(), cropRect[1]/(float)position.height(),

            (left+bounds.width())/viewport.width(), (top+height)/viewport.height(), 0,
            cropRect[2]/(float)position.width(), cropRect[3]/(float)position.height()
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
    }
}

