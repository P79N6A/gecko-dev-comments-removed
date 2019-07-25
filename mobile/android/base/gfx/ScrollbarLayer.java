




































package org.mozilla.gecko.gfx;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import javax.microedition.khronos.opengles.GL10;




public class ScrollbarLayer extends SingleTileLayer {
    private static final int BAR_SIZE = 8;  

    



    private ScrollbarLayer(CairoImage image) {
        super(image);
    }

    public static ScrollbarLayer create() {
        Bitmap bitmap = Bitmap.createBitmap(BAR_SIZE, BAR_SIZE, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);

        Paint painter = new Paint();
        painter.setColor(Color.argb(127, 0, 0, 0));
        canvas.drawRect(0.0f, 0.0f, BAR_SIZE, BAR_SIZE, painter);

        return new ScrollbarLayer(new BufferedCairoImage(bitmap));
    }

    void drawVertical(GL10 gl, IntSize screenSize, Rect pageRect) {
        float barStart = (float)screenSize.height * (float)(0 - pageRect.top) / pageRect.height();
        float barEnd = (float)screenSize.height * (float)(screenSize.height - pageRect.top) / pageRect.height();
        float scale = Math.max(1.0f, (barEnd - barStart) / BAR_SIZE);
        gl.glLoadIdentity();
        gl.glScalef(1.0f, scale, 1.0f);
        gl.glTranslatef(screenSize.width - BAR_SIZE, barStart / scale, 0.0f);
        draw(gl);
    }

    void drawHorizontal(GL10 gl, IntSize screenSize, Rect pageRect) {
        float barStart = (float)screenSize.width * (float)(0 - pageRect.left) / pageRect.width();
        float barEnd = (float)screenSize.width * (float)(screenSize.width - pageRect.left) / pageRect.width();
        float scale = Math.max(1.0f, (barEnd - barStart) / BAR_SIZE);
        gl.glLoadIdentity();
        gl.glScalef(scale, 1.0f, 1.0f);
        gl.glTranslatef(barStart / scale, screenSize.height - BAR_SIZE, 0.0f);
        draw(gl);
    }
}
