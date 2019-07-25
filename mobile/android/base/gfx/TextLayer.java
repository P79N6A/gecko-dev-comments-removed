




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.SingleTileLayer;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;




public class TextLayer extends SingleTileLayer {
    private ByteBuffer mBuffer;
    private BufferedCairoImage mImage;
    private IntSize mSize;
    private String mText;

    public TextLayer(IntSize size) {
        super(false);

        mBuffer = ByteBuffer.allocateDirect(size.width * size.height * 4);
        mSize = size;
        mImage = new BufferedCairoImage(mBuffer, size.width, size.height,
                                        CairoImage.FORMAT_ARGB32);
        mText = "";
    }

    public void setText(String text) {
        mText = text;
        renderText();
        paintImage(mImage);
    }

    private void renderText() {
        Bitmap bitmap = Bitmap.createBitmap(mSize.width, mSize.height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);

        Paint textPaint = new Paint();
        textPaint.setAntiAlias(true);
        textPaint.setColor(Color.WHITE);
        textPaint.setFakeBoldText(true);
        textPaint.setTextSize(18.0f);
        textPaint.setTypeface(Typeface.DEFAULT_BOLD);
        float width = textPaint.measureText(mText) + 18.0f;

        Paint backgroundPaint = new Paint();
        backgroundPaint.setColor(Color.argb(127, 0, 0, 0));
        canvas.drawRect(0.0f, 0.0f, width, 18.0f + 6.0f, backgroundPaint);

        canvas.drawText(mText, 6.0f, 18.0f, textPaint);

        bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }
}

