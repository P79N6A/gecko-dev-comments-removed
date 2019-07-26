




package org.mozilla.gecko;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.ComposeShader;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;






public class LightweightThemeDrawable extends Drawable {
    private Paint mPaint;
    private Paint mColorPaint;

    private Bitmap mBitmap;
    private Resources mResources;

    private int mStartColor;
    private int mEndColor;

    public LightweightThemeDrawable(Resources resources, Bitmap bitmap) {
        mBitmap = bitmap;
        mResources = resources;

        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStrokeWidth(0.0f);
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);
        initializeBitmapShader();
    }

    @Override
    public void draw(Canvas canvas) {
        
        if (mColorPaint != null)
            canvas.drawPaint(mColorPaint);

        
        canvas.drawPaint(mPaint);
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public void setAlpha(int alpha) {
        
        
        mPaint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter filter) {
        mPaint.setColorFilter(filter);
    }		

    




    public void setColor(int color) {
        mColorPaint = new Paint(mPaint);
        mColorPaint.setColor(color);
    }

    





    public void setColorWithFilter(int color, int filter) {
        mColorPaint = new Paint(mPaint);
        mColorPaint.setColor(color);
        mColorPaint.setColorFilter(new PorterDuffColorFilter(filter, PorterDuff.Mode.SRC_OVER));
    }

    





    public void setAlpha(int startAlpha, int endAlpha) {
        mStartColor = startAlpha << 24;
        mEndColor = endAlpha << 24;
        initializeBitmapShader();
    }

    private void initializeBitmapShader() {
	
        
        
	BitmapShader bitmapShader = new BitmapShader(mBitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);

	
	LinearGradient gradient = new LinearGradient(0, 0, 0, mBitmap.getHeight(), mStartColor, mEndColor, Shader.TileMode.CLAMP);

	
        
	
	mPaint.setShader(new ComposeShader(bitmapShader, gradient, PorterDuff.Mode.DST_IN));
    }
}
