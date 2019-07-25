




































package org.mozilla.gecko.gfx;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.util.Log;
import java.nio.ByteBuffer;
import javax.microedition.khronos.opengles.GL10;
import org.mozilla.gecko.FloatUtils;




public class ScrollbarLayer extends TileLayer {
    public static final long FADE_DELAY = 500; 
    private static final float FADE_AMOUNT = 0.03f; 

    private static final int PADDING = 1;   
    private static final int BAR_SIZE = 6;
    private static final int CAP_RADIUS = (BAR_SIZE / 2);

    private static final int[] CROPRECT_MIDPIXEL = new int[] { CAP_RADIUS, CAP_RADIUS, 1, 1 };
    private static final int[] CROPRECT_TOPCAP = new int[] { 0, CAP_RADIUS, BAR_SIZE, -CAP_RADIUS };
    private static final int[] CROPRECT_BOTTOMCAP = new int[] { 0, BAR_SIZE, BAR_SIZE, -CAP_RADIUS };
    private static final int[] CROPRECT_LEFTCAP = new int[] { 0, BAR_SIZE, CAP_RADIUS, -BAR_SIZE };
    private static final int[] CROPRECT_RIGHTCAP = new int[] { CAP_RADIUS, BAR_SIZE, CAP_RADIUS, -BAR_SIZE };

    private final boolean mVertical;
    private final ByteBuffer mBuffer;
    private final Bitmap mBitmap;
    private final Canvas mCanvas;
    private float mOpacity;

    private ScrollbarLayer(CairoImage image, boolean vertical, ByteBuffer buffer) {
        super(false, image);
        mVertical = vertical;
        mBuffer = buffer;

        mBitmap = Bitmap.createBitmap(image.getWidth(), image.getHeight(), Bitmap.Config.ARGB_8888);
        mCanvas = new Canvas(mBitmap);
    }

    public static ScrollbarLayer create(boolean vertical) {
        
        
        int imageSize = nextPowerOfTwo(BAR_SIZE);
        ByteBuffer buffer = ByteBuffer.allocateDirect(imageSize * imageSize * 4);
        CairoImage image = new BufferedCairoImage(buffer, imageSize, imageSize, CairoImage.FORMAT_ARGB32);
        return new ScrollbarLayer(image, vertical, buffer);
    }

    




    public boolean fade() {
        if (FloatUtils.fuzzyEquals(mOpacity, 0.0f)) {
            return false;
        }
        beginTransaction();
        try {
            setOpacity(Math.max(mOpacity - FADE_AMOUNT, 0.0f));
            invalidate();
        } finally {
            endTransaction();
        }
        return true;
    }

    




    public boolean unfade() {
        if (FloatUtils.fuzzyEquals(mOpacity, 1.0f)) {
            return false;
        }
        beginTransaction();
        try {
            setOpacity(1.0f);
            invalidate();
        } finally {
            endTransaction();
        }
        return true;
    }

    private void setOpacity(float opacity) {
        mOpacity = opacity;

        Paint foregroundPaint = new Paint();
        foregroundPaint.setAntiAlias(true);
        foregroundPaint.setStyle(Paint.Style.FILL);
        
        foregroundPaint.setColor(Color.argb((int)Math.round(mOpacity * 127), 0, 0, 0));

        mCanvas.drawColor(Color.argb(0, 0, 0, 0), PorterDuff.Mode.CLEAR);
        mCanvas.drawCircle(CAP_RADIUS, CAP_RADIUS, CAP_RADIUS, foregroundPaint);

        mBitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }

    @Override
    public void draw(RenderContext context) {
        if (!initialized())
            return;

        try {
            GLES11.glEnable(GL10.GL_BLEND);

            Rect rect = RectUtils.round(mVertical ? getVerticalRect(context) : getHorizontalRect(context));
            GLES11.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());

            float viewHeight = context.viewport.height();

            
            
            GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, CROPRECT_MIDPIXEL, 0);
            GLES11Ext.glDrawTexfOES(rect.left, viewHeight - rect.bottom, 0.0f, rect.width(), rect.height());

            if (mVertical) {
                
                GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, CROPRECT_TOPCAP, 0);
                GLES11Ext.glDrawTexfOES(rect.left, viewHeight - rect.top, 0.0f, BAR_SIZE, CAP_RADIUS);
                
                GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, CROPRECT_BOTTOMCAP, 0);
                GLES11Ext.glDrawTexfOES(rect.left, viewHeight - (rect.bottom + CAP_RADIUS), 0.0f, BAR_SIZE, CAP_RADIUS);
            } else {
                
                GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, CROPRECT_LEFTCAP, 0);
                GLES11Ext.glDrawTexfOES(rect.left - CAP_RADIUS, viewHeight - rect.bottom, 0.0f, CAP_RADIUS, BAR_SIZE);
                
                GLES11.glTexParameteriv(GL10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, CROPRECT_RIGHTCAP, 0);
                GLES11Ext.glDrawTexfOES(rect.right, viewHeight - rect.bottom, 0.0f, CAP_RADIUS, BAR_SIZE);
            }
        } finally {
            GLES11.glDisable(GL10.GL_BLEND);
        }
    }

    private RectF getVerticalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = (viewport.height() * viewport.top / pageSize.height) + CAP_RADIUS;
        float barEnd = (viewport.height() * viewport.bottom / pageSize.height) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float right = viewport.width() - PADDING;
        return new RectF(right - BAR_SIZE, barStart, right, barEnd);
    }

    private RectF getHorizontalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = (viewport.width() * viewport.left / pageSize.width) + CAP_RADIUS;
        float barEnd = (viewport.width() * viewport.right / pageSize.width) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float bottom = viewport.height() - PADDING;
        return new RectF(barStart, bottom - BAR_SIZE, barEnd, bottom);
    }
}
