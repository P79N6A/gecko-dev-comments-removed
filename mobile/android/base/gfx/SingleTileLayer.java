





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.TileLayer;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.RegionIterator;
import android.opengl.GLES20;
import android.util.Log;
import java.nio.FloatBuffer;
import javax.microedition.khronos.opengles.GL10;






public class SingleTileLayer extends TileLayer {
    private static final String LOGTAG = "GeckoSingleTileLayer";

    private Rect mMask;

    public SingleTileLayer(CairoImage image) { this(false, image); }

    public SingleTileLayer(boolean repeat, CairoImage image) {
        super(image, repeat ? TileLayer.PaintMode.REPEAT : TileLayer.PaintMode.NORMAL);
    }

    public SingleTileLayer(CairoImage image, TileLayer.PaintMode paintMode) {
        super(image, paintMode);
    }

    


    public void setMask(Rect aMaskRect) {
        mMask = aMaskRect;
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        RectF bounds;
        RectF textureBounds;
        RectF viewport = context.viewport;

        if (repeats()) {
            
            
            
            bounds = getBounds(context);
            textureBounds = new RectF(0.0f, 0.0f, bounds.width(), bounds.height());
            bounds = new RectF(0.0f, 0.0f, viewport.width(), viewport.height());
        } else if (stretches()) {
            
            
            bounds = new RectF(0.0f, 0.0f, context.pageSize.width, context.pageSize.height);
            textureBounds = bounds;
        } else {
            bounds = getBounds(context);
            textureBounds = bounds;
        }

        Rect intBounds = new Rect();
        bounds.roundOut(intBounds);
        Region maskedBounds = new Region(intBounds);
        if (mMask != null) {
            maskedBounds.op(mMask, Region.Op.DIFFERENCE);
            if (maskedBounds.isEmpty())
                return;
        }

        
        
        RegionIterator i = new RegionIterator(maskedBounds);
        for (Rect subRect = new Rect(); i.next(subRect);) {
            
            
            RectF subRectF = new RectF(Math.max(bounds.left, (float)subRect.left),
                                       Math.max(bounds.top, (float)subRect.top),
                                       Math.min(bounds.right, (float)subRect.right),
                                       Math.min(bounds.bottom, (float)subRect.bottom));

            
            
            int[] cropRect = new int[] { Math.round(subRectF.left - bounds.left),
                                         Math.round(bounds.bottom - subRectF.top),
                                         Math.round(subRectF.right - bounds.left),
                                         Math.round(bounds.bottom - subRectF.bottom) };

            float left = subRectF.left - viewport.left;
            float top = viewport.bottom - subRectF.bottom;
            float right = left + subRectF.width();
            float bottom = top + subRectF.height();

            float[] coords = {
                
                left/viewport.width(), bottom/viewport.height(), 0,
                cropRect[0]/textureBounds.width(), cropRect[1]/textureBounds.height(),

                left/viewport.width(), top/viewport.height(), 0,
                cropRect[0]/textureBounds.width(), cropRect[3]/textureBounds.height(),

                right/viewport.width(), bottom/viewport.height(), 0,
                cropRect[2]/textureBounds.width(), cropRect[1]/textureBounds.height(),

                right/viewport.width(), top/viewport.height(), 0,
                cropRect[2]/textureBounds.width(), cropRect[3]/textureBounds.height()
            };

            FloatBuffer coordBuffer = context.coordBuffer;
            int positionHandle = context.positionHandle;
            int textureHandle = context.textureHandle;

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());

            
            coordBuffer.position(0);
            coordBuffer.put(coords);

            
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}

