




































package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.graphics.RectF;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.util.Log;
import java.nio.ByteBuffer;
import javax.microedition.khronos.opengles.GL10;




public class ScrollbarLayer extends TileLayer {
    private static final int BAR_SIZE = 8;  

    private final boolean mVertical;

    private ScrollbarLayer(CairoImage image, boolean vertical) {
        super(false, image);
        mVertical = vertical;
    }

    public static ScrollbarLayer create(boolean vertical) {
        ByteBuffer buffer = ByteBuffer.allocateDirect(4);
        buffer.put(3, (byte)127);   
        CairoImage image = new BufferedCairoImage(buffer, 1, 1, CairoImage.FORMAT_ARGB32);
        return new ScrollbarLayer(image, vertical);
    }

    @Override
    public void draw(RenderContext context) {
        if (!initialized())
            return;

        try {
            GLES11.glEnable(GL10.GL_BLEND);

            RectF rect = mVertical ? getVerticalRect(context) : getHorizontalRect(context);
            GLES11.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());

            float y = context.viewport.height() - rect.bottom;
            GLES11Ext.glDrawTexfOES(rect.left, y, 0.0f, rect.width(), rect.height());
        } finally {
            GLES11.glDisable(GL10.GL_BLEND);
        }
    }

    private RectF getVerticalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = viewport.height() * viewport.top / pageSize.height;
        float barEnd = viewport.height() * viewport.bottom / pageSize.height;
        return new RectF(viewport.width() - BAR_SIZE, barStart, viewport.width(), barEnd);
    }

    private RectF getHorizontalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = viewport.width() * viewport.left / pageSize.width;
        float barEnd = viewport.width() * viewport.right / pageSize.width;
        return new RectF(barStart, viewport.height() - BAR_SIZE, barEnd, viewport.height());
    }
}
