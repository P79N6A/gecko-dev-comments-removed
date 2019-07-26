




package org.mozilla.gecko;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.ComposeShader;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;







public class LightweightThemeDrawable extends Drawable {
    private Paint mPaint;
    private Paint mColorPaint;
    private Paint mTexturePaint;

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
	
        if (mTexturePaint != null)
            canvas.drawPaint(mTexturePaint);

        
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

    







    public void setTexture(int textureId) {
        Shader.TileMode modeX = Shader.TileMode.REPEAT;
        Shader.TileMode modeY = Shader.TileMode.REPEAT;

        
        Bitmap texture = BitmapFactory.decodeResource(mResources, textureId);

        if (texture == null) {
            
            Drawable drawable = mResources.getDrawable(textureId);
            if (drawable != null && drawable instanceof BitmapDrawable) {
                BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
                texture = bitmapDrawable.getBitmap();
                modeX = bitmapDrawable.getTileModeX();
                modeY = bitmapDrawable.getTileModeY();
            }
        }

        
        if (texture != null) {
            mTexturePaint = new Paint(mPaint);
            mTexturePaint.setShader(new BitmapShader(texture, modeX, modeY));
        }
    }

    




    public void setColor(int color) {
        mColorPaint = new Paint(mPaint);
        mColorPaint.setColor(color);
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
