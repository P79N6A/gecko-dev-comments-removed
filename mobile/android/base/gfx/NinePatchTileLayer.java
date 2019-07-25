





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.FloatSize;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import javax.microedition.khronos.opengles.GL10;
import java.nio.FloatBuffer;
import android.opengl.GLES20;







public class NinePatchTileLayer extends TileLayer {
    private static final int PATCH_SIZE = 16;
    private static final int TEXTURE_SIZE = 64;

    public NinePatchTileLayer(CairoImage image) {
        super(false, image);
    }

    @Override
    public void draw(RenderContext context) {
        if (!initialized())
            return;

        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glEnable(GLES20.GL_BLEND);

	GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());
	drawPatches(context);
    }

    private void drawPatches(RenderContext context) {
        











        FloatSize size = context.pageSize;
        float width = size.width, height = size.height;

        drawPatch(context, 0, PATCH_SIZE * 3,                                              
                  0.0f, 0.0f, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE, PATCH_SIZE*3,                                       
                  PATCH_SIZE, 0.0f, width, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE * 2, PATCH_SIZE*3,                                   
                  PATCH_SIZE + width, 0.0f, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, 0, PATCH_SIZE * 2,                                              
                  0.0f, PATCH_SIZE, PATCH_SIZE, height);
        drawPatch(context, PATCH_SIZE * 2, PATCH_SIZE * 2,                                 
                  PATCH_SIZE + width, PATCH_SIZE, PATCH_SIZE, height);
        drawPatch(context, 0, PATCH_SIZE,                                                  
                  0.0f, PATCH_SIZE + height, PATCH_SIZE, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE, PATCH_SIZE,                                         
                  PATCH_SIZE, PATCH_SIZE + height, width, PATCH_SIZE);
        drawPatch(context, PATCH_SIZE * 2, PATCH_SIZE,                                     
                  PATCH_SIZE + width, PATCH_SIZE + height, PATCH_SIZE, PATCH_SIZE);
    }

    private void drawPatch(RenderContext context, int textureX, int textureY,
                           float tileX, float tileY, float tileWidth, float tileHeight) {
        RectF viewport = context.viewport;
        float viewportHeight = viewport.height();
        float drawX = tileX - viewport.left - PATCH_SIZE;
        float drawY = viewportHeight - (tileY + tileHeight - viewport.top - PATCH_SIZE);

        float[] coords = {
            
            drawX/viewport.width(), drawY/viewport.height(), 0,
            textureX/(float)TEXTURE_SIZE, textureY/(float)TEXTURE_SIZE,

            drawX/viewport.width(), (drawY+tileHeight)/viewport.height(), 0,
            textureX/(float)TEXTURE_SIZE, (textureY+PATCH_SIZE)/(float)TEXTURE_SIZE,

            (drawX+tileWidth)/viewport.width(), drawY/viewport.height(), 0,
            (textureX+PATCH_SIZE)/(float)TEXTURE_SIZE, textureY/(float)TEXTURE_SIZE,

            (drawX+tileWidth)/viewport.width(), (drawY+tileHeight)/viewport.height(), 0,
            (textureX+PATCH_SIZE)/(float)TEXTURE_SIZE, (textureY+PATCH_SIZE)/(float)TEXTURE_SIZE

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

        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
                               GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
                               GLES20.GL_CLAMP_TO_EDGE);

        
        
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                               GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                               GLES20.GL_LINEAR);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }
}
