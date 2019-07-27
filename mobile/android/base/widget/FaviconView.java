




package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;
import org.mozilla.gecko.favicons.Favicons;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.widget.ImageView;





public class FaviconView extends ImageView {
    private Bitmap mIconBitmap;

    
    
    private Bitmap mUnscaledBitmap;

    
    
    
    private String mIconKey;

    private int mActualWidth;
    private int mActualHeight;

    
    private boolean mScalingExpected;

    
    private int mDominantColor;

    
    private static float sStrokeWidth;

    
    private static final Paint sStrokePaint;

    
    private static final Paint sBackgroundPaint;

    
    private final RectF mStrokeRect;

    
    private final RectF mBackgroundRect;

    
    private final boolean isDominantBorderEnabled;

    
    private final boolean isOverrideScaleTypeEnabled;

    
    static {
        sStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        sStrokePaint.setStyle(Paint.Style.STROKE);

        sBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        sBackgroundPaint.setStyle(Paint.Style.FILL);
    }

    public FaviconView(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.getTheme().obtainStyledAttributes(attrs, R.styleable.FaviconView, 0, 0);

        try {
            isDominantBorderEnabled = a.getBoolean(R.styleable.FaviconView_dominantBorderEnabled, true);
            isOverrideScaleTypeEnabled = a.getBoolean(R.styleable.FaviconView_overrideScaleType, true);
        } finally {
            a.recycle();
        }

        if (isOverrideScaleTypeEnabled) {
            setScaleType(ImageView.ScaleType.CENTER);
        }

        mStrokeRect = new RectF();
        mBackgroundRect = new RectF();

        if (sStrokeWidth == 0) {
            sStrokeWidth = getResources().getDisplayMetrics().density;
            sStrokePaint.setStrokeWidth(sStrokeWidth);
        }

        mStrokeRect.left = mStrokeRect.top = sStrokeWidth;
        mBackgroundRect.left = mBackgroundRect.top = sStrokeWidth * 2.0f;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh){
        super.onSizeChanged(w, h, oldw, oldh);

        
        if (w == mActualWidth && h == mActualHeight) {
            return;
        }

        mActualWidth = w;
        mActualHeight = h;

        mStrokeRect.right = w - sStrokeWidth;
        mStrokeRect.bottom = h - sStrokeWidth;
        mBackgroundRect.right = mStrokeRect.right - sStrokeWidth;
        mBackgroundRect.bottom = mStrokeRect.bottom - sStrokeWidth;

        formatImage();
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (isDominantBorderEnabled) {
            
            sBackgroundPaint.setColor(mDominantColor & 0x46FFFFFF);
            canvas.drawRect(mStrokeRect, sBackgroundPaint);

            sStrokePaint.setColor(mDominantColor);
            canvas.drawRoundRect(mStrokeRect, sStrokeWidth, sStrokeWidth, sStrokePaint);
        }
    }

    




    private void formatImage() {
        
        if (mIconBitmap == null || mActualWidth == 0 || mActualHeight == 0) {
            showNoImage();
            return;
        }

        if (mScalingExpected && mActualWidth != mIconBitmap.getWidth()) {
            scaleBitmap();
            
            mScalingExpected = false;
        }

        setImageBitmap(mIconBitmap);

        
        
        
        
        if (Math.abs(mIconBitmap.getWidth() - mActualWidth) > 3) {
            mDominantColor = Favicons.getFaviconColor(mIconKey);
            if (mDominantColor == -1) {
                mDominantColor = 0;
            }
        } else {
            mDominantColor = 0;
        }
    }

    private void scaleBitmap() {
        
        
        int doubledSize = mIconBitmap.getWidth()*2;
        if (mActualWidth > doubledSize) {
            
            
            mIconBitmap = Bitmap.createScaledBitmap(mIconBitmap, doubledSize, doubledSize, true);
        } else {
            
            mIconBitmap = Bitmap.createScaledBitmap(mIconBitmap, mActualWidth, mActualWidth, true);
        }
    }

    












    private void updateImageInternal(Bitmap bitmap, String key, boolean allowScaling) {
        if (bitmap == null) {
            showDefaultFavicon();
            return;
        }

        
        if (mUnscaledBitmap == bitmap) {
            return;
        }
        mUnscaledBitmap = bitmap;
        mIconBitmap = bitmap;
        mIconKey = key;
        mScalingExpected = allowScaling;

        
        formatImage();
    }

    public void showDefaultFavicon() {
        setImageResource(R.drawable.favicon_globe);
        mDominantColor = 0;
    }

    private void showNoImage() {
        setImageDrawable(null);
        mDominantColor = 0;
    }

    


    public void clearImage() {
        showNoImage();
        mUnscaledBitmap = null;
        mIconBitmap = null;
        mIconKey = null;
        mScalingExpected = false;
    }

    













    public void updateAndScaleImage(Bitmap bitmap, String key) {
        updateImageInternal(bitmap, key, true);
    }

    







    public void updateImage(Bitmap bitmap, String key) {
        updateImageInternal(bitmap, key, false);
    }

    public Bitmap getBitmap() {
        return mIconBitmap;
    }
}
