




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.FloatSize;
import android.graphics.PointF;
import android.graphics.RectF;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;
import java.nio.FloatBuffer;







public class NinePatchTileLayer extends TileLayer {
    private static final int PATCH_SIZE = 16;
    private static final int TEXTURE_SIZE = 48;

    public NinePatchTileLayer(CairoImage image) {
        super(false, image);
    }

    @Override
    public void draw(RenderContext context) {
        if (!initialized())
            return;

        GLES11.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
        GLES11.glEnable(GL10.GL_BLEND);
        try {
            GLES11.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());
            drawPatches(context);
        } finally {
            GLES11.glDisable(GL10.GL_BLEND);
        }
    }

    private void drawPatches(RenderContext context) {
        











        FloatSize size = context.pageSize;
        float width = size.width, height = size.height;

        drawPatch(context, 0, 0,                                                    
                  0.0f, 0.0f, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE, 0,                                           
                  PATCH_SIZE, 0.0f, width, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE * 2, 0,                                       
                  PATCH_SIZE + width, 0.0f, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, 0, PATCH_SIZE,                                           
                  0.0f, PATCH_SIZE, PATCH_SIZE, height);
        drawPatch(context, PATCH_SIZE * 2, PATCH_SIZE,                              
                  PATCH_SIZE + width, PATCH_SIZE, PATCH_SIZE, height);
        drawPatch(context, 0, PATCH_SIZE * 2,                                       
                  0.0f, PATCH_SIZE + height, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE, PATCH_SIZE * 2,                              
                  PATCH_SIZE, PATCH_SIZE + height, width, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE * 2, PATCH_SIZE * 2,                          
                  PATCH_SIZE + width, PATCH_SIZE + height, PATCH_SIZE, PATCH_SIZE);
    }

    private void drawPatch(RenderContext context, int textureX, int textureY, float tileX,
                           float tileY, float tileWidth, float tileHeight) {
        int[] cropRect = { textureX, textureY + PATCH_SIZE, PATCH_SIZE, -PATCH_SIZE };
        GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, cropRect,
                                0);

        RectF viewport = context.viewport;
        float viewportHeight = viewport.height();
        float drawX = tileX - viewport.left - PATCH_SIZE;
        float drawY = viewportHeight - (tileY + tileHeight - viewport.top - PATCH_SIZE);
        GLES11Ext.glDrawTexfOES(drawX, drawY, 0.0f, tileWidth, tileHeight);
    }
}
